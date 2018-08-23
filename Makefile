all:
	@echo 'run "make native", "make net" or "make hosted"'

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
