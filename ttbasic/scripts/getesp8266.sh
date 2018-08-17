#!/bin/bash
ARDUINO_VERSION=1.8.5
ARDUINO_URL=https://downloads.arduino.cc/arduino-${ARDUINO_VERSION}-linux64.tar.xz

ARDUINO_ESP8266_GIT_URL=https://github.com/esp8266/Arduino.git
ARDUINO_ESP8266_NOWIFI_GIT_URL=https://github.com/uli/Arduino_nowifi.git

set -e

mkdir -p tools/esp8266
cd tools/esp8266

if ! test -e downloaded1 ; then
    wget -c $ARDUINO_URL
    tar xvf ${ARDUINO_URL##*/}
    touch downloaded1
fi

cd arduino-${ARDUINO_VERSION}
cd hardware
mkdir -p esp8266com
cd esp8266com

if ! test -e downloaded2 ; then
    git clone ${ARDUINO_ESP8266_GIT_URL} esp8266
    touch downloaded2
fi

cd esp8266/tools

if ! test -e downloaded3 ; then
    python get.py
    touch downloaded3
fi

cd ../..

if ! test -e downloaded4 ; then
    git clone ${ARDUINO_ESP8266_NOWIFI_GIT_URL} esp8266_nowifi
    touch downloaded4
fi

cd esp8266_nowifi/tools

if ! test -e downloaded5 ; then
    python get.py
    touch downloaded5
fi
