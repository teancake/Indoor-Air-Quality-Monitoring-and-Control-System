/* Control unit software for the office environment monitoring and control
 * system. The program should be programmed into the ESP8266 chip. 
 * Xiaoke Yang (das.xiaoke@hotmail.com) 
 * IFFPC, Beihang University
 * Last Modified: Mon 16 Jan 2017 17:06:39 CST
 */ 


#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define RELAY_PIN 4

/************************* WiFi Access Point *********************************/

#define WLAN_SSID       "wifissid"
#define WLAN_PASS       "wifipassword"

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "192.168.xx.xxx"
#define AIO_SERVERPORT  1883
#define AIO_CID         "airpurifier_switch"
#define AIO_USERNAME    "mqtt_username"
#define AIO_KEY         "mqtt_password"

#define MQTT_TOPIC_CONTROL "office/switch/airpurifier"

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;


void setup_wifi() {
  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  if ((char)payload[0] == '0') {
    digitalWrite(RELAY_PIN, LOW); 
  } else {
    digitalWrite(RELAY_PIN, HIGH);  
  }

}

void reconnect() {
  while (!client.connected()) {
    if (client.connect(AIO_CID, AIO_USERNAME, AIO_KEY )) {
      // subscribe control actions topic
      client.subscribe(MQTT_TOPIC_CONTROL);
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
  pinMode(RELAY_PIN, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  setup_wifi();
  client.setServer(AIO_SERVER, AIO_SERVERPORT);
  client.setCallback(callback);
}


void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
