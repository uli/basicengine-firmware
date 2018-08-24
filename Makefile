all:
	@echo 'Run one of the following commands to compile the firmware:'
	@echo
	@echo -e '"make native"\tBASIC Engine (ESP8266) firmware'
	@echo -e '"make net"\tBASIC Engine (ESP8266) firmware with network support'
	@echo -e '"make esp32"\tBASIC Engine Shuttle (ESP32) firmware'
	@echo -e '"make hosted"\tPC-hosted Engine BASIC build'
	@echo
	@echo 'To upload firmware, run "make upload_native" (ESP8266) or'
	@echo '"make upload_esp32" (ESP32).'
	@echo
	@echo 'You can specify a serial port if necessary:'
	@echo '"make upload_native SER=/dev/ttyUSB1"'
	@echo
	@echo 'The build system will try to automatically install required packages'
	@echo 'and build suitable toolchains for each target. To this end, it will'
	@echo 'download a considerable amount of data. Be sure not to run it on a'
	@echo 'slow or metered connection the first time.'

.PHONY: native net esp32 hosted downloads downloads_esp8266 downloads_esp32 downloads_hosted

native: downloads_esp8266
	cd ttbasic ; $(MAKE)
net:	downloads_esp8266
	cd ttbasic ; $(MAKE) net
esp32:  downloads_esp32
	cd ttbasic ; $(MAKE) esp32
hosted: downloads_hosted
	$(MAKE) -f Makefile.hosted

downloads:
	bash ttbasic/scripts/installpackages.sh
	bash ttbasic/scripts/getlibs.sh

downloads_esp8266: downloads
	bash ttbasic/scripts/getesp8266.sh
downloads_esp32: downloads
	bash ttbasic/scripts/getesp32.sh
downloads_hosted:
	bash ttbasic/scripts/installpackages.sh
	bash ttbasic/scripts/installpackages_hosted.sh

upload_native: downloads_esp8266
	cd ttbasic ; $(MAKE) upload
upload_esp32: downloads_esp32
	cd ttbasic ; $(MAKE) upload_esp32
