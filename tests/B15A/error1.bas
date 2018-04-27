  100 rem test
  110 on error goto 200
  120 for x == 1 to 10
  130   print x
  140 next x
  150 goto 999
  200 print "ERL="; RET(1); ",ERR="; RET(0); ", ERR$="; ERROR$(RET(0))
  999 end
