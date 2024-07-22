default: all
.PHONY: all clean

TOP = ../../..

include ${TOP}/Makefile.config

ifeq ("${CONFIG_OS}","iphoneos")
DIFFSCRIPT=diff-iphoneos.sh 
else
DIFFSCRIPT=diff-generic.sh
endif

all:
	sh ${DIFFSCRIPT}
clean:
	rm -Rf out

