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
import json
import requests
import os
from datetime import datetime
from pprint import pprint
# pip install mysql-connector-python
import mysql.connector
# Start program with date and time for Logs
now = datetime.now()
print("[", now, "] Temperature Check Start")
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
print(response.status_code)
# JSON Parse. Fully decoded
temps = response.json()
#
#   DATABASE
#
try:
    sql = "INSERT INTO temperatures ("
    temperaturesCount = len(temps["temperatures"])
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
    print(mycursor.rowcount, "record inserted.")
except Exception as e:
    print("Failed to add values to database")