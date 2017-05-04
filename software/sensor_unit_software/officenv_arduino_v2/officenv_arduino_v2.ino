/* Sensor unit software for the office environment monitoring and control
   system. This is an upgraded program for the updated software, in which
   the arduino board does not control the ESP8266 module, but sends the
   measurement data via the serial to the ESP module. The ESP module has
   its own program which manages the wifi and MQTT connections and reads
   data from the serial to relay them to the wifi.

   Xiaoke Yang (das.xiaoke@hotmail.com)
   IFFPC, Beihang University
   Last Modified: Mon 16 Jan 2017 16:50:19 CST
*/
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "DHT.h"               /* DHT Library by Adafruit version 1.2.2 */
#include "SoftwareSerial.h"



/* TFT LCD pins */
#define TFT_RST 8
#define TFT_DC 9
#define TFT_CS 10
/* Use hardware SPI (on Pro Mini, #13, #12, #11) and the above for CS/DC */
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

/* DHT data pin */
#define DHTPIN 7

/* DHT 22  (AM2302), AM2321 */
#define DHTTYPE DHT22

/* Initialize DHT sensor.
   Note that older versions of this library took an optional third parameter to
   tweak the timings for faster processors.  This parameter is no longer needed
   as the current DHT reading algorithm adjusts itself to work on faster procs.
*/
DHT dht(DHTPIN, DHTTYPE);

/* Tx and Rx pins for the PM sensor */
#define PM_RX 2
#define PM_TX 3
/* a software serial is used to connect to the PM sensor */
SoftwareSerial pmSerial(PM_TX, PM_RX);


/* some dimensions of the UI on the LCD display */
#define PARAM_CURSOR_X  220
#define PARAM_WIDTH    100
#define PARAM_HEIGHT 24
#define FONT_SIZE   3
#define LINE_SPACE 5
#define TOP_MARGIN 2
#define LEFT_MARGIN 2

/* global variables for PM2.5, PM10, humidity, temperature and heat index */
float pm2, pm10;
float h, t, hic;
/* This is a scaling factor for the data, since the serial transmits only
   integer values for the convenience of parsing in the ESP module */
int scale = 10;

/* data buffer */
char serial_out_buf[50];
char serial_in_buf[25];
char idx = 0;
boolean serial_read_complete = 0;

/* the original device ID is no longer used here, but embedded in the MQTT
   topics programmed into the ESP module
*/
/* #define devID 1 */

/* data transimission interval in micro-seconds */
#define TS_MS 20000
/* DHT, PM sensor and LCD refresh interval in micro-seconds */
#define TS_DHT_MS 5000



/* wifi connected flag */
boolean wifi_connected = 0;
/* MQTT server connected flag */
boolean mqtt_connected = 0;
/* wifi connected flag */
boolean wifi_connected_prev = 0;
/* MQTT server connected flag */
boolean mqtt_connected_prev = 0;

/* IP address */
char ip_addr[16];
/* data transimittion and measurement refresh timing */
long millis_prev = 0;
long millis_dht_prev = 0;


void setup() {
  /* set up the tft screen */
  init_lcd();
  /* display initial wifi connection information */
  update_wifi_info(1, 1, 0, 0, ip_addr);
  /* start the dht sensor */
  dht.begin();
  /* start the software serial connecting the PM sensor */
  pmSerial.begin(9600);
  /* start the serial connection with the ESP module */
  Serial.begin(19200);
}


void loop(void) {
  /* read the pm sensor at each loop to empty the input buffer of the serial */
  /* do not put get_pms() inside the interval check */
  get_pms();
  /* if the given sampling period is reached, or millis() function overflows */
  if (millis() - millis_dht_prev > TS_DHT_MS || millis() - millis_dht_prev < 0 ) {
    get_dht();
    millis_dht_prev = millis();
    display_env_params(t, hic, h, pm2, pm10);
  }
  if (millis() - millis_prev > TS_MS || millis() - millis_prev < 0 ) {
    send_data_to_esp(scale, t, h, pm2, pm10, serial_out_buf);
    millis_prev = millis();
  }
  /* now we check responses from the ESP module on the serial port */
  // if there's any serial available, read it:
  while (Serial.available() > 0) {
    char inchar = Serial.read();
    if (inchar == '\n') {
      serial_read_complete = 1;
      break;
    }
    serial_in_buf[idx] = inchar;
    idx++;
  }
  if (serial_read_complete) {
    /* save previous flags */
    wifi_connected_prev = wifi_connected;
    mqtt_connected_prev = mqtt_connected;
    /* read the wifi connection status, 1-connected, 0-not connected */
    if (serial_in_buf[0] == '1') {
      wifi_connected = 1;
    } else {
      wifi_connected = 0;
    }
    /* read the MQTT connection status, 1-connected, 0-not connected */
    if (serial_in_buf[2] == '1') {
      mqtt_connected = 1;
    } else {
      mqtt_connected = 0;
    }
    /* read the IP address */
    memset(ip_addr, 0, sizeof(ip_addr));
    memcpy(ip_addr,&serial_in_buf[4],16);
    /* now update the wifi and MQTT connection status */
    update_wifi_info(wifi_connected_prev, mqtt_connected_prev, wifi_connected, mqtt_connected, ip_addr);
    /* reset the index */
    idx = 0;
    serial_read_complete = 0;
    /* clear the buffer */
    memset(serial_in_buf, 0, sizeof(serial_in_buf));
  }

}

