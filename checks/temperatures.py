# /usr/bin/python
#
#   Dieser Check sammelt alle Temperaturen die sich der ESP32 gerade gespeichert hat
#   Am ESP32 werden die Temperaturen alle 50s neu abgerufen, unabhängig von diesem Python Skript.
#
#   crontab -e
#
#   */2 * * * * /usr/bin/python /var/www/IOT-Heizung/checks/temperatures.py >> /var/www/IOT-Heizung/checks/logs/temperatures.log
#   
#
# Define class for log colours
class logcolour:
    HEADER = '\033[95m'
    OKBLUE = '\033[94m'
    OKCYAN = '\033[96m'
    OKGREEN = '\033[92m'
    WARNING = '\033[93m'
    FAIL = '\033[91m'
    ENDC = '\033[0m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'

import json
import requests
import os
from datetime import datetime
from pprint import pprint
# pip install mysql-connector-python
import mysql.connector
# Start program with date and time for Logs
now = datetime.now()
print(f"{logcolour.OKCYAN}[{now}]{logcolour.ENDC} Temperature Check Start.")
#print("[", now, "] Temperature Check Start")
# json with username and password
LOGIN_FILE = "/home/david/.confidential/heizung-rest-user.json"
username=""
password=""
try:
    # Open file to get login data
    fp = open(LOGIN_FILE, "r")
    credentials = json.load(fp)
    username = credentials["user"]
    password = credentials["password"]
    fp.close()
except FileNotFoundError:
    print("Failed to read " + LOGIN_FILE)
    exit()

# URL
url = "http://heizung/api/temperatures"
response = requests.get(url, auth=(username, password))
print("REST API Status Code:\t" + str(response.status_code))
# JSON Parse. Fully decoded
temps = response.json()
temperaturesCount = len(temps["temperatures"])
#
#   Count Values with Zero
#
zerocount = 0
for i in range(temperaturesCount):
    if(temps["temperatures"][i]["value"] == 0):
        zerocount += 1
del i

# Only one value is allowed to be Zero.
if(zerocount > 1):
    print(f"{logcolour.FAIL}Implausible data returned:")
    for i in range(temperaturesCount):
        vali = temps["temperatures"][i]["value"]
        if(vali == 0):
            print(f"{logcolour.FAIL}{logcolour.BOLD}")
        else:
            print(logcolour.OKBLUE)
        print(f'{temps["temperatures"][i]["name"]} = {vali} °C{logcolour.ENDC}')
    del i
    exit()

del zerocount
#
#   DATABASE
#
try:
    sql = "INSERT INTO temperatures ("
    # Build the keys
    for i in range(temperaturesCount):
        #print(temps["temperatures"][i]["name"])
        sql += temps["temperatures"][i]["name"]
        # no comma for lat one
        if(i != temperaturesCount - 1):
            sql +=","
    del i
    # Some SQL stuff
    sql += ") VALUES ("
    # Build the values
    for i in range(temperaturesCount):
        # Add float values
        sql += str(temps["temperatures"][i]["value"])
        # no comma for lat one
        if(i != temperaturesCount - 1):
            sql +=","
    del i
    # Finish SQL
    sql += ");"
    print(sql)
    # Database stuff
    mydb = mysql.connector.connect(
        host="localhost",
        user=username,
        password=password,
        database="heizung"
    )

    mycursor = mydb.cursor()
    mycursor.execute(sql)
    mydb.commit()
    print(f"{logcolour.OKGREEN}{mycursor.rowcount} record inserted.{logcolour.ENDC}")
except Exception as e:
    print(f"{logcolour.FAIL}Failed to insert values in Database{logcolour.ENDC}")
finally:
    if mydb.is_connected():
        mydb.close()
        mycursor.close()