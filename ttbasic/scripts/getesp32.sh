#!/bin/bash
ARDUINO_VERSION=1.8.5
ARDUINO_ARCH=`uname -m|sed -e s,aarch64,arm, -e s,x86_64,64,`
ARDUINO_URL=https://downloads.arduino.cc/arduino-${ARDUINO_VERSION}-linux${ARDUINO_ARCH}.tar.xz
ARDUINO_ESP32_GIT_URL=https://github.com/espressif/arduino-esp32.git
ARDUINO_ESP32_COMMIT=80c110ece70b179ddfe686e8ee45b6c808779454
ESP_IDF_GIT_URL=https://github.com/uli/esp-idf-beshuttle.git

set -e

mkdir -p tools/esp32
cd tools/esp32

if ! test -e downloaded1 ; then
    git clone ${ESP_IDF_GIT_URL} esp-idf
    touch downloaded1
fi

if ! test -e downloaded2 ; then
    cd ..
    wget -c $ARDUINO_URL
    cd esp32
    tar xvf ../${ARDUINO_URL##*/}
    # work around arduino-builder error
    sed -i 's,{runtime.tools.ctags.path},/usr/bin,' arduino-${ARDUINO_VERSION}/hardware/platform.txt
    touch downloaded2
fi

if ! test -e downloaded3 ; then
    git clone ${ARDUINO_ESP32_GIT_URL} arduino-esp32
    cd arduino-esp32
    git checkout $ARDUINO_ESP32_COMMIT
    cd ..
    touch downloaded3
fi
cd arduino-esp32
if ! test -e downloaded4 ; then
    git submodule update --init --recursive
    touch downloaded4
fi
cd tools
if ! test -e downloaded5 ; then
    if test `uname -m` == aarch64 ; then
        # no pre-built ARM-hosted toolchain
        sudo apt-get -y install gawk grep gettext python-dev automake texinfo help2man libtool libtool-bin
        pushd .
        cd ../..
        mkdir -p toolchain
        cd toolchain
        git clone -b xtensa-1.22.x https://github.com/espressif/crosstool-NG.git
        cd crosstool-NG
        ./bootstrap
        ./configure --enable-local
        MAKELEVEL=0 make install
        ./ct-ng xtensa-esp32-elf
        ./ct-ng build
        chmod -R u+w builds/xtensa-esp32-elf
        popd
        ln -sf ../../toolchain/crosstool-NG/builds/xtensa-esp32-elf .
    else
    	python get.py
    fi
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
