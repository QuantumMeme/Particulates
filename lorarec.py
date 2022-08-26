import serial, time, json, threading, atexit
from serial import SerialException
from datetime import datetime
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

import RPi.GPIO as GPIO

#proper cloud
token = "nR0uF_IUcnwiYMBGyqTEzbSM4_kKOo6twv8KZNt_T8bnGZ5SSsiQLS3nKz8o48g36gRbTC4cCMKyQQEmqbKE8w=="
org = "david.pesin@gmail.com"
bucket = "asbestos"

#local
#token = "cRrWg4e7btFQdUyk3Su4-vAYDlCXtzt9cPUp_pKB1v4ahwt1xgspXKQ-H_mA0mgpvDC4AVKe0SP9WUrnoNpclg=="
#org = "ORNL"
#bucket = "data"

SLEEPTIME = 0.7
MAX_SIZE= 184

CONN_ERR  = 0 # for diagnostics
PARSE_ERR = 0
CLEAN_ERR = 0
SMALL_ERR = 0
SUCCEEDED = 0
TOTAL     = 0

#atexit 
def closing(cli):
    cli.close()
    print("---------------------------------------------------")
    print("Total received: ", TOTAL)
    print("Successful send: ", SUCCEEDED)
    print("Failed sending due to:")
    print("\tConnection issues: (InfluxDB, WiFi)", CONN_ERR)
    print("\tCleaning issues (UnicodeError): ", CLEAN_ERR)
    print("\tParsing issues (KeyError): ", PARSE_ERR)
    print("\tNot enough data received (not a software error): ", SMALL_ERR)
    try:
        print("Success rate: {}%".format(round(float(SUCCEEDED/TOTAL*100), 2)))
    except ZeroDivisionError:
        print("Success rate: 0%")

# Sometimes the messages come through with a header of bytes that can't be used.
# This checks the byte value of each character in the message to make sure it's a readable character.
def clean_message(message):
    global CLEAN_ERR
    flag = 0
    clean_msg = message
    for i in range(len(message)):
        if message[i] > 128 or message[i] == 0:
            if i == 0:
                clean_msg = message[(i+1):]
            else:
                clean_msg = clean_msg[:(i-1)] + clean_msg[(i+1):]
    try:
        return clean_msg.decode("utf-8")

    except UnicodeError:
        print ("UnicodeError in func clean_message")
        CLEAN_ERR += 1


def extractInflux(datalist):
    #largest list that can be produced by a normal packet is 28 (27)

    extracted = []
    cap = len(datalist)
    sensorID = "0x" + datalist[0]

    cnt = 1
    while cnt < cap:

        if datalist[cnt].find("PC0.1") > -1:
            datalist[cnt] = "PC0.1"
            extracted = []

        try:
            float(datalist[cnt+1])
        except ValueError: # Just a failsafe. Move to the next item in the list if its valid, otherwise skip it.
            if cnt + 1 <= cap:
                if datalist[cnt+1].find("PC0.1") > -1: # Whenever "PC0.1" is detected, generally it means that a new measurement overwrote the initial one.
                    datalist[cnt+1] = "PC0.1"
                    cnt += 1
            
                elif datalist[cnt+1].find("PC") > -1 or datalist[cnt+1].find("PM") > -1:
                    cnt += 1
                else:
                    cnt += 2
        except IndexError: #lol 
            break
        else:
            extracted.append("particulate,sensor={} {}={}".format(sensorID, datalist[cnt],float(datalist[cnt+1])))
            cnt += 2
            
    return extracted

def main():
    global SUCCEEDED, CONN_ERR, PARSE_ERR, CLEAN_ERR, SMALL_ERR, TOTAL #debugging things

    GPIO.setwarnings(False)
    GPIO.setmode(GPIO.BCM)
    GPIO.setup(26, GPIO.OUT)

    client = InfluxDBClient(url="https://us-east-1-1.aws.cloud2.influxdata.com", token=token, org=org)
    #client = InfluxDBClient(url="http://localhost:8086", token=token, org=org)
    write_api = client.write_api(write_options=SYNCHRONOUS)

    atexit.register(closing, client)

    # Loop to keep trying to open serial until it works.
    print("opening serial...")
    while True:
        try:
            ser = serial.Serial('/dev/ttyS0', 115200, timeout = 0)
        except SerialException: 
            print("Could not connect! Trying again.")
            time.sleep(0.5)
        else:
            break
        print("opened!")

    ser.flush()

    while True:
        GPIO.output(26, GPIO.LOW)
    #Checking if enough data is sent
        msgavailable = 0
        final_msg = {}

        while True:
            if ser.in_waiting > MAX_SIZE:
                raw_msg = ser.readline()
                break
            else:
                SMALL_ERR += 1
        print("\nraw message\n{}\n".format(raw_msg))
        
        if len (raw_msg) < MAX_SIZE:
            msgavailable = -1
        else:
            try:
                clean_msg = clean_message(raw_msg)
            except UnicodeError:
                msgavailable = 1
            if not clean_msg:
                msgavailable = -1
        

        #checking if enough _usable_ info is sent
        if msgavailable == 0: 
            print("clean\n{}".format(clean_msg))

            parti_msg = clean_msg.split(",") # I love python

            # sometimes the first label comes out as "PPC0.1" instead of "PC0.1," so this makes things easier.
            print("\npartitioned msg\n{}\n".format(parti_msg))
            parti_msg[0] = "PC0.1"

            #turn split list into dict
            final_msg = extractInflux(parti_msg)
        
            print("final message\n{}\n-------".format(final_msg))
            

            # There is a MUCH better way to do this, i just messed the logic up for now
            print("sending...")
            try:
                write_api.write(bucket, org, final_msg)
            except Exception as e:
                print("error sending")
                print(e)
                CONN_ERR += 1
                TOTAL += 1
            else:
                GPIO.output(26, GPIO.HIGH)
                SUCCEEDED += 1
                TOTAL += 1
                print("sent!")
            time.sleep(SLEEPTIME)
        elif msgavailable == 1:
            print("-----------------\nsome undecodable bytes were sent")
            print("-----------------\n")
            TOTAL+=1
            CLEAN_ERR+=1


        ser.flush()
        #print(final_msg)
        
if __name__ == "__main__":
    main()