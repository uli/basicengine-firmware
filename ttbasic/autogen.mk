funtbl.h kwtbl.h: icode.txt icode.py
	python icode.py
epigrams.h: epigrams.txt epi.py
	python epi.py <epigrams.txt >epigrams.h
version.h: ../.git/index
	echo "#define STR_VARSION \"$(shell git describe --abbrev=4 --dirty --always --tags)\"" >$@

autogen_clean:
	rm -f *funtbl.h kwtbl.h kwenum.h epigrams.h version.h
