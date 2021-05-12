all:
	@echo 'Run one of the following commands to compile the firmware:'
	@echo
	@echo -e '"make h3"\tAllwinner H3 Engine BASIC build'
	@echo -e '"make sdl"\tEngine BASIC build with SDL 1.2 backend'
	@echo
	@echo 'You have to install dependencies and toolchains manually.'
	@echo 'You have to edit Makefile.h3 to point to the required'
	@echo 'allwinner-bare-metal framework.'

.PHONY: h3 sdl

h3:
	$(MAKE) -f Makefile.h3
sdl:
	$(MAKE) -f Makefile.sdl
