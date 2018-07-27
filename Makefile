all:
	@echo 'run "make native", "make net" or "make hosted"'

native:
	cd ttbasic ; $(MAKE)
net:
	cd ttbasic ; $(MAKE) net
.PHONY: hosted
hosted:
	$(MAKE) -f Makefile.hosted
