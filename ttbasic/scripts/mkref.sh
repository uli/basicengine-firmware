function import_def {
	val="`grep -h "#define\s*${1}\s*" *.h |sed "s,#define\s*${1}\s*,,g"`"
	echo ":$1: ${val}" >>$REFDOC
	echo ":$1_m1: $((val-1))" >>$REFDOC
}

REFDOC=../doc/reference.adoc
rm -f $REFDOC

import_def VS23_MAX_SPRITES
import_def VS23_MAX_BG
import_def VS23_MAX_SPRITE_W
import_def VS23_MAX_SPRITE_H
import_def VS23_NUM_COLORSPACES

cat scripts/groups.txt|while read grp desc; do
  sed -nf scripts/bdoc_1.sed <basic.cpp|sed -zf scripts/bdoc_1a.sed|
  	  grep "/\*\*\*b[a-z]\s*${grp}\s*"|
          sort|sed -f scripts/bdoc_1b.sed|sed -f scripts/bdoc_2.sed >../doc/reference_${grp}.adoc
  { echo "=== $desc"
    echo ":numbered!:"
    echo "include::reference_${grp}.adoc[]"
    echo ":numbered:"
    echo
  } >>../doc/reference.adoc
done

asciidoctor ../doc/manual.adoc
