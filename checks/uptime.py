# /usr/bin/python
#
#   Dieser Check überprüft die Uptime, errecnet daraus den Reload zeitpunkt und speichert den in die Datenbank.
#   Wenn sich die Uptime Ändert kommt eine Warnung! Beispiel: zur vollen Stunde:
#
#   crontab -e
#
#   0 */1 * * * /usr/bin/python /var/www/IOT-Heizung/checks/uptime.py >> /var/www/IOT-Heizung/checks/logs/uptime.log
#   
#
import json
import requests
import os
from datetime import datetime
from datetime import timedelta
from pprint import pprint
# pip install mysql-connector-python
import mysql.connector

def resetTime(uptime):
    time = datetime.now() - timedelta(seconds=uptime)
    return time.strftime("%Y-%m-%d %H:%M")


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
url = "http://heizung/api/reset"
response = requests.get(url, auth=(username, password))
print("REST API Status Code:\t" + str(response.status_code))
# JSON Parse. Fully decoded
reset = response.json()
pprint(reset)
# time variables
tc_dead_zone = "TIME('00:03')"
#
#   Reference https://www.techonthenet.com/mariadb
#
# select * from check_uptime where ABS(TIMEDIFF('2022-05-15 16:36', reload)) <= TIME('00:03');
# Building SQL
reset_time = resetTime(reset["uptime"])
sql = "select count(1) as existing from check_uptime where ABS(TIMEDIFF('"
sql += reset_time
sql += "', reload)) <= "
sql += tc_dead_zone
sql += ";"
print(sql)

reloadfound = False

try:
    mydb = mysql.connector.connect(
        host="localhost",
        user=username,
        password=password,
        database="heizung"
    )

    mycursor = mydb.cursor()
    mycursor.execute(sql)
    for row in mycursor.fetchall():
        reloadfound = row[0] > 0
    del row

    if(reloadfound):
        print("No Record found. OK")
    else:
        print("New Reload. WARNING")
        sql = "insert into check_uptime (reload,reason,ref) values ('"
        sql += reset_time
        sql += "','"
        sql += reset["reason"]
        sql += "',false);"
        print(sql)
        mycursor = mydb.cursor()
        mycursor.execute(sql)
        mydb.commit()
        print(mycursor.rowcount, "record inserted.")
except Exception as e:
    print("Failed to fetch existing Reloads")
finally:
    if mydb.is_connected():
        mydb.close()
        mycursor.close()