import json
import requests
import sys
from pprint import pprint

if(len(sys.argv) != 3):
    print("ARGV länge muss 3 sein");
    exit();

myobj = {sys.argv[1]: sys.argv[2]}

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
url = "http://heizung/api/pumps"
response = requests.post(url, auth=(username, password), timeout=3)
print("REST API Status Code:\t" + str(response.status_code))
print(response)
# JSON Parse. Fully decoded
#temps = response.json()