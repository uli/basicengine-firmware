  100 rem Test ERROR 73
  110 on error goto 200
  120 out 1, 2
  130 print "test failed"
  140 goto 999
  200 print "ERL="; RET(1); ", ERR="; RET(0); ", ERR$="; ERROR$(RET(0))
  999 end

