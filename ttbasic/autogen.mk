funtbl.h kwtbl.h: icode.txt icode.py
	python icode.py
epigrams.h: epigrams.txt epi.py
	python epi.py <epigrams.txt >epigrams.h
version.h: ../.git/index
	echo "#define STR_VARSION \"$(shell git describe --abbrev=4 --dirty --always --tags)\"" >$@
helptext.cpp: help.h scripts/help.py $(shell ls basic*.cpp)
	cat *.cpp|sed -f scripts/bdoc_1.sed  |python3 scripts/help.py >helptext.cpp

autogen_clean:
	rm -f *funtbl.h kwtbl.h kwenum.h epigrams.h version.h helptext.cpp
