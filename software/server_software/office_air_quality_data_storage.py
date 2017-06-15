#!/usr/bin/env python
# save office air quality data into a mysql database 
#
# Xiaoke Yang (das.xiaoke@hotmail.com) 
# IFFPC, Beihang University
# Last Modified: Mon 16 Jan 2017 16:26:49 CST

import pymysql
import paho.mqtt.client as mqtt
from datetime import date
import time

# mysql database information
MYSQL_HOST_ADDR = "127.0.0.1"
MYSQL_USER = "mysql_user_name"
MYSQL_PASSWORD = "mysql_user_password"
MYSQL_DB = "officenv"
MYSQL_PORT = 3306

# MQTT Topics
MQTT_TOPIC_PM25 = "office/sensor/airquality1/pm25"
MQTT_TOPIC_PM10 = "office/sensor/airquality1/pm10"
MQTT_TOPIC_TEMPERATURE = "office/sensor/airquality1/temperature"
MQTT_TOPIC_HUMIDITY = "office/sensor/airquality1/humidity"
MQTT_TOPIC_OUTSIDE_PM25 = "internet/sensor/beijing/airquality/pm25"
MQTT_TOPIC_AIR_PURIFIER = "office/switch/airpurifier"
# mqtt broker
MQTT_USER_NAME = "mqtt_user_name"
MQTT_USER_PASSWORD = "mqtt_user_password"
MQTT_HOST_ADDR = "127.0.0.1"
MQTT_HOST_PORT = 1883
MQTT_CLIENT_ID = "office_aqc_data_storage"
auth_info = {'username':MQTT_USER_NAME, 'password':MQTT_USER_PASSWORD}

# device ID
DEV_ID = 1
# global variables
PM25 = 0
PM10 = 0
Airpurifier = 0
Outside_PM25 = 0
Temperature = 0
Humidity = 0
# sampling period
TS_S = 20
# previous time of heartbeat
t_hb = 0
# time interval to determine the loss-of-connection of the client
T_HB_S = 60
flag_online = 0
# message callback
def on_message(client, userdata, message):
        global PM25, PM10, Airpurifier, Outside_PM25, Temperature, Humidity, t_hb
        if message.topic != MQTT_TOPIC_OUTSIDE_PM25:
            t_hb = time.time()
        if message.topic == MQTT_TOPIC_PM25:
            PM25 = float(message.payload)
        elif message.topic == MQTT_TOPIC_PM10:
            PM10 = float(message.payload)
        elif message.topic == MQTT_TOPIC_AIR_PURIFIER:
            Airpurifier = float(message.payload)
        elif message.topic == MQTT_TOPIC_OUTSIDE_PM25:
            Outside_PM25 = float(message.payload)
        elif message.topic == MQTT_TOPIC_TEMPERATURE:
            Temperature = float(message.payload)
        elif message.topic == MQTT_TOPIC_HUMIDITY:
            Humidity = float(message.payload)

# create an mqtt client
mqttc = mqtt.Client(client_id=MQTT_CLIENT_ID)
mqttc.username_pw_set(MQTT_USER_NAME, MQTT_USER_PASSWORD)
# connect to a broker
mqttc.connect(MQTT_HOST_ADDR, MQTT_HOST_PORT)
# subscribe topics
mqttc.subscribe([(MQTT_TOPIC_PM25,0), (MQTT_TOPIC_PM10,0), (MQTT_TOPIC_AIR_PURIFIER,0), (MQTT_TOPIC_OUTSIDE_PM25,0), (MQTT_TOPIC_TEMPERATURE,0), (MQTT_TOPIC_HUMIDITY,0)])
# define new message callback
mqttc.on_message = on_message
mqttc.loop_start()

# database connection
conn = pymysql.connect(host="localhost", port=MYSQL_PORT, user=MYSQL_USER, passwd=MYSQL_PASSWORD, db=MYSQL_DB)
# define a database cursor
cur = conn.cursor()

t_hb = 0

while True:
    time.sleep(TS_S)
    try:
        # reconnect to MySQL if connection is broken
        if not conn.open:
            conn = pymysql.connect(host="localhost", port=MYSQL_PORT, user=MYSQL_USER, passwd=MYSQL_PASSWORD, db=MYSQL_DB)
            cur = conn.cursor()
        # resubscribe MQTT topics in case connection is broken
        mqttc.subscribe([(MQTT_TOPIC_PM25,0), (MQTT_TOPIC_PM10,0), (MQTT_TOPIC_AIR_PURIFIER,0), (MQTT_TOPIC_OUTSIDE_PM25,0), (MQTT_TOPIC_TEMPERATURE,0), (MQTT_TOPIC_HUMIDITY,0)])
        #print(time.time() - t_hb)
        flag_online = (time.time() - t_hb) < T_HB_S
        # update status
        if flag_online:
            str_query = "UPDATE devices SET status=1 WHERE devID=%s" % (DEV_ID)
        else:
            str_query = "UPDATE devices SET status=0 WHERE devID=%s" % (DEV_ID)
        # print(str_query)
        try:
            cur.execute(str_query)
            conn.commit()
        except Exception as e:
            conn.rollback()
            # print("Skipping the current loop.")
            continue
        if flag_online:
            # date and time
            d = date.today()
            d = d.strftime("%d-%m-%Y")
            t = time.strftime("%H:%M:%S")
            # SQL query string
            str_query = "INSERT INTO data (devID,date,time,attr1,attr2,attr3,attr4,attr5,attr6) VALUES (%s, \"%s\", \"%s\", %s, %s, %s, %s, %s, %s)" % (DEV_ID, d, t, PM25, PM10, Airpurifier, Outside_PM25, Temperature, Humidity) 
            # print(str_query)
            try:
                # execute the SQL command
                cur.execute(str_query)
                # commit the changes in the database, i.e. to save the changes.
                conn.commit()
            except Exception as e:
                conn.rollback()
                # print("Skipping the current loop.")
                continue

    except KeyboardInterrupt:
        break
    except Exception as e:
        print(e)
# clean up on exit
mqttc.loop_stop()
# close the cursor
cur.close()
# close the database connections
conn.close()
#print ("DB Connection closed.")
