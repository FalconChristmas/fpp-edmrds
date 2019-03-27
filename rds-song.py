#!/usr/bin/env python
"""
Configuration of the MiniRDS as per http://www.edmdesign.com/docs/EDM-TX-RDS.pdf
Address information and valudes found from http://pira.cz/rds/mrds192.pdf
Step 1: Set PTY Coding
PTY Coding (set for US)
PTY Flag is set when we do a write to EEPROM as we would never change this otherwise!
PTY = program type = 0x0A = value of 2 which is Information
Step 2: Set Subcarier phase shift to 85.23 degrees, Clock source Auto(stero), PLL Lock Range 19000+/- 5Hz
Step 3: Set PS name
Step 4: Set Radio text
Dyanmic PS is also used by default
"""

import argparse
import sys
import time

import bb_I2C
import pigpio


if len(sys.argv) == 1:
    sys.exit(
        "The total numbers of arguments passed is 1.\nRun the app with the -h command for help."
    )

parser = argparse.ArgumentParser(description='RDS Setting Application')
parser.add_argument(
    '-c',
    '--change',
    help='Input station name (8 characters max)',
    required=False)
parser.add_argument('-s', '--song', help='Song name', required=False)
parser.add_argument(
    '-l',
    '--liststation',
    help='Print out the station id',
    required=False,
    action="store_true")
parser.add_argument(
    '-n',
    '--nowplaying',
    help='Print out current radiotext',
    required=False,
    action="store_true")
parser.add_argument(
    '-i',
    '--install',
    help='Run the first time you install to turn off Dynamic PS',
    required=False,
    action="store_true")
parser.add_argument(
    '-w',
    '--write',
    help='Write to memory',
    required=False,
    action="store_true")
args = parser.parse_args()

pi = pigpio.pi()
s = bb_I2C.I2C(pi, 23, 24, 600)
if args.change:
    # We have a station name to change
    # So...Set name of radio station
    if len(args.change) >= 9:
        sys.exit("The station name has to be 8 characters or less")
    stationname = args.change
    stationname = stationname.ljust(8, ' ')
    s.S()
    s.TX(214)
    s.TX(2)
    for x in range(0, 8):
        s.TX(ord(stationname[x]))
    s.E()
    print("Station name changed to: %s" % stationname)

if args.install:
    # Store settings to eeprom
    print("Setting dynamic PS to off and saving eeprom...")
    # turn off dynamic PS because it's bad (although EDM has it on by default - http://www.rds.org.uk/2010/Usage-of-PS.htm)
    s.S()
    s.TX(214)
    s.TX(0x76)
    s.TX(0)
    s.E()
    s.S()
    s.TX(214)
    s.TX(0x71)
    s.TX(0x45)
    s.E()
    print("Settings saved to EEPROM")

if args.song:
    # we have to change the song title playing
    if len(args.song) >= 64:
        sys.exit("The song has to be 64 characters or less")
    radiotext = args.song
    radiotext = radiotext.ljust(64, ' ')
    s.S()
    s.TX(214)
    s.TX(0x20)
    for x in range(0, 64):
        s.TX(ord(radiotext[x]))
    s.E()
    print("Radiotext changed to: %s" % radiotext)

if args.liststation:
    # print out the radio station id
    s.S()
    s.TX(214)
    s.TX(2)  # 0x77 is for Dynamic PS 0x20 is RT
    s.S()
    s.TX(215)
    numbers = []
    # 1 = ack, 0-nack
    numbers.append(s.RX(1))
    numbers.append(s.RX(1))
    numbers.append(s.RX(1))
    numbers.append(s.RX(1))
    numbers.append(s.RX(1))
    numbers.append(s.RX(1))
    numbers.append(s.RX(1))
    numbers.append(s.RX(0))
    print('%s' % ''.join([chr(n) for n in numbers]))
    s.E()

if args.nowplaying:
    # print out the current radio text information
    radiotext = ""
    s.S()
    s.TX(214)
    s.TX(0x20)
    s.S()
    s.TX(215)
    for x in range(0, 63):
        radiotext = radiotext + chr(s.RX(1))
    radiotext = radiotext + chr(s.RX(0))
    s.E()
    print("%s" % radiotext)

if args.write:
    # Store settings to eeprom
    print("Saving...")
    s.S()
    s.TX(214)
    s.TX(0x71)
    s.TX(0x45)
    s.E()
    print("Settings saved to EEPROM")

s.cancel()
pi.stop()
