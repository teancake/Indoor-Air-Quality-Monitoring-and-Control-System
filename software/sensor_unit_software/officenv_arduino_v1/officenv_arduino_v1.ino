/* Sensor unit software for the office environment monitoring and control
 * system.  
 * Xiaoke Yang (das.xiaoke@hotmail.com) 
 * IFFPC, Beihang University
 * Last Modified: Mon 16 Jan 2017 16:50:19 CST
 */ 
#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "DHT.h"
#include "SoftwareSerial.h"
#include "ESP8266xy.h"



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

float pm2, pm10;
float h, t, hic;


/* wifi module works at STA MODE, i.e. as a client */
/* SSID of the target network */
#define SSID "networkssid"
/* password */
#define PASSWORD "networkpassword"
/* the address of the host to be connected */
#define HOST_ADDR  "192.168.xx.200"
/* the port for the TCP connection on the host */
#define HOST_TCP_PORT  "9000"
/* device ID*/
#define devID 1

/* sampling period in micro-seconds */
#define TS_MS 20000
#define TS_WIFI_RECONN 600000
#define TS_DHT_MS 5000


/* This is a scaling factor for the data */
#define SF 10


WIFI wifi;
char wifi_connected = 0;
long millis_prev = 0;
long millis_dht_prev = 0;
long millis_wifi_prev = 0;
void setup() {
  /* set up the tft screen */
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.drawFastHLine(0, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 , 320 , ILI9341_WHITE);
  tft.setCursor(LEFT_MARGIN, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 5 + 5);
  tft.setTextSize(1);
  tft.println("Recommended value: axx lckvjlskfsoudladkfja");

  /* start the dht sensor */
  dht.begin();
  /* start the software serial connecting the PM sensor */
  pmSerial.begin(9600);
  wifi_init();
}


void loop(void) {
  /* read the pm sensor at each loop to empty the input buffer of the serial */
  /* do not put getPMs() inside the interval check */
  getPMs();
  if (millis() - millis_dht_prev > TS_DHT_MS || millis() - millis_dht_prev < 0 ) {
    getDHT();
    millis_dht_prev = millis();
    display_env_params(t, hic, h, pm2, pm10);
  }
  if (wifi_connected) {
    /* if given sampling period is reached, or millis() function overflows */
    if (millis() - millis_prev > TS_MS || millis() - millis_prev < 0 ) {
      send_data_via_wifi(t, h, pm2, pm10);
      millis_prev = millis();
    }
  } else {
    /* try to connect every 1 min */
    if (millis() - millis_wifi_prev > TS_WIFI_RECONN || millis() - millis_wifi_prev < 0 ) {
      wifi_init();
    }
  }
}

/**
   initialise the wifi module
 **
*/
void wifi_init() {
  /* start the wifi module */
  wifi.begin();
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(LEFT_MARGIN, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 6 + 5);
  tft.println("Connecting to wifi...");
  /* connect the wifi module to the wireless network */
  bool b = wifi.Initialize(STA, SSID, PASSWORD);
  /* if failed*/
  if (!b) {
    /* display some information on the tft screen */
    tft.setCursor(LEFT_MARGIN, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 6 + 5);
    tft.fillRect(0, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 6 + 5, 320, PARAM_HEIGHT, ILI9341_BLACK);
    tft.println("Unable to connect to WiFi.");
    /* set the wifi_connected flag to be false*/
    wifi_connected = 0;
  } else {
    /* set the wifi connected module to be true */
    wifi_connected = 1;
    wifi_show_connection_info();
  }
}

void wifi_show_connection_info() {
  /* display the ip address on the screen */
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(LEFT_MARGIN, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 6 + 5);
  tft.fillRect(0, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 6 + 5, 320, PARAM_HEIGHT, ILI9341_BLACK);
  String temp = wifi.showJAP();
  int a = temp.indexOf("busy");
  tft.print("Connected to ");
  tft.print(temp);
  tft.print(a);
  tft.setCursor(LEFT_MARGIN, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 6 + 13);
  temp = wifi.showIP();
  tft.print("IP address is ");
  tft.println(temp);
  /* configure the wifi module as single connection mode */
}
/**
   get the PM sensor's readings
 **
*/
void getPMs() {
  char buf[10];
  if (pmSerial.available()) {
    byte c = pmSerial.read();
    if (c == B10101010) {
      c = pmSerial.read();
      if (c == B11000000) {
        // read data
        pmSerial.readBytes(buf, 8);
        byte hbyte, lbyte;
        hbyte = buf[1];
        lbyte = buf[0];
        int tmp1 = ((hbyte << 8) + lbyte);
        if (tmp1 > 0 && tmp1 < 10000) {
          pm2 = tmp1 / 10.0;
        }
        // PM10
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

  

void getDHT() {
  /* Reading temperature or humidity takes about 250 milliseconds! */
  /* Sensor readings may also be up to 2 seconds 'old' */
  float tmp_h = dht.readHumidity();
  /* Read temperature as Celsius (the default) */
  float tmp_t = dht.readTemperature();
  /* Check if any reads failed and exit early (to try again). */
  if (isnan(tmp_h) || isnan(tmp_t)) {
  }else{
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

/**
   Send the measurement data via the wifi connection to the host
 **
*/
void send_data_via_wifi(float T, float H, float PM2, float PM10) {
  /* data buffer */
  char wifi_buf[50];
  /* print information on the LCD display */
  tft.setCursor(LEFT_MARGIN, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 7 + 24);
  tft.fillRect(LEFT_MARGIN, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 7 + 24, 320, PARAM_HEIGHT, ILI9341_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.print("Establishing connections to server...");
  /* connect to the TCP server */
  if (!wifi.ipConfig(TCP, HOST_ADDR, HOST_TCP_PORT)) {
    /* if failed */
    tft.setCursor(LEFT_MARGIN, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 7 + 24);
    tft.fillRect(0, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 7 + 24, 320, PARAM_HEIGHT, ILI9341_BLACK);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_RED);
    tft.print("Connection to server failed.");
    tft.setTextColor(ILI9341_WHITE);
  } else {
    tft.setCursor(LEFT_MARGIN, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 7 + 24);
    tft.setTextSize(1);
    tft.setTextColor(ILI9341_WHITE);
    tft.fillRect(0, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 7 + 24, 320, PARAM_HEIGHT, ILI9341_BLACK);
    tft.print("Connection estabilished. Sending data...");
    sprintf(wifi_buf, "%d,%d,%d,%d,%d,%d,%d", devID, 1, SF, (int)(T * SF), (int)(H * SF), (int)(PM2 * SF), (int)(PM10 * SF));
    tft.setCursor(LEFT_MARGIN, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 7 + 24);
    tft.setTextSize(1);
    /* send the measurement data */

    if (wifi.Send(wifi_buf)) {

      tft.setTextColor(ILI9341_GREEN);
      tft.fillRect(0, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 7 + 24, 320, PARAM_HEIGHT, ILI9341_BLACK);
      tft.print("Data successfully sent.");
      tft.setTextColor(ILI9341_WHITE);
    } else {
      tft.setTextColor(ILI9341_RED);
      tft.fillRect(0, TOP_MARGIN + (PARAM_HEIGHT + LINE_SPACE) * 7 + 24, 320, PARAM_HEIGHT, ILI9341_BLACK);
      tft.print("Sending data failed.");
      tft.setTextColor(ILI9341_WHITE);
    }
  }
  /* close the TCP connection */
  wifi.closeMux();
}