/*
   Initialise the LCD screen
*/
void init_lcd() {
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.drawFastHLine(0, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 , 320 , ILI9341_WHITE);
  tft.setCursor(LEFT_MARGIN, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 + 5);
  /* print connection information */
  tft.setTextSize(1);
  tft.print("Wi-Fi: ");
  tft.setCursor(LEFT_MARGIN + 160, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 + 5);
  tft.println("MQTT server: ");
  tft.print("IP Address:");
}

/* get the PM sensor's readings */
void get_pms() {
  char buf[10];
  if (pmSerial.available()) {
    byte c = pmSerial.read();
    if (c == B10101010) {
      c = pmSerial.read();
      if (c == B11000000) {
        /* read data */
        pmSerial.readBytes(buf, 8);
        byte hbyte, lbyte;
        hbyte = buf[1];
        lbyte = buf[0];
        int tmp1 = ((hbyte << 8) + lbyte);
        if (tmp1 > 0 && tmp1 < 10000) {
          pm2 = tmp1 / 10.0;
        }
        /* PM10 */
        hbyte = buf[3];
        lbyte = buf[2];
        tmp1 = ((hbyte << 8) + lbyte);
        if (tmp1 > 0 && tmp1 < 10000) {
          pm10 = tmp1 / 10.0;
        }
      }
    }
  }
}



void get_dht() {
  /* Reading temperature or humidity takes about 250 milliseconds! */
  /* Sensor readings may also be up to 2 seconds 'old' */
  float tmp_h = dht.readHumidity();
  delay(1);
  /* Read temperature as Celsius (the default) */
  float tmp_t = dht.readTemperature();
  /* Check if any reads failed and exit early (to try again). */
  if (isnan(tmp_h) || isnan(tmp_t)) {
  } else {
    h = tmp_h;
    t = tmp_t;
    /* Compute heat index in Celsius (isFahreheit = false) */
    hic = dht.computeHeatIndex(t, h, false);
  }
}


/**
   Display values on the LCD Screen
 **/
void display_env_params(float T, float HIC, float H, float PM2, float PM10) {
  tft.setTextSize(FONT_SIZE);
  /* Temperature */
  tft.setCursor(LEFT_MARGIN, TOP_MARGIN);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("Temp(C)");
  tft.setCursor(LEFT_MARGIN + PARAM_CURSOR_X, TOP_MARGIN);
  tft.fillRect(LEFT_MARGIN + PARAM_CURSOR_X, TOP_MARGIN, PARAM_WIDTH, PARAM_HEIGHT, ILI9341_BLACK);
  tft.setTextColor(ILI9341_GREEN);
  tft.print(T);

  /* Humidity */
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(LEFT_MARGIN , TOP_MARGIN + PARAM_HEIGHT + LINE_SPACE);
  tft.print("Humidity(%)");
  tft.setCursor(LEFT_MARGIN + PARAM_CURSOR_X, TOP_MARGIN + PARAM_HEIGHT + LINE_SPACE);
  tft.fillRect(LEFT_MARGIN + PARAM_CURSOR_X, TOP_MARGIN + PARAM_HEIGHT + LINE_SPACE, PARAM_WIDTH, PARAM_HEIGHT, ILI9341_BLACK);
  tft.setTextColor(ILI9341_GREEN);
  tft.print(H);

  /* Heat Index */
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(LEFT_MARGIN, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 2);
  tft.print("Heat Index");
  tft.setCursor(LEFT_MARGIN + PARAM_CURSOR_X, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 2);
  tft.fillRect(LEFT_MARGIN + PARAM_CURSOR_X, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 2, PARAM_WIDTH, PARAM_HEIGHT, ILI9341_BLACK);
  tft.setTextColor(ILI9341_GREEN);
  tft.print(HIC);

  /* PM2.5 */
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(LEFT_MARGIN , TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 3);
  tft.print("PM2.5(ug/m3)");
  tft.setCursor(LEFT_MARGIN + PARAM_CURSOR_X, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 3);
  tft.fillRect(LEFT_MARGIN + PARAM_CURSOR_X, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 3, PARAM_WIDTH, PARAM_HEIGHT, ILI9341_BLACK);
  tft.setTextColor(ILI9341_GREEN);
  tft.print(PM2, 1);

  /* PM10 */
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(LEFT_MARGIN , TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 4);
  tft.print("PM10(ug/m3)");
  tft.setCursor(LEFT_MARGIN + PARAM_CURSOR_X, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 4);
  tft.fillRect(LEFT_MARGIN + PARAM_CURSOR_X, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 4, PARAM_WIDTH, PARAM_HEIGHT, ILI9341_BLACK);
  tft.setTextColor(ILI9341_GREEN);
  tft.println(PM10, 1);
}

