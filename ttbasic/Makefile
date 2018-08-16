# Change these paths to match your system:

# Path to Arduino installation used for ESP8266
ARDUINO_DIR = ~/devel/esp8266/arduino-1.8.3
# Path to Arduino installation used for ESP32
ARDUINO_ESP32_DIR = ~/devel/esp32/arduino-1.8.5
# Path to Arduino ESP32 core
ARDUINO_CORE_ESP32_DIR = ~/devel/esp32/arduino-esp32

# No changes required below this point.

ESPTOOL = $(ARDUINO_DIR)/hardware/esp8266com/esp8266_nowifi/tools/esptool/esptool
ESPTOOL_ESP32 = $(ARDUINO_ESP32_DIR)/hardware/espressif/esp32/tools/esptool.py

BUILD_PATH = $(PWD)/../build

DEST_ADDR = 0x00000000

all: elf doc

elf:	funtbl.h kwtbl.h epigrams.h version.h
	$(ARDUINO_DIR)/arduino-builder \
		-hardware "$(ARDUINO_DIR)/hardware" \
		-tools "$(ARDUINO_DIR)/tools-builder" \
		-fqbn "esp8266com:esp8266_nowifi:basic_engine:CpuFrequency=160,FlashSize=4M3M" \
		-built-in-libraries $(ARDUINO_DIR)/libraries \
		-libraries ../libraries \
		-warnings all \
		-build-path $(BUILD_PATH) -compile ttbasic.ino
	scripts/check8266iram.sh $(BUILD_PATH)/ttbasic.ino.elf

elf_net:	funtbl.h kwtbl.h epigrams.h version.h
	$(ARDUINO_DIR)/arduino-builder \
		-hardware "$(ARDUINO_DIR)/hardware" \
		-tools "$(ARDUINO_DIR)/tools-builder" \
		-fqbn "esp8266com:esp8266:d1_mini:CpuFrequency=160,FlashSize=4M3M" \
		-built-in-libraries $(ARDUINO_DIR)/libraries \
		-libraries ../libraries \
		-warnings all \
		-build-path $(BUILD_PATH)_net -compile ttbasic.ino

esp32:	funtbl.h kwtbl.h epigrams.h version.h
	$(ARDUINO_ESP32_DIR)/arduino-builder \
		-build-path $(BUILD_PATH)_esp32 \
		-hardware ~/devel/esp32/arduino-1.8.5/hardware \
		-tools $(ARDUINO_CORE_ESP32_DIR)/tools \
		-fqbn "espressif:esp32:esp32:FlashFreq=80,FlashMode=dio,PartitionScheme=no_ota" \
		-libraries ../libraries \
		-compile ttbasic.ino

funtbl.h kwtbl.h: icode.txt icode.py
	python icode.py
epigrams.h: epigrams.txt epi.py
	python epi.py <epigrams.txt >epigrams.h
version.h: ../.git/index
	echo "#define STR_VARSION \"$(shell git describe --abbrev=4 --dirty --always --tags)\"" >$@

clean:
	rm -fr $(BUILD_PATH)/*
	rm -fr $(BUILD_PATH)_NET/*
	rm -f ../doc/reference*.adoc ../doc/manual.html

doc:
	bash scripts/mkref.sh || true

SER ?= /dev/ttyUSB0
upload: elf
	for i in 1 2 ; do $(ESPTOOL) -cd ck -cb 921600 -cp $(SER) -ca $(DEST_ADDR) -cf $(BUILD_PATH)/ttbasic.ino.bin && break ; done
upload_net: elf_net
	for i in 1 2 ; do $(ESPTOOL) -cd ck -cb 921600 -cp $(SER) -ca $(DEST_ADDR) -cf $(BUILD_PATH)_net/ttbasic.ino.bin && break ; done

upload_esp32: esp32
	python $(ESPTOOL_ESP32) \
		--chip esp32 --port $(SER) --baud 921600  --before default_reset \
		--after hard_reset write_flash -z --flash_mode dio --flash_freq 80m \
		--flash_size detect \
		0xe000 $(ARDUINO_CORE_ESP32_DIR)/tools/partitions/boot_app0.bin \
		0x1000 $(ARDUINO_CORE_ESP32_DIR)/tools/sdk/bin/bootloader_dio_80m.bin \
		0x10000 "$(BUILD_PATH)_esp32/ttbasic.ino.bin" \
		0x8000 "$(BUILD_PATH)_esp32/ttbasic.ino.partitions.bin"
