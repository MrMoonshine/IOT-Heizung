from datetime import datetime
from enum import Enum
import json
from pprint import pprint
from colorama import Cursor
# pip install mysql-connector-python
import mysql.connector
from mysql.connector.errors import Error
# File with credentials as json
LOGIN_FILE = "/home/david/.confidential/heizung-rest-user.json"

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

class CheckStatus(Enum):
    OK = 0
    WARNING = 1
    CRITICAL = 2
    UNKNOWN = 3
    PENDING = 4

class Check:
    # Constructor
    def __init__(self, cname):
        self.name = cname
        self.id = -1
        self.status = CheckStatus.UNKNOWN
        print(print(f"{logcolour.OKCYAN}[{datetime.now()}]{logcolour.BOLD} {self.name} Check Start.{logcolour.ENDC}"))
        try:
            # Open file to get login data
            fp = open(LOGIN_FILE, "r")
            credentials = json.load(fp)
            self.username = credentials["user"]
            self.password = credentials["password"]
            fp.close()
        except FileNotFoundError:
            print(f"{logcolour.FAIL}Failed to read {LOGIN_FILE}{logcolour.ENDC}")
            exit()
        
        # Handle database stuff
        try:
            self.db = mysql.connector.connect(
            host="localhost",
            user=self.username,
            password=self.password,
            database="heizung"
            )
            self.cursor = self.db.cursor()
            # Does check already exist?
            existquery = f'select check_id from check_names where check_name = "{self.name}";'
            self.cursor.execute(existquery)
            result = self.cursor.fetchall()
            # if no records, create new one
            if(self.cursor.rowcount < 1):
                print("Adding new Check: " + self.name)
                self.execDML(f'insert into check_names(check_name) values("{self.name}");')
                # Fetch again, now ther must exist one
                self.cursor.execute(existquery)
                result = self.cursor.fetchall()
            # set ID
            for row in result:
                self.id = row[0]
            del row
        except Error as err:
            print(f"{logcolour.FAIL}Database Error: {err}{logcolour.ENDC}")
        except Exception as e:
            print(f"{logcolour.FAIL}Failed to fetch existing Reloads{logcolour.ENDC}")
    # CLose database connection
    def close(self, comment = ""):
        self.execDML(f'insert into check_status (check_id, check_status, comment) values ({self.id}, {self.status.value}, "{comment}");')
        if self.db.is_connected():
            self.cursor.close()
            self.db.close()
    # Insert Delete Update
    def execDML(self, sql):
        try:
            self.cursor = self.db.cursor()
            self.cursor.execute(sql)
            self.db.commit()
            print(self.cursor.rowcount, " record(s) inserted.")
        except Exception as e:
            print(f"{logcolour.FAIL}Failed to execute Query:\n{sql}\n{Error()}{logcolour.ENDC}")