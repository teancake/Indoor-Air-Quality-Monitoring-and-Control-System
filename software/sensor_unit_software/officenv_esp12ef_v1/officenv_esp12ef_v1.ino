/* Sensor unit software for the ESP-12E/F module in the office environment monitoring
    and control system. The program should be programmed into the ESP8266 chip.
    Xiaoke Yang (das.xiaoke@hotmail.com)
    IFFPC, Beihang University
    Last Modified: Mon 16 Jan 2017 17:06:39 CST
*/


#include <ESP8266WiFi.h>
#include <PubSubClient.h>

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "your_wifi_ssid"
#define WLAN_PASS       "your_wifi_password"

/************************* MQTT Server Setup *********************************/

#define AIO_SERVER      "192.168.xx.xxx"
#define AIO_SERVERPORT  1883
#define AIO_CID         "air quality sensor"
#define AIO_USERNAME    "mqtt_username"
#define AIO_KEY         "mqtt_password"

/* MQTT Topics */
#define MQTT_TOPIC_PM25 "office/sensor/airquality1/pm25"
#define MQTT_TOPIC_PM10 "office/sensor/airquality1/pm10"
#define MQTT_TOPIC_TEMPERATURE "office/sensor/airquality1/temperature"
#define MQTT_TOPIC_HUMIDITY "office/sensor/airquality1/humidity"

WiFiClient espClient;
PubSubClient client(espClient);
int wifi_connected = 0;
int mqtt_connected = 0;
int scale;
float pm25f, pm10f, temperaturef, humidityf;
int pm25, pm10, temperature, humidity;
char buf[10];
IPAddress ip_addr;



void setup() {
  /* start serial */
  Serial.begin(19200);
  /* start wifi */
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  /* delay 10 seconds */
  delay(10000);
  /* set up MQTT server */
  client.setServer(AIO_SERVER, AIO_SERVERPORT);
}


void loop() {
  wifi_connected = check_wifi();
  if (wifi_connected) {
    ip_addr = WiFi.localIP();
    mqtt_connected = mqtt_reconnect();
  } else {
    mqtt_connected = 0;
  }
  // if there's any serial available, read it:
  if (Serial.available() > 0) {
    /* read the scale */
    scale = Serial.parseInt();
    /* read the comma */
    Serial.read();
    /* read pm2.5 value */
    pm25 = Serial.parseInt();
    /* read the comma */
    Serial.read();
    /* read pm10 value */
    pm10 = Serial.parseInt();
    /* read the comma */
    Serial.read();
    /* read the temperature */
    temperature = Serial.parseInt();
    /* read the comma */
    Serial.read();
    /* read the humidity */
    humidity = Serial.parseInt();
    /* read the new line */
    Serial.read();
    /* convert all integer values to float ones */
    pm25f = pm25;
    pm10f = pm10;
    temperaturef = temperature;
    humidityf = humidity;
    // sprintf(buf, "%d", scale);
    // Serial.println(buf);
    /* publish all topics */
    if (mqtt_connected) {
      dtostrf(pm25f / scale, 4, 2, buf);
      client.publish(MQTT_TOPIC_PM25, buf);
      // Serial.println(buf);
      dtostrf(pm10f / scale, 4, 2, buf);
      client.publish(MQTT_TOPIC_PM10, buf);
      // Serial.println(buf);
      dtostrf(temperaturef / scale, 4, 2, buf);
      client.publish(MQTT_TOPIC_TEMPERATURE, buf);
      // Serial.println(buf);
      dtostrf(humidityf / scale, 4, 2, buf);
      client.publish(MQTT_TOPIC_HUMIDITY, buf);
      // Serial.println(buf);
    }
    /* now print connection information, format: wifi, mqtt, ip addr */
    Serial.print(wifi_connected);
    Serial.print(',');
    Serial.print(mqtt_connected);
    Serial.print(',');
    Serial.println(ip_addr);
  }
}



int check_wifi() {
  if (WiFi.status() != WL_CONNECTED) {
    return 0;
  } else {
    return 1;
  }
}

/* check mqtt connection status and reconnect if disconnected */
int mqtt_reconnect() {
  if (client.connected()) {
    return 1;
  }
  if (client.connect(AIO_CID, AIO_USERNAME, AIO_KEY )) {
    return 1;
  } else {
    return 0;
  }
}

