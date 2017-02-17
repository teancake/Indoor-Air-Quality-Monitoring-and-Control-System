# Indoor Air Quality Monitoring and Control System
## Control Unit Software

The control unit software lies in the ESP8266 chip of the ESP-12F WiFi module. The program utilises an MQTT client library and waits for incoming control messages from the MQTT broker and manipulates the GPIO pin connected to the relay accordingly.

The program can be uploaded to the ESP8266 chip via the Arduino IDE + [ESP8266 Arduino Core] (https://github.com/esp8266/Arduino "ESP8266 Arduino Core"), or via [PlatformIO] (http://platformio.org/ "PlatformIO") commands.
