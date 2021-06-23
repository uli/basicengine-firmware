#!/bin/bash

# defaults for configurable parameters
test -z "$MAKE" && MAKE=make
test -z "$CC" && CC=${CROSS_COMPILE}gcc
test -z "$CXX" && CXX=${CROSS_COMPILE}g++

test -z "$H3_OSDIR" && H3_OSDIR=../../orangepi/allwinner-bare-metal
test -z "$H3_OBJDIR" && H3_OBJDIR=build_h3

test -z "$SDL_OBJDIR" && SDL_OBJDIR=build_sdl

test -z "$INITFS_DIR" && INITFS_DIR=init_dir
test -z "$DEMOS_DIR" && DEMOS_DIR=../basicengine_demos

# functions generating build statements
generate_link() {
	echo -n "build $1: link "
	shift
	objdir="$1"
	shift
	for s in "$@" ; do
		echo -n "$objdir/${s%.*}.o "
	done
	echo "|| $LINK_DEPS"
}

generate_build() {
	rule="$1"
	shift
	objdir="$1"
	shift
	for s in "$@"; do
		echo "build $objdir/${s%.*}.o: $rule $s || $AUTOGEN_DEPS"
	done
}

# (non-English) languages
LANGS="de fr es ja"

# source files common to all builds
COMMON_SOURCES="`echo ttbasic/*.cpp libraries/TTBAS_LIB/*.cpp \
	libraries/ESP8266SAM/src/*.cpp libraries/TTVoutfonts/*.cpp libraries/azip/*.cpp \
	libraries/alpha-lib/src/*.cpp arduino_compat/*.cpp`"

COMMON_SOURCES_C="`echo ttbasic/*.c libraries/stb/*.c libraries/TTBAS_LIB/*.c \
	libraries/atto/*.c` libraries/tinycc/libtcc.c"

# platform-specific source files
H3_SOURCES="$COMMON_SOURCES `ls h3/*.cpp`"
H3_SOURCES_C="$COMMON_SOURCES_C"

SDL_SOURCES="$COMMON_SOURCES `ls sdl/*.cpp`"
SDL_SOURCES_C="$COMMON_SOURCES_C libraries/tinycc/lib/va_list.c"	# XXX: x86-64 only!

# implicit outputs of message file and helptext generators
MSG_IMPLICIT_OUT="ttbasic/msgs_${LANGS// /.h ttbasic/msgs_}.h"
HELPTEXT_IMPLICIT_OUT="ttbasic/helptext_${LANGS// /.json ttbasic/helptext_}.json"

# source files of the former
MSG_DEPS="`echo ttbasic/basic*.cpp libraries/TTBAS_LIB/sdfiles.cpp h3/net.cpp`"
HELPTEXT_DEPS="$MSG_DEPS `echo po/helptext_*.po`"

# implicit outputs of icode table generator
ICODE_IMPLICIT_OUT="ttbasic/kwtbl.h ttbasic/kwenum.h ttbasic/strfuntbl.h ttbasic/numfuntbl.h"

# dependencies that need to be created first
AUTOGEN_DEPS="ttbasic/msgs_en.h $MSG_IMPLICIT_OUT ttbasic/funtbl.h $ICODE_IMPLICIT_OUT ttbasic/epigrams.h ttbasic/version.h"
LINK_DEPS="\$objdir/dyncall/dyncall/libdyncall_s.a"

# ninja file included in all builds
# NB: Make sure $cc, $cxx and $objdir are defined before including this!
cat <<EOT >build.ninja.common
warn_flags = -Wall -Wno-unused -Wno-sign-compare -Wno-implicit-fallthrough -Wno-maybe-uninitialized -Wno-psabi -Wno-format-truncation -Wno-stringop-truncation

common_include = -Ittbasic -Ilibraries/TTVoutfonts -Ilibraries/TTBAS_LIB -Ilibraries/TKeyboard/src \$
  -Ilibraries/ESP8266SAM/src -Ilibraries/azip -Ilibraries/stb \$
  -Ilibraries/alpha-lib/include -Igfx -Ilibraries/dyncall/dyncall \$
  -Ilibraries/tinycc -Iarduino_compat

common_cflags = -g -DENGINEBASIC -D_GNU_SOURCE -fdiagnostics-color=always
common_cxxflags = -fpermissive -std=c++11 -fno-exceptions

common_libs = -lstdc++ -L\$objdir/dyncall/dyncall -ldyncall_s

rule icode
  command = cd ttbasic ; python icode.py

build ttbasic/funtbl.h | $ICODE_IMPLICIT_OUT: icode ttbasic/icode.txt ttbasic/icode.py

rule epigram
  command = python ttbasic/epi.py <\$in >\$out

build ttbasic/epigrams.h: epigram ttbasic/epigrams.txt

rule version
  command = echo "#define STR_VARSION \"\`git describe --abbrev=4 --dirty --always --tags\`\"" >\$out

build ttbasic/version.h: version .git/index

