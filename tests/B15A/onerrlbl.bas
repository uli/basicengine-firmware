rem onerrlbl.bas -- test bwBASIC ON ERROR GOSUB statement with label
print "Test bwBASIC ON ERROR GOSUB statement"
on error goto &handler
print "The next line will include an error"
if d$ = 78.98 then print "This should not print"
print "This is the line after the error"
end
&handler:
print "This is the error handler"
print "The error number is ";ret(0)
print "The error line   is ";ret(1)
resume
