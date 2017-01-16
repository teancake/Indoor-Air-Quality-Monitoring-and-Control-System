#!/usr/bin/env python
# Indoor Air Quality Controller - Bang-Bang Control
# Xiaoke Yang (das.xiaoke@hotmail.com) 
# IFFPC, Beihang University
# Last Modified: Tue 20 Dec 2016 16:39:28 CST

import time
import paho.mqtt.client as mqtt
import numpy as np

## Global variables
# mqtt information
MQTT_TOPIC_PM25 = "office/sensor/airquality1/pm25"
MQTT_TOPIC_PM10 = "office/sensor/airquality1/pm10"
MQTT_TOPIC_CONTROL = "office/switch/airpurifier"
MQTT_USER_NAME = "your_own_name"
MQTT_USER_PASSWORD = "your_own_password"
MQTT_HOST_ADDR = "your_own_ip_addr"
MQTT_HOST_PORT = 1883
MQTT_CLIENT_ID = "controller"
auth_info = {'username':MQTT_USER_NAME, 'password':MQTT_USER_PASSWORD}

list_pm10 = [];
list_pm25 = [];

# filter and controller information
TS_S = 10               # sampling period
NH = 10                 # FIR filter horizon length
BBC_LEVEL_ON = 35       # bang-bang controller on level (PMs)
BBC_LEVEL_OFF = 25      # bang-bang controller on level (PMs)

# output filtering
def output_filtering(list1, list2):
    pm25 = 0
    pm10 = 0
    if len(list1) > 0:
        pm25 = np.mean(list1)
    if len(list2) > 0:
        pm10 = np.mean(list2) 
    return pm25, pm10

# A bang-band controller (i.e. a hysteresis controller)
def compute_control_action(pm25, pm10, u_current, off_level, on_level):
    pm = (pm25+pm10)/2.0
    if (u_current == 0):    # lower
        if (pm > on_level):
            u = 1
        else:
            u = 0
    else:                   # upper
        if (pm < off_level):
            u = 0
        else:
            u = 1
    return u

# MQTT subscription callback function
def on_message(client, userdata, message):
        global NH, list_pm10, list_pm25
        if message.topic == MQTT_TOPIC_PM25:
            tmp = float(message.payload)
            list_pm25.append(tmp)
            if len(list_pm25) > NH:
                list_pm25.pop(0)
#            print(list_pm25)
        elif message.topic == MQTT_TOPIC_PM10:
            tmp = float(message.payload)
            list_pm10.append(tmp)
            if len(list_pm10) > NH:
                list_pm10.pop(0)
#            print(list_pm10)

## the main program
# mqtt client
mqttc = mqtt.Client(client_id=MQTT_CLIENT_ID)
mqttc.username_pw_set(MQTT_USER_NAME, MQTT_USER_PASSWORD)
mqttc.connect(MQTT_HOST_ADDR, MQTT_HOST_PORT)
mqttc.subscribe([(MQTT_TOPIC_PM25,0), (MQTT_TOPIC_PM10,0)])
mqttc.on_message = on_message
mqttc.loop_start()
# initialise control output
uPM = 0
while True:
    # output filtering
    pm25, pm10 = output_filtering(list_pm25, list_pm10)
#    print(pm25)
#    print(pm10)
    # compute control actions
    uPM = compute_control_action(pm25, pm10, uPM, BBC_LEVEL_OFF, BBC_LEVEL_ON)
    u_prev = uPM
    # get the current hour
    d = time.strftime("%H")
    d = int(d);
    # if the current time is NOT beteen 8:00 and 22:00, switch is OFF.
    if d <= 7 or d >= 23:
        uPM = 0
    # publish MQTT messages of control actions
    mqttc.publish(MQTT_TOPIC_CONTROL, str(uPM)) 
#        print("Data published.")
    time.sleep(TS_S)
mqttc.loop_stop()
