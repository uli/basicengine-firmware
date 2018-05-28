# removes line breaks within basicdoc blocks so the blocks can be sorted
s,\n,==LB==,g
s,\(\*\*\*/\)==LB==,\1\n,g
