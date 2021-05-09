funtbl.h kwtbl.h: icode.txt icode.py
	python icode.py
epigrams.h: epigrams.txt epi.py
	python epi.py <epigrams.txt >epigrams.h
version.h: ../.git/index
	echo "#define STR_VARSION \"$(shell git describe --abbrev=4 --dirty --always --tags)\"" >$@
msgs_de.h: scripts/msgs.py $(shell ls basic*.cpp ../libraries/TTBAS_LIB/sdfiles.cpp ../h3/net.cpp)
	xgettext -k_ -k__ --from-code utf-8 basic.cpp basic_*.cpp ../libraries/TTBAS_LIB/sdfiles.cpp ../h3/net.cpp -s -o tmpmsgs.po
	for i in en de fr es ; do cat errdef.h tmpmsgs.po | python3 scripts/msgs.py msgs_$$i.h $$i ; done
	rm tmpmsgs.po
helptext_en.h: help.h scripts/help.py $(shell ls basic*.cpp) $(shell ls ../po/helptext_*.po)
	for i in en de fr es ; do cat *.cpp|sed -f scripts/bdoc_1.sed  |python3 scripts/help.py helptext_$${i}.h $${i} ; done

autogen_clean:
	rm -f *funtbl.h kwtbl.h kwenum.h epigrams.h version.h helptext_??.h msgs_??.h
