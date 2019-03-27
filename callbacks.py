#!/usr/bin/env python

import argparse
import json
import os
import subprocess
import sys
import time


class Logger(object):
    def __init__(self, filename="/var/log/edmrds.log"):
        self.terminal = sys.stdout
        self.log = open(filename, "a")

    def write(self, message):
        self.terminal.write(message)
        self.log.write(message)


sys.stdout = Logger("/home/fpp/media/logs/edmrds.log")

parser = argparse.ArgumentParser(description='RDS Setting Application')
parser.add_argument(
    '-t',
    '--type',
    help='Input station name (8 characters max)',
    required=False)
parser.add_argument('-l', '--list', help='Song name', action='store_true')
parser.add_argument('-d', '--data', help='Song name')
args = parser.parse_args()

if args.list:
    # Tell the plugin that we should be registered for media
    print("media")

if args.type:
    # Callback should be invoked with --type media
    print("type: %s" % args.type)

if args.data:
    # Get the json string from FPP
    print("data: %s" % args.data)
    data = json.loads(args.data)
    title = data['title']
    print("title: %s" % title)
    plugin_dir = os.path.dirname(__file__)
    subprocess.call(
        [os.path.join(plugin_dir, 'rds-song.py'), '-s', title], env={"PYTHONPATH": plugin_dir})
