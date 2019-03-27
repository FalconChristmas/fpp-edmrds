#!/bin/bash

sudo apt-get -y update
sudo apt-get -y install pigpio python-pigpio python3-pigpio

sudo systemctl enable pigpiod

########################################################################
# Raspbian Stretch defaults to IPv6 before IPv4 so this breaks pigpiod
# startup.  This is a known issue documented in pigpiod github at:
#
# https://github.com/joan2937/pigpio/issues/203
#
# The work-around is to force binding to 127.0.0.1
#
sudo sed -i -e "s/pigpiod -l/pigpiod -l -n 127.0.0.1/" /lib/systemd/system/pigpiod.service
sudo systemctl daemon-reload
########################################################################

sudo systemctl start pigpiod

sudo python /home/fpp/media/plugins/fpp-edmrds/rds-song.py -i

