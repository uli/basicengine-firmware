#!/bin/bash
ARDUINO_DIR=../tools/esp8266/arduino-1.8.5
ARDUINO_BSP=../tools/esp8266/arduino-1.8.5/hardware/esp8266com/esp8266_nowifi
TOOLCHAIN_DIR=${ARDUINO_BSP}/tools/xtensa-lx106-elf
LIBS_DIR=../libraries
clang-tidy-6.0 "$@" -- \
  -nostdinc \
  -I${ARDUINO_DIR}/libraries/Time \
  -I${TOOLCHAIN_DIR}/xtensa-lx106-elf/include/c++/4.8.2/xtensa-lx106-elf \
  -I${TOOLCHAIN_DIR}/xtensa-lx106-elf/include/c++/4.8.2 \
  -I${TOOLCHAIN_DIR}/lib/gcc/xtensa-lx106-elf/4.8.2/include \
  -I${ARDUINO_BSP}/cores/esp8266 \
  -I${ARDUINO_BSP}/tools/sdk/include \
  -I${ARDUINO_BSP}/tools/sdk/libc/xtensa-lx106-elf/include \
  -I${ARDUINO_BSP}/variants/basic_engine \
  -I${LIBS_DIR}/TTVoutfonts -I${ARDUINO_BSP}/libraries/SPI -I${LIBS_DIR}/TTBAS_LIB -I${LIBS_DIR}/TKeyboard/src \
  -I${LIBS_DIR}/ESP8266SAM/src \
  -I${LIBS_DIR}/azip \
  -I${LIBS_DIR}/Time \
  -I${LIBS_DIR}/lua \
  -I${ARDUINO_BSP}/libraries/Wire -I${LIBS_DIR}/SdFat/src \
  -D_GCC_LIMITS_H_ -DARDUINO -D__XTENSA__ -DESP8266 -DESP8266_NOWIFI -D__ets__ \
  -DF_CPU=160000000 \
  -DSTATIC_ANALYSIS \
  -std=c++11 -m32 -U__GNUC__ -Wno-unknown-attributes -Wall
