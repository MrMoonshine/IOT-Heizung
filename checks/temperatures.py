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
import json
import requests
import os
from datetime import datetime
from datetime import timedelta
import check as common

# Create Check Class
check = common.Check("temperatures")
 
# URL
url = "http://heizung/api/temperatures"
response = requests.get(url, auth=(check.username, check.password))
print("REST API Status Code:\t" + str(response.status_code))
# JSON Parse. Fully decoded
temps = response.json()

names = ["blue","red","green","yellow","white", "brown", "room", "solar"]

#
#   Count Values with Zero
#
zerocount = 0
for i in names:
    if(temps[i] == 0):
        zerocount += 1
del i

# Only one value is allowed to be Zero.
if(zerocount > 1):
    check.status = common.CheckStatus.WARNING;
    check.close("Implausible result")
    exit()
del zerocount
#
#   SQL
#
sql = "INSERT INTO temperatures ("
for i in range(len(names)):
    sql += names[i]
    if(i != len(names) - 1):
            sql +=","
del i
sql += ") VALUES ("
for i in range(len(names)):
    sql += str(temps[names[i]])
    if(i != len(names) - 1):
            sql +=","
del i
sql += ");"
print(sql)
check.execDML(sql)

check.status = common.CheckStatus.OK
check.close("All OK")