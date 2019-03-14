#!/bin/bash
ARDUINO_DIR=../tools/esp8266/arduino-1.8.5
ARDUINO_BSP=../tools/esp8266/arduino-1.8.5/hardware/esp8266com/esp8266_nowifi

test -z "$PLATFORM" && PLATFORM="ESP8266_NOWIFI"

if test "$PLATFORM" == ESP8266_NOWIFI ; then
 	PFLAGS="-DARDUINO -D__XTENSA__ -DESP8266 -DESP8266_NOWIFI"
	GNUPF="xtensa-lx106-elf"
	GCCVER="4.8.2"
	ARDUINO_CORE="esp8266"
elif test "$PLATFORM" == ESP8266 ; then
	ARDUINO_BSP=../tools/esp8266/arduino-1.8.5/hardware/esp8266com/esp8266
	PFLAGS="-DARDUINO -D__XTENSA__ -DESP8266"
	GNUPF="xtensa-lx106-elf"
	GCCVER="4.8.2"
	ARDUINO_CORE="esp8266"
elif test "$PLATFORM" == ESP32 ; then
	ARDUINO_DIR=../tools/esp32/arduino-1.8.5
	ARDUINO_BSP=../tools/esp32/arduino-1.8.5/hardware/espressif/esp32
	ESP32INC="${ARDUINO_BSP}/tools/sdk/include"
	GNUPF="xtensa-esp32-elf"
	GCCVER="5.2.0"
	ARDUINO_CORE="esp32"
	# what an incredibly annoying platform
	PFLAGS="-DARDUINO -D__XTENSA__ -DESP32 -I${ESP32INC}/soc"
	PFLAGS="$PFLAGS -I${ESP32INC}/newlib -I${ESP32INC}/freertos"
	PFLAGS="$PFLAGS -I../tools/esp32/p/build/include -I${ESP32INC}/esp32"
	PFLAGS="$PFLAGS -I${ESP32INC}/soc -I${ARDUINO_BSP}/tools/${GNUPF}/${GNUPF}/include"
	PFLAGS="$PFLAGS -I${ESP32INC}/heap -I${ESP32INC}/driver"
	PFLAGS="$PFLAGS -I${ESP32INC}/log -I${ARDUINO_BSP}/variants/esp32"
elif test "$PLATFORM" == H3 ; then
	OSDIR=~/devel/orangepi/allwinner-bare-metal
	GNUPF="arm-unknown-eabihf"
	GCCVER="8.3.0"
	TOOLCHAIN_DIR=~/x-tools/${GNUPF}
	PFLAGS="-DH3 -I../h3 -I../arduino_compat -I ${OSDIR} -I ${OSDIR}/tinyusb/src -I ${TOOLCHAIN_DIR}/${GNUPF}/include -DATTR_CONST= -DATTR_ALWAYS_INLINE="
fi

test -z "$TOOLCHAIN_DIR" && TOOLCHAIN_DIR=${ARDUINO_BSP}/tools/${GNUPF}

LIBS_DIR=../libraries

clang-tidy-6.0 "$@" -- \
  -nostdinc \
  -I${ARDUINO_DIR}/libraries/Time \
  -I${TOOLCHAIN_DIR}/${GNUPF}/include/c++/${GCCVER}/${GNUPF} \
  -I${TOOLCHAIN_DIR}/${GNUPF}/include/c++/${GCCVER} \
  -I${TOOLCHAIN_DIR}/lib/gcc/${GNUPF}/${GCCVER}/include \
  -I${ARDUINO_BSP}/cores/${ARDUINO_CORE} \
  -I${ARDUINO_BSP}/tools/sdk/include \
  -I${ARDUINO_BSP}/tools/sdk/libc/${GNUPF}/include \
  -I${ARDUINO_BSP}/variants/basic_engine \
  -I${LIBS_DIR}/TTVoutfonts -I${ARDUINO_BSP}/libraries/SPI -I${LIBS_DIR}/TTBAS_LIB -I${LIBS_DIR}/TKeyboard/src \
  -I${LIBS_DIR}/ESP8266SAM/src \
  -I${LIBS_DIR}/azip \
  -I${LIBS_DIR}/Time \
  -I${LIBS_DIR}/lua \
  -I${ARDUINO_BSP}/libraries/Wire -I${LIBS_DIR}/SdFat/src \
  -D_GCC_LIMITS_H_ $PFLAGS -D__ets__ \
  -DF_CPU=160000000 \
  -DSTATIC_ANALYSIS \
  -std=c++11 -m32 -U__GNUC__ -Wno-unknown-attributes -Wall
