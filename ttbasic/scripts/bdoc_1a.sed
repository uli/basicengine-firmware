# removes line breaks within basicdoc blocks so the blocks can be sorted
s,\n,==LB==,g
s,\(\*\*\*/\)==LB==,\1\n,g
# \usage: end/start code block when encountering two LFs
s,\(\\usage\s*==LB==[^\]*\)==LB====LB==,\1==LB==----==LB==----==LB==,g
# \usage: start new code block at the beginning of the section
s,\(\\usage\s*==LB==\),\1----==LB==,g
# \usage: terminate code block when end of bdoc block is reached
s,\(\\usage\s*==LB==[^\]*\)\(\*\*\*/\),\1----==LB==\2,g
# \usage: terminate code block when end of section is reached
s,\(\\usage\s*==LB==[^\]*\)\(\\[a-z]\),\1----==LB==\2,g