rule msgs
  command = xgettext -k_ -k__ --from-code utf-8 \$in -s -o ttbasic/tmpmsgs.po ; cd ttbasic ; \$
            for i in en $LANGS ; do cat errdef.h tmpmsgs.po | python3 scripts/msgs.py msgs_\$\$i.h \$\$i ; done ; \$
            rm tmpmsgs.po

build ttbasic/msgs_en.h | $MSG_IMPLICIT_OUT: msgs $MSG_DEPS | ttbasic/scripts/msgs.py

rule helptext
  command = cd ttbasic ; for i in en $LANGS ; do cat *.cpp|sed -f scripts/bdoc_1.sed  |python3 scripts/help.py helptext_\$\${i}.json \$\${i} ; done

build ttbasic/helptext_en.json | $HELPTEXT_IMPLICIT_OUT: helptext $HELPTEXT_DEPS | ttbasic/scripts/help.py

rule dyncall_config
  command = mkdir -p \$objdir/dyncall ; cd \$objdir/dyncall ; \$
  CC=\$cc CXX=\$cxx ../../libraries/dyncall/configure

build \$objdir/dyncall/Makefile.config: dyncall_config

rule dyncall_build
  command = CC=\$cc CXX=\$cxx $MAKE -C \$objdir/dyncall/dyncall

build \$objdir/dyncall/dyncall/libdyncall_s.a: dyncall_build || \$objdir/dyncall/Makefile.config

rule initdir
  command = mkdir -p \$out/sys/help && \$
            rsync -av $DEMOS_DIR/* \$out/ || true && \$
            rsync -av include/ \$out/sys/include/ && \$
            mkdir -p \$out/sys/include/tcc && \$
            cp -p ttbasic/eb_*.h \$out/sys/include/ && rm -f \$out/sys/include/eb_*_int.h && \$
            cp -p ttbasic/errdef.h ttbasic/error.h ttbasic/kwenum.h \$out/sys/include/ && \$
            cp -p ttbasic/mcurses*.h \$out/sys/include/ && \$
            cp -p libraries/tinycc/include/*.h \$out/sys/include/tcc/ && \$
            cp -p libraries/tinycc/libtcc.h \$out/sys/include/ && \$
            cp -p libraries/stb/*.h \$out/sys/include/ && \$
            cp -p ttbasic/helptext_*.json \$out/sys/help/ && \$
            mkdir -p \$out/sys/fonts && \$
            cp -p fonts/k8x12w.ttf fonts/misaki_gothic_w.ttf \$out/sys/fonts/ && \$
            rsync -av tests/ \$out/tests/

build $INITFS_DIR: initdir | ttbasic/helptext_en.json $HELPTEXT_IMPLICIT_OUT ttbasic/kwenum.h
EOT

# ninja file for H3 platform
cat <<EOT >build.ninja.h3
include $H3_OSDIR/build.ninja.common

objdir = $H3_OBJDIR
cc = \$aw_cc
cxx = \$aw_cxx

include build.ninja.common

cflags = -O3 \$common_cflags \$warn_flags \$
  -DH3 -Ih3 \$common_include -DENABLE_NEON -DSTBI_NEON \$
  -U__UINT32_TYPE__ -U__INT32_TYPE__ -D__UINT32_TYPE__="unsigned int" -D__INT32_TYPE__=int

cxxflags = \$cflags \$common_cxxflags

libs = \$common_libs

build basic.bin: bin basic.elf
build basic.uimg: uimg basic.bin

default basic.uimg

EOT

{
generate_link basic.elf $H3_OBJDIR $H3_SOURCES $H3_SOURCES_C
generate_build cc $H3_OBJDIR $H3_SOURCES_C
generate_build cxx $H3_OBJDIR $H3_SOURCES
} >>build.ninja.h3

# ninja file for SDL platform
cat <<EOT >build.ninja.sdl
cc = $CC
cxx = $CXX
objdir = $SDL_OBJDIR

include build.ninja.common

cflags = -O3 \$common_cflags \$warn_flags -funroll-loops -fomit-frame-pointer -Isdl \$
  \$common_include -DSDL -DDISABLE_NEON `sdl-config --cflags`
cxxflags = \$cflags \$common_cxxflags

libs = \$common_libs `sdl-config --libs` -lm

rule cc
  depfile = \$out.d
  command = $CC -MD -MF \$out.d \$cflags -c -o \$out \$in

rule cxx
  depfile = \$out.d
  command = $CXX -MD -MF \$out.d \$cxxflags -c -o \$out \$in

rule link
  command = $CC \$in -o \$out \$libs

EOT

{
generate_link basic.sdl $SDL_OBJDIR $SDL_SOURCES $SDL_SOURCES_C
echo "default basic.sdl"
generate_build cc $SDL_OBJDIR $SDL_SOURCES_C
generate_build cxx $SDL_OBJDIR $SDL_SOURCES
} >>build.ninja.sdl

echo "configuration complete"
echo "run \"ninja -f build.ninja.h3\" or \"ninja -f build.ninja.sdl\" to compile"
