/\\args.*$/,/\(\\[a-z]\|\*\*\*\)/s,^\(\\[b-z].*\|\*\*\*/\),|===\n\1,	# terminate \args table
#s,/\*\*\*bc\s*,==== ,			# command start
s,/\*\*\*bc\s*\S*\s*\(.*\),[reftext=\1]\n==== [blue]#\1# ,	# command start
/\\tok\s*/d				# delete \tok (used for online help)
#/^$/,/\\/ s/^$/_DELETE/
#/_DELETE/d
s,/\*\*\*bf\s*\S*\s*\(\S*\)\s*,[reftext=\1()]\n==== [green]#\1()# ,	# function start
s,/\*\*\*bn\s*\S*\s*\(\S*\)\s*,[reftext=\1]\n==== [purple]#\1# ,	# constant start
# most \usage processing is done in bdoc_1a.sed
s,\\usage\s*\(\S.*\)$,\n===== USAGE\n----\n\1\n----,	# monotype \usage tag and value in single line
s,\\usage\s*,\n===== USAGE,		# \usage
s,\\desc\s*,\n===== DESCRIPTION,	# \desc
s,\\args\s*,\n===== PARAMETERS\n|===,	# \args
s,@\(\S*\)\s*,| `\1` | ,		# argument
s,\\ret\s*\(\S\)*$,\n===== RETURN VALUE\n\1\n,
s,\\ret\s*,\n===== RETURN VALUE\n,	# \ret
s,\\bugs\s*,\n===== BUGS,		# \bugs
s,\\note\s*,\n===== NOTE,		# \note
s,\\sec\s*\(.*\)$,\n===== \1\n,		# \sec <custom section>
/\\ref/s,\([A-Z0-9]\S*\),<<\1>>\,,g	# link \ref items
/\\ref/s/,$//				# remove extraneous comma at end of \ref
/\\ref/s/_/ /g				# replace underscores with spaces in \ref
s,\\ref\s*,\n===== SEE ALSO\n,		# \ref
s,\*\*\*/$,,				# end of block
