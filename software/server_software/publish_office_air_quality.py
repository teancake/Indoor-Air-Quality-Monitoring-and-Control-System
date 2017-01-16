#!/usr/bin/env python
# Read and publish office air quality values
# This program is derived from the Multi-threaded TCP server program at
# https://github.com/edaniszewski/MultithreadedTCP
#
# Xiaoke Yang (das.xiaoke@hotmail.com) 
# IFFPC, Beihang University
# Last Modified: Tue 20 Dec 2016 16:49:54 CST

"""
Multi-threaded TCP Server

multithreadedServer.py acts as a standard TCP server, with multi-threaded integration, spawning a new thread for each client request it receives.

This is derived from an assignment for the Distributed Systems class at Bennington College
"""

from threading import Lock, Thread
import socket 
import time
import paho.mqtt.publish as publish


# -------------------------------------------#
########## DEFINE GLOBAL VARIABLES ##########
#-------------------------------------------#
HOST_ADDR = "your_tcp_host_addr"
HOST_PORT = 9000
response_message = "Now serving, number: "
thread_lock = Lock()


# mqtt information
MQTT_TOPIC_PM25 = "office/sensor/airquality1/pm25"
MQTT_TOPIC_PM10 = "office/sensor/airquality1/pm10"
MQTT_TOPIC_TEMPERATURE = "office/sensor/airquality1/temperature"
MQTT_TOPIC_HUMIDITY = "office/sensor/airquality1/humidity"
MQTT_USER_NAME = "your_own_user_name"
MQTT_USER_PASSWORD = "your_own_user_password"
MQTT_HOST_ADDR = "your_own_host_addr"
MQTT_HOST_PORT = 1883
MQTT_CLIENT_ID = "any"
auth_info = {'username':MQTT_USER_NAME, 'password':MQTT_USER_PASSWORD}

# Create a server TCP socket and allow address re-use
s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

while (HOST_ADDR != socket.gethostbyname(socket.gethostname())):
    time.sleep(5)

s.bind((HOST_ADDR, HOST_PORT))

#---------------------------------------------------------#
########## THREADED CLIENT HANDLER CONSTRUCTION ###########
#---------------------------------------------------------#


class ClientHandler(Thread):
    def __init__(self, address, port, socket, response_message, lock):
        Thread.__init__(self)
        self.address = address
        self.port = port
        self.socket = socket
        self.response_message = response_message
        self.lock = lock
        # Define the actions the thread will execute when called.
    def run(self):
        # set a timeout value for the socket
        self.socket.settimeout(5)
        while 1:
            try:
                # get data from socket
                data_recv = self.socket.recv(1024)
#                print len(data)
#                print data
                if len(data_recv) == 0:
#                    print "Data length is 0"
                    break;
#                print ">> Received data: ", data, " from: ", self.address
                # unpack the data
                dp = data_recv.decode("utf-8").split(',')
#                print(len(dp))
#                print(dp)
                if len(dp) >= 7:
                    devID = dp[0]
                    msgID = dp[1]
                    scale = float(dp[2])
                    T = float(dp[3])/scale
                    H = float(dp[4])/scale
                    pm25 = float(dp[5])/scale
                    pm10 = float(dp[6])/scale
                # publish MQTT messages
                msgs = [(MQTT_TOPIC_PM25, str(pm25), 0, False), {'topic':MQTT_TOPIC_PM10, 'payload': str(pm10)}, {'topic':MQTT_TOPIC_TEMPERATURE, 'payload': str(T)}, {'topic':MQTT_TOPIC_HUMIDITY, 'payload': str(H)}]
                publish.multiple(msgs, hostname=MQTT_HOST_ADDR, port=MQTT_HOST_PORT, client_id=MQTT_CLIENT_ID, auth=auth_info)
#                print("Data published.")
            # check socket errors
            except socket.error:
#                print e.args[0]
                break;
        # close the tcp socket
        self.socket.close()
#        print "TCP Connection closed."


#-----------------------------------------------#
########## MAIN-THREAD SERVER INSTANCE ##########
#-----------------------------------------------#

# Continuously listen for a client request and spawn a new thread to handle every request

t_prev = time.time();
while 1:
    try:
        # Listen for a request
        s.listen(1)
        # Accept the request
        sock, addr = s.accept()
#        print "Accept client: ", addr
        # Spawn a new thread for the given request
        newThread = ClientHandler(addr[0], addr[1], sock, response_message, thread_lock)
        newThread.start()
    except KeyboardInterrupt:
#        print "\nExiting Server\n"
        break


