#!/bin/bash

# defaults for configurable parameters
test -z "$MAKE" && MAKE=make
test -z "$CC" && CC=${CROSS_COMPILE}gcc
test -z "$CXX" && CXX=${CROSS_COMPILE}g++
test -z "$AR" && AR=${CROSS_COMPILE}ar

test -z "$H3_OSDIR" && H3_OSDIR=../allwinner-bare-metal
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
	echo "| $PLATFORM_LINK_DEPS $LINK_DEPS"
}

generate_build() {
	rule="$1"
	shift
	objdir="$1"
	shift
	for s in "$@"; do
		echo "build $objdir/${s%.*}.o: $rule $s || $AUTOGEN_DEPS"
		# Linux ABI and EABI (allegedly) use differently sized
		# enums; whether that is true or not (it doesn't seem to
		# make much sense to me), the crosstool-ng toolchain is
		# built with -fshort-enums, while the SDL server sends SDL
		# events from Linux with long enums. We therefore have to
		# compile stuff that includes SDL headers with
		# -fno-short-enums, while making sure that it doesn't use
		# any other data structures containing enums... :((
		[[ "$s" =~ "sdl_client" ]] && echo "  cxxflags = \$cxxflags -fno-short-enums"
	done
}

# (non-English) languages
LANGS="de fr es ja"

# source files common to all builds
COMMON_SOURCES="`echo ttbasic/*.cpp libraries/TTBAS_LIB/*.cpp \
	libraries/TTVoutfonts/*.cpp \
	libraries/alpha-lib/src/*.cpp arduino_compat/*.cpp`"

COMMON_SOURCES_C="`echo ttbasic/*.c libraries/stb/*.c` \
	libraries/tinycc/libtcc.c"

# platform-specific source files
H3_SOURCES="$COMMON_SOURCES `ls h3/*.cpp`"
H3_SOURCES_C="$COMMON_SOURCES_C"

if [[ "$CC" =~ mingw ]] ; then
	SDL_PLATFORM=windows
elif [[ "$CC" =~ darwin ]] ; then
	SDL_PLATFORM=osx
else
	SDL_PLATFORM=linux
fi

SDL_SOURCES="$COMMON_SOURCES `ls sdl/*.cpp`"
SDL_SOURCES_C="$COMMON_SOURCES_C"
if test $SDL_PLATFORM == linux ; then
SDL_SOURCES_C="$SDL_SOURCES_C libraries/libsoc/lib/i2c.c \
	libraries/libsoc/lib/spi.c \
	libraries/libsoc/lib/file.c"
fi
	
SDL_SOURCES_C="$SDL_SOURCES_C libraries/tinycc/lib/va_list.c"	# XXX: x86-64 only!

# implicit outputs of message file and helptext generators
MSG_IMPLICIT_OUT="ttbasic/msgs_${LANGS// /.h ttbasic/msgs_}.h"
HELPTEXT_IMPLICIT_OUT="ttbasic/helptext_${LANGS// /.json ttbasic/helptext_}.json"

# source files of the former
MSG_DEPS="`echo ttbasic/*.cpp ttbasic/*.c libraries/TTBAS_LIB/*.cpp h3/*.cpp h3/*.h`"
HELPTEXT_DEPS="$MSG_DEPS `echo po/helptext_*.po`"

# implicit outputs of icode table generator
ICODE_IMPLICIT_OUT="ttbasic/kwtbl.h ttbasic/kwenum.h ttbasic/strfuntbl.h ttbasic/numfuntbl.h"

# dependencies that need to be created first
AUTOGEN_DEPS="ttbasic/msgs_en.h $MSG_IMPLICIT_OUT ttbasic/funtbl.h $ICODE_IMPLICIT_OUT ttbasic/epigrams.h ttbasic/version.h"
LINK_DEPS="\$objdir/dyncall/dyncall/libdyncall_s.a"

WARN_FLAGS="-Wall -Wno-unused"

if [[ "$CC" == *gcc || "$CXX" == g++ || "$CXX" == *-g++ ]]; then
  WARN_FLAGS="$WARN_FLAGS -Wno-sign-compare -Wno-implicit-fallthrough -Wno-maybe-uninitialized -Wno-psabi -Wno-format-truncation -Wno-stringop-truncation"
