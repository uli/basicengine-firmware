all:
	@echo 'run "make native", "make net" or "make hosted"'

native: downloads_esp8266
	cd ttbasic ; $(MAKE)
net:	downloads_esp8266
	cd ttbasic ; $(MAKE) net
.PHONY: hosted downloads downloads_esp8266 downloads_esp32
hosted:
	$(MAKE) -f Makefile.hosted

downloads:
	bash ttbasic/scripts/installpackages.sh
	bash ttbasic/scripts/getlibs.sh

downloads_esp8266: downloads
	bash ttbasic/scripts/getesp8266.sh
