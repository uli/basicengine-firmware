ARDUINO_DIR = /home/uli/devel/esp8266/arduino-1.8.3
ESPTOOL = $(ARDUINO_DIR)/hardware/esp8266com/esp8266_nowifi/tools/esptool/esptool
BUILD_PATH = $(PWD)/../build

DEST_ADDR = 0x00000000

all: elf

elf:	funtbl.h kwtbl.h epigrams.h version.h
	$(ARDUINO_DIR)/arduino-builder \
		-hardware "$(ARDUINO_DIR)/hardware" \
		-tools "$(ARDUINO_DIR)/tools-builder" \
		-fqbn "esp8266com:esp8266_nowifi:basic_engine:CpuFrequency=160,FlashSize=4M3M" \
		-built-in-libraries $(ARDUINO_DIR)/libraries \
		-libraries ../libraries \
		-warnings all \
		-build-path $(BUILD_PATH) -compile ttbasic.ino

elf_net:	funtbl.h kwtbl.h epigrams.h version.h
	$(ARDUINO_DIR)/arduino-builder \
		-hardware "$(ARDUINO_DIR)/hardware" \
		-tools "$(ARDUINO_DIR)/tools-builder" \
		-fqbn "esp8266com:esp8266:d1_mini:CpuFrequency=160,FlashSize=4M3M" \
		-built-in-libraries $(ARDUINO_DIR)/libraries \
		-libraries ../libraries \
		-warnings all \
		-build-path $(BUILD_PATH)_net -compile ttbasic.ino

funtbl.h kwtbl.h: icode.txt icode.py
	python icode.py
epigrams.h: epigrams.txt epi.py
	python epi.py <epigrams.txt >epigrams.h
version.h: ../.git/index
	echo "#define STR_VARSION \"$(shell git describe --abbrev=4 --dirty --always --tags)\"" >$@

clean:
	rm -fr $(BUILD_PATH)/*
	rm -fr $(BUILD_PATH)_NET/*

SER ?= /dev/ttyUSB0
upload: elf
	for i in 1 2 ; do $(ESPTOOL) -cd ck -cb 921600 -cp $(SER) -ca $(DEST_ADDR) -cf $(BUILD_PATH)/ttbasic.ino.bin && break ; done
upload_net: elf_net
	for i in 1 2 ; do $(ESPTOOL) -cd ck -cb 921600 -cp $(SER) -ca $(DEST_ADDR) -cf $(BUILD_PATH)_net/ttbasic.ino.bin && break ; done