else
  WARN_FLAGS="$WARN_FLAGS -Wno-c99-designator -Wno-char-subscripts"
  H3_COMPILER_CFLAGS="-DNAME_MAX=255 -I\${aw_sysroot}/include/c++/8.3.0/arm-unknown-eabihf -I\${aw_sysroot}/include/c++/8.3.0"
fi

# ninja file included in all builds
# NB: Make sure $cc, $cxx and $objdir are defined before including this!
cat <<EOT >build.ninja.common
warn_flags = $WARN_FLAGS

common_include = -Ittbasic -Ilibraries/TTVoutfonts -Ilibraries/TTBAS_LIB -Ilibraries/TKeyboard/src \$
  -Ilibraries/stb \$
  -Ilibraries/alpha-lib/include -Igfx -Ilibraries/dyncall/dyncall \$
  -Ilibraries/tinycc -Iarduino_compat

common_cflags = -g -DENGINEBASIC -D_GNU_SOURCE -fdiagnostics-color=always
common_cxxflags = -fpermissive -std=c++11 -fno-exceptions

common_libs = -L\$objdir/dyncall/dyncall -ldyncall_s

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
  CC="\$cc" CXX="\$cxx" AR="$AR" ../../libraries/dyncall/configure

build \$objdir/dyncall/Makefile.config: dyncall_config

rule dyncall_build
  command = CC="\$cc" CXX="\$cxx" $MAKE -C \$objdir/dyncall/dyncall

build \$objdir/dyncall/dyncall/libdyncall_s.a: dyncall_build || \$objdir/dyncall/Makefile.config

