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
import check as common

def resetTime(uptime):
    time = datetime.now() - timedelta(seconds=uptime)
    return time.strftime("%Y-%m-%d %H:%M")

# Create Check Class
check = common.Check("uptime")

# URL
url = "http://heizung/api/reset"
response = requests.get(url, auth=(check.username, check.password))
print("REST API Status Code:\t" + str(response.status_code))
# JSON Parse. Fully decoded
reset = response.json()
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
check.cursor.execute(sql)
for row in check.cursor.fetchall():
    reloadfound = row[0] > 0
del row

if(reloadfound):
    print("No Record found. OK")
    check.status = common.CheckStatus.OK
else:
    print("New Reload. WARNING")
    sql = "insert into check_uptime (reload,reason,ref) values ('"
    sql += reset_time
    sql += "','"
    sql += reset["reason"]
    sql += "',false);"
    print(sql)
    check.execDML(sql)
    check.status = common.CheckStatus.WARNING
#delete check
check.close(f'Up since {reset_time}')