# Indoor Air Quality Monitoring and Control System
## Server Software
The server software listed in this directory runs in an [Arch Linux ARM] (https://archlinuxarm.org/ "Arch Linux ARM") operating system on the Raspberry Pi 2B minicomputer. These python scripts were tested under Python 3.5.0, and please report to me if there are issues with other versions of Python. 


The server software includes a sensor data acquisition program, an outside air quality publishing program, a data storage program, and a controller program, and is built around an MQ Telemetry Transport (MQTT) broker. 
### Installation of Arch Linux ARM on Raspberry Pi 2B
To install Arch Linux ARM on the Raspberry Pi board, follow the instructions here at https://archlinuxarm.org/platforms/armv7/broadcom/raspberry-pi-2. After installation, copy the folder `rpiconfig` into the home directory on the mircoSD card, power up the Pi board, ssh to it, and execute the commands in setup.sh.

### The Indoor Sensor Data Acquisition Program 
The sensor data acquisition program `publish_office_air_quality.py` establishes a TCP server, which accepts connections from the sensor unit software. The program then interprets the incoming sensor measurements and publishes measurement messages. *This program is no longer used, since with the updated sensor software, the sensor can connect directly to an MQTT server, and publish data without a dedicated TCP server.*

### The Outside Air Quality Publishing Program
The outside air quality publishing program `publish_beijing_air_quality.py` polls the Beijing air quality from various sources and publish the data obtained in MQTT messages. This program has its own repository at [Beijing PM Values](https://github.com/teancake/Beijing-PM-Values "Beijing PM Values").

### The Data Storage Program
The data storage program `office_air_quality_data_storage.py` subscribes all the published MQTT messages and stores them in a database. Note this data storage program writes to the microSD card of the Raspberry Pi very frequently, in order to prevent the corruption of the SD card, use a high quality power adaptor for the Raspberry Pi and always carry out a proper power cycle. 

### The Controller Program 
The controller program `office_air_quality_controller.py` subscribes measurement messages from the MQTT broker. A measurement filtering block then applies a filtering algorithm to the measurement returned by the MQTT callback procedure. The control law in this program is an on-off controller. 

### The MQTT Broker
An MQTT broker is essential for the above sensor software. It manages the publications and subscriptions of the MQTT messages and distribute incoming messages to target clients. The mosquitto MQTT broker is recommended.  

## The Systemd Service Configuration Files 
The folder `systemd_services` contains the systemd service configuration files for the server programs, enabling the power-on-auto-start and restart-on-failure capabilities of the server programs. Theses files should be put in the `/usr/lib/systemd/system` directory of the Arch Linux ARM operating system and be configured by the `systemctl` command.
