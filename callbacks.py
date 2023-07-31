#!/usr/bin/env python3

import time
import os
import argparse   
import sys
import json
import subprocess

class Logger(object):
    def __init__(self, filename="/var/log/edmrds.log"):
        self.terminal = sys.stdout
        self.log = open(filename, "a")

    def write(self, message):
        self.terminal.write(message)
        self.log.write(message)

sys.stdout = Logger("/home/fpp/media/logs/edmrds.log")

parser = argparse.ArgumentParser(description='RDS Setting Application')
parser.add_argument('-t','--type', help='Input station name (8 characters max)', required=False)
parser.add_argument('-l','--list',help='Song name',action='store_true')
parser.add_argument('-d','--data',help='Song name')
args = parser.parse_args()

if args.list:
   #Tell the plugin that we should be registered for media
   print("media")

if args.type:
   # Look for our type of plugin, doubt this is even needed at all
   mytypearg = args.type
   print("My type args: %s" %mytypearg)

if args.data:
   #get the json string from FPP
   mydataarg = args.data
   print("My data args: %s" %mydataarg)
   data = json.loads(mydataarg)
   title = data['title'] 
   print("Title: %s" %title)
   os.chdir("/home/fpp/media/plugins/fpp-edmrds/")
   directory = os.getcwd()
   print("Directory: %s" %directory)
   #from subprocess import call
   #call(["rds-song.py", "-s %title"])
   subprocess.call(['/home/fpp/media/plugins/fpp-edmrds/rds-song.py', '-s', title])
