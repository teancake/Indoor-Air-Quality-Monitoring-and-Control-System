#!/bin/bash
su
cp mirrorlist /etc/padman.d/ # use USTC China source
pacman -Sy
pacman -S vim sudo
vim /etc/sudoers # uncomment %wheel ...
exit

# install necessary software
sudo pacman -S mosquitto
sudo pacman -S python python-setuptools python-pip
sudo pip install paho-mqtt
sudo timedatectl set-timezone Asia/Shanghai
sudo useradd -r -s /bin/false mosquitto
sudo cp mosquitto.conf /etc/mosquitto/
sudo mosquitto_passwd -c /etc/mosquitto/pwfile yourusername

sudo systemctl enable mosquitto
sudo systemctl start mosquitto

mkdir ~/bin
cp path/to/office_air_quality_controller.py ~/bin

sudo cp path/to/office-aq-controller.service /lib/systemd/system/
sudo systemctl enable office-aq-controller
sudo systemctl start office-aq-controller
