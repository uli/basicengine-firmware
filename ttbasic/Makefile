ARDUINO_DIR = /home/uli/devel/esp8266/arduino-1.8.3

all: elf

elf:
	$(ARDUINO_DIR)/arduino-builder \
		-hardware "$(ARDUINO_DIR)/hardware" \
		-tools "$(ARDUINO_DIR)/tools-builder,/home/uli/.arduino15/packages" \
		-fqbn "esp8266com:esp8266_nowifi:d1_mini:CpuFrequency=80,FlashSize=4M3M" \
		-built-in-libraries $(ARDUINO_DIR)/libraries \
		-build-path $(PWD)/build -compile ttbasic.ino

clean:
	rm -fr $(PWD)/build/*
