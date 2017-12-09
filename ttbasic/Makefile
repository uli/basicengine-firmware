ARDUINO_DIR = /home/uli/devel/esp8266/arduino-1.8.3
ESPTOOL = $(ARDUINO_DIR)/hardware/esp8266com/esp8266_nowifi/tools/esptool/esptool

DEST_ADDR = 0x00000000

all: elf

elf:	funtbl.h
	$(ARDUINO_DIR)/arduino-builder \
		-hardware "$(ARDUINO_DIR)/hardware" \
		-tools "$(ARDUINO_DIR)/tools-builder,/home/uli/.arduino15/packages" \
		-fqbn "esp8266com:esp8266_nowifi:d1_mini:CpuFrequency=80,FlashSize=4M3M" \
		-built-in-libraries $(ARDUINO_DIR)/libraries \
		-build-path $(PWD)/build -compile ttbasic.ino

funtbl.h: icode.txt icode.py
	python icode.py

clean:
	rm -fr $(PWD)/build/*

upload: elf
	$(ESPTOOL) -cd ck -cb 460800 -cp /dev/ttyUSB0 -ca $(DEST_ADDR) -cf build/ttbasic.ino.bin
