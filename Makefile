all:
	@echo 'Run ./configure.sh, then run one of the following'
	@echo 'commands to compile the firmware:'
	@echo
	@echo -e '"ninja -f build.ninja.h3"\tAllwinner H3 Engine BASIC build'
	@echo -e '"ninja -f build.ninja.sdl"\tEngine BASIC build with SDL 1.2 backend'
	@echo
	@echo 'You have to install dependencies and toolchains manually.'
	@echo 'You have to run configure.sh with the H3_OSDIR environment variable set'
	@echo 'to the allwinner-bare-metal framework directory for the H3 build to work.'

.PHONY: h3 sdl

build.ninja.common:
	H3_OSDIR=$(H3_OSDIR) ./configure.sh 

h3:	build.ninja.common
	ninja -f build.ninja.h3
sdl:	build.ninja.common
	ninja -f build.ninja.sdl

