#!/usr/bin/env python
# Indoor Air Quality Controller - Bang-Bang Control
# Xiaoke Yang (das.xiaoke@hotmail.com) 
# IFFPC, Beihang University
# Last Modified: Tue 20 Dec 2016 16:39:28 CST

import time
import paho.mqtt.client as mqtt

## Global variables
# mqtt information
MQTT_TOPIC_PM25 = "office/sensor/airquality1/pm25"
MQTT_TOPIC_PM10 = "office/sensor/airquality1/pm10"
MQTT_TOPIC_CONTROL = "office/switch/airpurifier"
MQTT_USER_NAME = "your_own_name"
MQTT_USER_PASSWORD = "your_own_password"
MQTT_HOST_ADDR = "127.0.0.1"
MQTT_HOST_PORT = 1883
MQTT_CLIENT_ID = "controller"
auth_info = {'username':MQTT_USER_NAME, 'password':MQTT_USER_PASSWORD}

list_pm10 = [];
list_pm25 = [];

# filter and controller information
TS_S = 10               # sampling period
NH = 10                 # FIR filter horizon length
BBC_LEVEL_ON = 50       # bang-bang controller on level (PMs)
BBC_LEVEL_OFF = 40      # bang-bang controller on level (PMs)

# output filtering
def output_filtering(list1, list2):
    pm25 = -1
    pm10 = -1
    if len(list1) > 0:
        pm25 = sum(list1)/len(list1)
    if len(list2) > 0:
        pm10 = sum(list2)/len(list2)
    return pm25, pm10

# An on-off controller (i.e. a hysteresis controller)
def compute_control_action(pm25, pm10, u_current, off_level, on_level):
    # we only regulate the pm2.5 concentration
    pm = pm25
    # the default control action is -1, which is an invalid value
    u = -1
    if (pm >= 0):
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
            # print(list_pm25)
        elif message.topic == MQTT_TOPIC_PM10:
            tmp = float(message.payload)
            list_pm10.append(tmp)
            if len(list_pm10) > NH:
                list_pm10.pop(0)
            # print(list_pm10)
def on_publish(client, userdata, mid):
    # print(client)
    # print(userdata)
    # print(mid)
    print("Data published.")
## the main program
# mqtt client
mqttc = mqtt.Client(client_id=MQTT_CLIENT_ID)
mqttc.username_pw_set(MQTT_USER_NAME, MQTT_USER_PASSWORD)
mqttc.connect(MQTT_HOST_ADDR, MQTT_HOST_PORT)
mqttc.subscribe([(MQTT_TOPIC_PM25,0), (MQTT_TOPIC_PM10,0)])
mqttc.on_message = on_message
mqttc.on_publish = on_publish
mqttc.loop_start()
# initialise control action
uPM = -1
while True:
    # subscribe to avoid unexpected mosquitto server stop/restart 
    mqttc.subscribe([(MQTT_TOPIC_PM25,0), (MQTT_TOPIC_PM10,0)])
    # output filtering
    pm25, pm10 = output_filtering(list_pm25, list_pm10)
    # print(pm25)
    # print(pm10)
    # compute control actions
    uPM = compute_control_action(pm25, pm10, uPM, BBC_LEVEL_OFF, BBC_LEVEL_ON)
    # get the current hour
    d = time.strftime("%H")
    d = int(d);
    # if the current time is NOT beteen 8:00 and 22:00, switch is OFF.
    if d <= 7 or d >= 23:
        uPM = 0
    # print(uPM)
    # only publish valid control actions
    if (uPM == 0) or (uPM ==1): 
        try:
            # publish MQTT messages of control actions
            mqttc.publish(MQTT_TOPIC_CONTROL, str(uPM)) 
        except Exception as e:
            print(e)
    time.sleep(TS_S)
mqttc.loop_stop()
