#!/bin/bash
ARDUINO_DIR=$HOME/.arduino15
ARDUINO_BSP=../../Arduino_nowifi
TOOLCHAIN_DIR=${ARDUINO_DIR}/packages/esp8266/tools/xtensa-lx106-elf-gcc/1.20.0-26-gb404fb9-2
SDFAT_DIR=../../../arduino/SdFat
LIBS_DIR=../libraries

clang-tidy-5.0 "$@" -- \
  -I${TOOLCHAIN_DIR}/xtensa-lx106-elf/include/c++/4.8.2/xtensa-lx106-elf \
  -I${TOOLCHAIN_DIR}/xtensa-lx106-elf/include/c++/4.8.2 \
  -I${TOOLCHAIN_DIR}/lib/gcc/xtensa-lx106-elf/4.8.2/include \
  -I${ARDUINO_BSP}/cores/esp8266 \
  -I${ARDUINO_BSP}/tools/sdk/include \
  -I${ARDUINO_BSP}/variants/basic_engine \
  -I${LIBS_DIR}/TTVoutfonts -I${ARDUINO_BSP}/libraries/SPI -I${LIBS_DIR}/TTBAS_LIB -I${LIBS_DIR}/TKeyboard/src \
  -I${ARDUINO_BSP}/libraries/Wire -I${SDFAT_DIR}/src \
  -D_GCC_LIMITS_H_ -DARDUINO -D__XTENSA__ -DESP8266 -DESP8266_NOWIFI -D__ets__ \
  -std=c++11 -m32 -U__GNUC__ -Wno-unknown-attributes -Wall
