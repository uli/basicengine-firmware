#!/bin/bash
ARDUINO_VERSION=1.8.5
ARDUINO_URL=https://downloads.arduino.cc/arduino-${ARDUINO_VERSION}-linux64.tar.xz
ARDUINO_ESP32_GIT_URL=https://github.com/espressif/arduino-esp32.git
ESP_IDF_GIT_URL=https://github.com/espressif/esp-idf.git

set -e

mkdir -p tools/esp32
cd tools/esp32

if ! test -e downloaded1 ; then
    git clone ${ESP_IDF_GIT_URL}
    # esp-idf moves faster than Arduino, need to use a known-working version
    cd esp-idf
    git checkout be81d2c16d7f4caeea9ceb29fece01510664caf3
    cd ..
    touch downloaded1
fi

if ! test -e downloaded2 ; then
    wget -c $ARDUINO_URL
    tar xvf ${ARDUINO_URL##*/}
    # work around arduino-builder error
    sed -i 's,{runtime.tools.ctags.path},/usr/bin,' arduino-${ARDUINO_VERSION}/hardware/platform.txt
    touch downloaded2
fi

if ! test -e downloaded3 ; then
    git clone ${ARDUINO_ESP32_GIT_URL} arduino-esp32
    touch downloaded3
fi
cd arduino-esp32
if ! test -e downloaded4 ; then
    git submodule update --init --recursive
    touch downloaded4
fi
cd tools
if ! test -e downloaded5 ; then
    python get.py
    touch downloaded5
fi
cd ../..

# link ESP32 core into Arduino installation
mkdir -p arduino-${ARDUINO_VERSION}/hardware/espressif
rm -f arduino-${ARDUINO_VERSION}/hardware/espressif/esp32
ln -s `pwd`/arduino-esp32 arduino-${ARDUINO_VERSION}/hardware/espressif/esp32

# rebuild ESP32 SDK with custom configuration
mkdir -p p
cd p
cp -pr ../esp-idf/examples/get-started/hello_world/{Makefile,main} .
mkdir -p components
cd components
rm -fr arduino-esp32
cp -a ../../arduino-${ARDUINO_VERSION}/hardware/espressif/esp32 arduino-esp32
cd ..
export IDF_PATH=`pwd`/../esp-idf
cp -p ../../../ttbasic/scripts/sdkconfig.esp32gfx sdkconfig
export PATH=`pwd`/../arduino-${ARDUINO_VERSION}/hardware/espressif/esp32/tools/xtensa-esp32-elf/bin:$PATH
make

# copy new SDK libs to Arduino installation
cp -p build/*/lib*.a ../arduino-esp32/tools/sdk/lib
cd ..
