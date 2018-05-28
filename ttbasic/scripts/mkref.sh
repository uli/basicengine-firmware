rm -f ../doc/reference.adoc
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