/*
    Send the measurement data to the ESP module via the serial port
*/
void send_data_to_esp(int SF, float T, float H, float PM2, float PM10, char *buf) {
  /* the data format is scale, PM2.5, PM10, temperature, humidity */
  sprintf(buf, "%d,%d,%d,%d,%d\n", SF, (int)(PM2 * SF), (int)(PM10 * SF), (int)(T * SF), (int)(H * SF));
  Serial.print(buf);

}

/* Update the wifi information display */
void update_wifi_info(boolean wifi_conn_prev, boolean mqtt_conn_prev, boolean wifi_conn, boolean mqtt_conn, char *ip) {
  if (wifi_conn_prev != wifi_conn || mqtt_conn_prev != mqtt_conn) {
    if ( wifi_conn ) {
      /* wifi connection */
      tft.setTextColor(ILI9341_GREEN);
      tft.setTextSize(1);
      tft.fillRect(LEFT_MARGIN + 6 * 7, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 + 5, 6 * 12, 8, ILI9341_BLACK);
      tft.setCursor(LEFT_MARGIN + 6 * 7, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 + 5);
      tft.print("connected");
      /* ip address */
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(1);
      tft.fillRect(LEFT_MARGIN + 6 * 12, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 + 5 + 8, 6 * 20, 8, ILI9341_BLACK);
      tft.setCursor(LEFT_MARGIN + 6 * 12, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 + 5 + 8);
      tft.print(ip);
      /* mqtt connection */
      if ( mqtt_conn ) {
        tft.setTextColor(ILI9341_GREEN);
        tft.setTextSize(1);
        tft.fillRect(LEFT_MARGIN + 160 + 6 * 13, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 + 5, 6 * 12, 8, ILI9341_BLACK);
        tft.setCursor(LEFT_MARGIN + 160 + 6 * 13, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 + 5);
        tft.print("connected");
      } else {
        tft.setTextColor(ILI9341_RED);
        tft.setTextSize(1);
        tft.fillRect(LEFT_MARGIN + 160 + 6 * 13, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 + 5, 6 * 12, 8, ILI9341_BLACK);
        tft.setCursor(LEFT_MARGIN + 160 + 6 * 13, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 + 5);
        tft.print("disconnected");
      }
    } else {
      /* wifi connection */
      tft.setTextColor(ILI9341_RED);
      tft.setTextSize(1);
      tft.fillRect(LEFT_MARGIN + 6 * 7, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 + 5, 6 * 12, 8, ILI9341_BLACK);
      tft.setCursor(LEFT_MARGIN + 6 * 7, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 + 5);
      tft.print("disconnected");
      /* mqtt connection */
      tft.setTextColor(ILI9341_RED);
      tft.setTextSize(1);
      tft.fillRect(LEFT_MARGIN + 160 + 6 * 13, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 + 5, 6 * 12, 8, ILI9341_BLACK);
      tft.setCursor(LEFT_MARGIN + 160 + 6 * 13, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 + 5);
      tft.print("disconnected");
      /* ip address */
      tft.setTextColor(ILI9341_WHITE);
      tft.setTextSize(1);
      tft.fillRect(LEFT_MARGIN + 6 * 12, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 + 5 + 8, 6 * 20, 8, ILI9341_BLACK);
      tft.setCursor(LEFT_MARGIN + 6 * 12, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 + 5 + 8);
      tft.print("not available");
    }
  }

}