rule initdir
  command = mkdir -p \$out/sys/help && \$
            rsync -a $DEMOS_DIR/* \$out/ || true && \$
            rsync -a include/ \$out/sys/include/ && \$
            mkdir -p \$out/sys/include/tcc && \$
            cp -p ttbasic/eb_*.h \$out/sys/include/ && rm -f \$out/sys/include/eb_*_int.h && \$
            cp -p ttbasic/errdef.h ttbasic/error.h ttbasic/kwenum.h \$out/sys/include/ && \$
            cp -p libraries/tinycc/include/*.h \$out/sys/include/tcc/ && \$
            cp -p libraries/tinycc/libtcc.h \$out/sys/include/ && \$
            cp -p libraries/stb/*.h \$out/sys/include/ && \$
            cp -p ttbasic/helptext_*.json \$out/sys/help/ && \$
            mkdir -p \$out/sys/fonts && \$
            cp -p fonts/k8x12w.ttf fonts/misaki_gothic_w.ttf \$out/sys/fonts/ && \$
            cp -p sdl/gamecontrollerdb.txt \$out/sys/ && \$
            rsync -a tests/ \$out/tests/

initfs_dir = $INITFS_DIR
build \$initfs_dir: initdir | ttbasic/helptext_en.json $HELPTEXT_IMPLICIT_OUT ttbasic/kwenum.h
EOT

# ninja file for H3 platform
cat <<EOT >build.ninja.h3
include $H3_OSDIR/build.ninja.common

objdir = $H3_OBJDIR
cc = \$aw_cc
cxx = \$aw_cxx

include build.ninja.common

cflags = $H3_COMPILER_CFLAGS -O3 \$common_cflags \$warn_flags \$
  -DH3 -Ih3 \$common_include -DENABLE_NEON -DSTBI_NEON \$
  -U__UINT32_TYPE__ -U__INT32_TYPE__ -D__UINT32_TYPE__="unsigned int" -D__INT32_TYPE__=int

cxxflags = \$cflags \$common_cxxflags

libs = -lstdc++ \$common_libs

build basic.bin: bin basic.elf
build basic.uimg: uimg basic.bin
build basic_sd.img: sdimg basic.uimg | \$initfs_dir
build upload: upload basic.bin

default basic.uimg

EOT

PLATFORM_LINK_DEPS="$H3_OSDIR/libos.a"
{
generate_link basic.elf $H3_OBJDIR $H3_SOURCES $H3_SOURCES_C
generate_build cc $H3_OBJDIR $H3_SOURCES_C
generate_build cxx $H3_OBJDIR $H3_SOURCES
} >>build.ninja.h3
PLATFORM_LINK_DEPS=

# ninja file for SDL platform

RDYNAMIC="-rdynamic"
test "$SDL_PLATFORM" == windows && RDYNAMIC=""
test "$SDL_PLATFORM" == osx && RDYNAMIC=""

UTIL_LIBS="-lutil -lgpiod -ldl"
test "$SDL_PLATFORM" == windows && UTIL_LIBS="-mconsole"
test "$SDL_PLATFORM" == osx && UTIL_LIBS=""

CXXLIB=stdc++
test "$SDL_PLATFORM" == osx && CXXLIB=c++

SDL2_CONFIG_LIBS="--libs"
test "$SDL_PLATFORM" == windows && SDL2_CONFIG_LIBS="--static-libs"

cat <<EOT >build.ninja.sdl
cc = $CC
cxx = $CXX
objdir = $SDL_OBJDIR

include build.ninja.common

cflags = -O3 \$common_cflags \$warn_flags -funroll-loops -fomit-frame-pointer -Isdl \$
  \$common_include -Ilibraries/libsoc/lib/include -DSDL `sdl2-config --cflags` \
  -fvisibility=hidden
cxxflags = \$cflags \$common_cxxflags

libs = -l${CXXLIB} \$common_libs `sdl2-config $SDL2_CONFIG_LIBS` -lm $UTIL_LIBS

rule cc
  depfile = \$out.d
  command = $CC -MD -MF \$out.d \$cflags -c -o \$out \$in

rule cxx
  depfile = \$out.d
  command = $CXX -MD -MF \$out.d \$cxxflags -c -o \$out \$in

rule link
  command = $CC $RDYNAMIC \$in -o \$out \$libs

EOT

{
generate_link basic.sdl $SDL_OBJDIR $SDL_SOURCES $SDL_SOURCES_C
echo "default basic.sdl"
generate_build cc $SDL_OBJDIR $SDL_SOURCES_C
generate_build cxx $SDL_OBJDIR $SDL_SOURCES
} >>build.ninja.sdl

# ninja files for clang-tidy
cat <<EOT >build.ninja.tidy.sdl
cc = clang-tidy-11
cxx = clang-tidy-11

include build.ninja.common

cflags = -O3 \$common_cflags \$warn_flags -funroll-loops -fomit-frame-pointer -Isdl \$
  \$common_include -Ilibraries/libsoc/lib/include -DSDL `sdl2-config --cflags` -fcolor-diagnostics
cxxflags = \$cflags \$common_cxxflags

libs = \$common_libs `sdl2-config --libs` -lm -lutil -lgpiod

rule cc
  command = \$cc --use-color \$in -- -MD -MF \$out.d \$cflags
  pool = console

rule cxx
  command = \$cxx --use-color \$in -- -MD -MF \$out.d \$cxxflags
  pool = console

rule link
  command = true
EOT

LINK_DEPS=
{
generate_link basic.phony $SDL_OBJDIR $SDL_SOURCES $SDL_SOURCES_C
echo "default basic.phony"
generate_build cc $SDL_OBJDIR $SDL_SOURCES_C
generate_build cxx $SDL_OBJDIR $SDL_SOURCES
} >>build.ninja.tidy.sdl

cat <<EOT >build.ninja.tidy.h3
include $H3_OSDIR/build.ninja.common

objdir = $H3_OBJDIR
cc = clang-tidy-11
cxx = clang-tidy-11

include build.ninja.common

cflags = --sysroot \$aw_sysroot -O3 \$common_cflags \$warn_flags \$
  -DH3 -Ih3 \$common_include -DENABLE_NEON -DSTBI_NEON \$
  -U__UINT32_TYPE__ -U__INT32_TYPE__ -D__UINT32_TYPE__="unsigned int" -D__INT32_TYPE__=int -D__arm__ -DNAME_MAX=255

cxxflags = \$cflags \$common_cxxflags -I/usr/include/c++/8

libs = \$common_libs

rule tcxx
  command = \$cxx --use-color \$in -- -MD -MF \$out.d \$aw_cxxflags \$cxxflags
  #pool = console

rule tcc
  command = \$cc --use-color \$in -- -MD -MF \$out.d \$aw_cflags \$cflags
  #pool = console

EOT

PLATFORM_LINK_DEPS=
{
generate_link basic.phony $H3_OBJDIR $H3_SOURCES $H3_SOURCES_C
echo "default basic.phony"
generate_build tcc $H3_OBJDIR $H3_SOURCES_C
generate_build tcxx $H3_OBJDIR $H3_SOURCES
} >>build.ninja.tidy.h3


echo "configuration complete"
echo "run \"ninja -f build.ninja.h3\" or \"ninja -f build.ninja.sdl\" to compile"
