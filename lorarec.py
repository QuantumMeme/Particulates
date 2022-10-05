import serial, time, json, threading, atexit
from serial import SerialException
from datetime import datetime
from influxdb_client import InfluxDBClient, Point, WritePrecision
from influxdb_client.client.write_api import SYNCHRONOUS

import RPi.GPIO as GPIO

#proper cloud
token = "vmLBmj4rlERpaYxjfy_j-UnYB7oRgIsBLk6A78QIy5VlEM3la3e4nelT9C_BW7i1Df2T-pU0UFJNmSAY33IszQ=="
org = "pesinda@ornl.gov"
bucket = "particulates"

#local
#token = "cRrWg4e7btFQdUyk3Su4-vAYDlCXtzt9cPUp_pKB1v4ahwt1xgspXKQ-H_mA0mgpvDC4AVKe0SP9WUrnoNpclg=="
#org = "ORNL"
#bucket = "data"

SLEEPTIME = 0.7
MAX_SIZE= 184

#atexit 
def closing(cli):
cli.close()

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


def extractInflux(datalist):
#largest list that can be produced by a normal packet is 28 (27)

extracted = []
cap = len(datalist)
sensorID = "sensor" + datalist[0]

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

def processData(rawmsg, ser, api):
    msgavailable = 0
    final_msg = {}

    print("\nraw message\n{}\n".format(rawmsg))

    if len (rawmsg) < MAX_SIZE:
        msgavailable = -1
    else:
        try:
            clean_msg = clean_message(rawmsg)
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
        parti_msg[1] = "PC0.1"

        #turn split list into dict
        final_msg = extractInflux(parti_msg)

        print("final message\n{}\n-------".format(final_msg))


        # There is a MUCH better way to do this, i just messed the logic up for now
        print("sending...")
        try:
            api.write(bucket, org, final_msg)
        except Exception as e:
            print("error sending")
            print(e)
        else:
            GPIO.output(26, GPIO.HIGH)
            print("sent!")
        time.sleep(SLEEPTIME)
    elif msgavailable == 1:
        print("-----------------\nsome undecodable bytes were sent")
        print("-----------------\n")


    ser.flush()

def main():
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

    while True:
        if ser.in_waiting > MAX_SIZE:
            raw_msg = ser.readline()
            process = threading.Thread(target=processData, args=(raw_msg, ser, write_api), daemon=True)
            process.start()

if __name__ == "__main__":
main()
