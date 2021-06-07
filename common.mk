COMMON_ARDFLAGS = -DENGINEBASIC -O3 -g -Iarduino_compat -Ittbasic $(INCLUDE) \
	   -Wall -Wno-unused -Wno-sign-compare -Wno-implicit-fallthrough \
	   -D_GNU_SOURCE

COMMON_SOURCES = $(shell ls libraries/lua/*.cpp) $(shell ls ttbasic/*.cpp) $(shell ls libraries/TTBAS_LIB/*.cpp) \
	$(shell ls libraries/ESP8266SAM/src/*.cpp) $(shell ls libraries/TTVoutfonts/*.cpp) \
	$(shell ls libraries/azip/*.cpp) \
	$(shell ls libraries/alpha-lib/src/*.cpp) \
	$(shell ls arduino_compat/*.cpp) \

COMMON_SOURCES_C = $(shell ls ttbasic/*.c) $(shell ls libraries/stb/*.c) \
	$(shell ls libraries/TTBAS_LIB/*.c) \
	$(shell ls libraries/atto/*.c) \
	libraries/tinycc/libtcc.c

PROF = $(SOURCES:.cpp=.gcda) $(SOURCES:.cpp=.gcno) \
	$(SOURCES_C:.c=.gcda) $(SOURCES_C:.c=.gcno)

COMMON_INCLUDE = -Ilibraries/TTVoutfonts -Ilibraries/TTBAS_LIB -Ilibraries/TKeyboard/src \
	-Ilibraries/ESP8266SAM/src -Ilibraries/azip -Ilibraries/lua -Ilibraries/stb \
	-Ilibraries/alpha-lib/include -Igfx -Ilibraries/dyncall/dyncall \
	-Ilibraries/tinycc

$(OBJDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(OBJDIR)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJDIR)/ttbasic/basic_help.o: ttbasic/helptext_en.json
ttbasic/helptext_en.json: ttbasic/scripts/help.py $(shell ls ttbasic/basic*.cpp) $(shell ls po/helptext_*.po)
	$(MAKE) -C ttbasic -f autogen.mk helptext_en.json

$(OBJDIR)/ttbasic/translate.o: ttbasic/msgs_de.h
ttbasic/msgs_de.h: ttbasic/scripts/msgs.py $(shell ls ttbasic/basic*.cpp) $(shell ls po/system_*.po)
	$(MAKE) -C ttbasic -f autogen.mk msgs_de.h

ttbasic/funtbl.h: ttbasic/icode.txt
	$(MAKE) -C ttbasic -f autogen.mk funtbl.h epigrams.h version.h
$(OUT_OBJS): ttbasic/funtbl.h
$(SOURCES): ttbasic/funtbl.h
$(COMMON_SOURCES): ttbasic/funtbl.h

$(OBJDIR)/dyncall/Makefile.config:
	mkdir -p $(OBJDIR)/dyncall
	cd $(OBJDIR)/dyncall ; CC=$(CC) CXX=$(CXX) ../../libraries/dyncall/configure

$(OBJDIR)/dyncall/dyncall/libdyncall_s.a: $(OBJDIR)/dyncall/Makefile.config
	CC=$(CC) CXX=$(CXX) $(MAKE) -C $(OBJDIR)/dyncall/dyncall

gcov:
	rm -f *.gcov
	gcov $(SOURCES) $(SOURCES_C)

init_dir: ttbasic/helptext_en.json
	mkdir -p init_dir/sys/help
	rsync -av $(DEMOS_DIR)/* init_dir/ || true
	rsync -av include/ init_dir/sys/include/
	cp -p ttbasic/eb_*.h init_dir/sys/include/ ; rm -f init_dir/sys/include/eb_*_int.h
	cp -p ttbasic/errdef.h ttbasic/error.h ttbasic/kwenum.h init_dir/sys/include/
	cp -p ttbasic/mcurses*.h init_dir/sys/include/
	cp -p libraries/tinycc/include/*.h init_dir/sys/include/
	cp -p libraries/tinycc/libtcc.h init_dir/sys/include/
	cp -p libraries/stb/*.h init_dir/sys/include/
	cp -p ttbasic/helptext_*.json init_dir/sys/help/
	mkdir -p init_dir/sys/fonts
	cp -p fonts/k8x12w.ttf fonts/misaki_gothic_w.ttf init_dir/sys/fonts/
