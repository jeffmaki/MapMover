#!/bin/sh

killall -9 map
killall -9 esd
esd &
cd /exhibit
/usr/bin/screen -d -m ./map
