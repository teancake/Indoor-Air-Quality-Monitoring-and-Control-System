# Indoor Air Quality Monitoring and Control System
## Sensor Unit Software
There are two ways of programming the sensor unit.
1. An Arduino program + the default firmware of the ESP module.
2. An Arduino program + An ESP program.

The folder `officenv_arduino_v1` corresponds to the first appraoch. It is an Arduino program, which can be complied and uploaded to the AVR microprocessor on the Arduino board via the Arduino IDE. The program manages connections with all the peripherals, including the sensors, the WiFi module, and the display. The values of the DHT22 humidity sensor and SDS011 PM sensor are sampled at a fixed interval and displayed on the LCD screen. Data transmission over the WiFi operates at another interval, which is much longer than the display refresh interval. The management of the WiFi connection is accomplished through AT commands.

Programs in folders `officenv_arduino_v2` and `officenv_esp12ef_v1` correspond to the second approach. The EPS8266 chip can be programmed either via the [ESP8266 Arduino extension](https://github.com/esp8266/Arduino), or by the [PlatformIO tools](http://platformio.org/).

