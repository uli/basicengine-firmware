function import_def {
	val="`grep -h "#define\s*${1}\s*" *.h |head -n1|sed "s,#define\s*${1}\s*,,g"`"
	echo ":$1: ${val}" >>$REFDOC
	echo ":$1_m1: $((val-1))" >>$REFDOC
}

REFDOC=../doc/reference.adoc
rm -f $REFDOC

import_def MAX_SPRITES
import_def MAX_BG
import_def MAX_SPRITE_W
import_def MAX_SPRITE_H
import_def CSP_NUM_COLORSPACES
import_def NUM_FONTS
import_def MAX_RETVALS
import_def MAX_USER_FILES
import_def SOUND_CHANNELS
import_def CONFIG_COLS
import_def MAX_PADS
echo ":max_err: $((`wc -l <errdef.h`-1))" >>$REFDOC

cat scripts/groups.txt|while read grp desc; do
  cat basic*.cpp net.cpp | sed -nf scripts/bdoc_1.sed | sed -zf scripts/bdoc_1a.sed |
  	  grep "/\*\*\*b[a-z]\s*${grp}\s*"|
          sort|sed -f scripts/bdoc_1b.sed|sed -f scripts/bdoc_2.sed >../doc/reference_${grp}.adoc
  { echo "=== $desc"
    echo ":numbered!:"
    echo "include::reference_${grp}.adoc[]"
    echo ":numbered:"
    echo
  } >>../doc/reference.adoc
done

asciidoctor -a stylesheet=../doc/asciidoctor.css ../doc/manual.adoc
