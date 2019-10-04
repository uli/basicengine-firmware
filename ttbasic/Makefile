# Change these paths to match your system:

# Path to Arduino installation used for ESP8266
ARDUINO_DIR = ../tools/esp8266/arduino-1.8.5
ESP_ROOT = $(ARDUINO_DIR)/hardware/esp8266com/esp8266
ESP_ROOT_NOWIFI = $(ARDUINO_DIR)/hardware/esp8266com/esp8266_nowifi

# Path to Arduino installation used for ESP32
ARDUINO_ESP32_DIR = ../tools/esp32/arduino-1.8.5
# Path to Arduino ESP32 core
ARDUINO_CORE_ESP32_DIR = ../tools/esp32/arduino-esp32

LIBS =	../libraries/TTVoutfonts \
	../libraries/TTBAS_LIB \
	../libraries/TKeyboard \
	../libraries/SdFat/src \
	../libraries/lua \
	../libraries/Time \
	../libraries/azip \
	../libraries/ESP8266SAM/src \
	../libraries/Psx \

# No changes required below this point.

ESPTOOL = scripts/esptool.py

BUILD_PATH = $(PWD)/../build

DEST_ADDR = 0x00000000

all: elf doc

elf:	funtbl.h kwtbl.h epigrams.h version.h
	make -f scripts/makeEspArduino.mk \
		CHIP=esp8266 \
		BOARD=basic_engine \
		FLASH_DEF=4M3M \
		F_CPU=160000000L \
		ESP_ROOT=$(ESP_ROOT_NOWIFI) \
		LIBS="$(ESP_ROOT_NOWIFI)/libraries/SPI $(ESP_ROOT_NOWIFI)/libraries/Wire $(LIBS)" \
		BUILD_DIR=$(BUILD_PATH) \
		SKETCH=ttbasic.ino \
		COMP_WARNINGS="-Wall -Wextra -Wno-unused-parameter"
	scripts/check8266iram.sh $(BUILD_PATH)/ttbasic.elf

elf_net:	funtbl.h kwtbl.h epigrams.h version.h
	make -f scripts/makeEspArduino.mk \
		CHIP=esp8266 \
		BOARD=d1_mini \
		FLASH_DEF=4M3M \
		F_CPU=160000000L \
		ESP_ROOT=$(ESP_ROOT) \
		LIBS="$(ESP_ROOT)/libraries/SPI $(ESP_ROOT)/libraries/Wire \
		      $(ESP_ROOT)/libraries/ESP8266WiFi \
		      $(ESP_ROOT)/libraries/ESP8266HTTPClient \
		      $(LIBS)" \
		BUILD_DIR=$(BUILD_PATH)_net \
		SKETCH=ttbasic.ino \
		COMP_WARNINGS="-Wall -Wextra -Wno-unused-parameter"

esp32:	funtbl.h kwtbl.h epigrams.h version.h
	make -f scripts/makeEspArduino.mk \
		CHIP=esp32 \
		BOARD=esp32 \
		FLASH_MODE=dio \
		FLASH_SPEED=80m \
		FLASH_DEF=no_ota \
		ESP_ROOT=$(ARDUINO_CORE_ESP32_DIR) \
		LIBS="$(ARDUINO_CORE_ESP32_DIR)/libraries/SPI \
		      $(ARDUINO_CORE_ESP32_DIR)/libraries/Wire \
		      $(ARDUINO_CORE_ESP32_DIR)/libraries/SD \
		      $(ARDUINO_CORE_ESP32_DIR)/libraries/FS \
		      $(ARDUINO_CORE_ESP32_DIR)/libraries/SPIFFS \
		      $(ARDUINO_CORE_ESP32_DIR)/libraries/WiFi \
		      $(ARDUINO_CORE_ESP32_DIR)/libraries/HTTPClient \
		      $(LIBS)" \
		BUILD_DIR=$(BUILD_PATH)_esp32 \
		SKETCH=ttbasic.ino \
		COMP_WARNINGS="-Wall -Wextra -Wno-unused-parameter -Wno-deprecated-declarations"

include autogen.mk

clean: autogen_clean
	rm -fr $(BUILD_PATH)/*
	rm -fr $(BUILD_PATH)_net/*
	rm -fr $(BUILD_PATH)_esp32/*
	rm -f ../doc/reference*.adoc ../doc/manual.html

doc:
	bash scripts/mkref.sh || true

SER ?= /dev/ttyUSB0
upload: elf
	python $(ESPTOOL) -c esp8266 -b 921600 -p $(SER) write_flash $(DEST_ADDR) $(BUILD_PATH)/ttbasic.bin
upload_net: elf_net
	python $(ESPTOOL) -c esp8266 -b 921600 -p $(SER) write_flash $(DEST_ADDR) $(BUILD_PATH)_net/ttbasic.ino.bin

upload_esp32: esp32
	python $(ESPTOOL) \
		--chip esp32 --port $(SER) --baud 921600  --before default_reset \
		--after hard_reset write_flash -z --flash_mode dio --flash_freq 80m \
		--flash_size detect \
		0xe000 $(ARDUINO_CORE_ESP32_DIR)/tools/partitions/boot_app0.bin \
		0x1000 $(ARDUINO_CORE_ESP32_DIR)/tools/sdk/bin/bootloader_dio_80m.bin \
		0x10000 "$(BUILD_PATH)_esp32/ttbasic.bin" \
		0x8000 "$(BUILD_PATH)_esp32/ttbasic.partitions.bin"
