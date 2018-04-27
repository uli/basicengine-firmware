  200 PRINT "I", "HEX$(I)", "BIN$(I)"
  210 FOR I = -3 TO 3
  220   PRINT I, HEX$(I), BIN$(I)
  230 NEXT I
235 END : rem rest is unimplemented
  300 PRINT "I", "HEX$(I)", "BIN$(I,I)"
  310 FOR I = 0 TO 3
  320   PRINT I, HEX$(I), BIN$(I,I)
  330 NEXT I
  400 PRINT "I", "BIN$(1,I)"
  410 FOR I = 0 TO 35
  420   PRINT I, BIN$(1,I)
  430 NEXT I
  500 PRINT "I", "BIN$(-1,I)"
  510 FOR I = 0 TO 35
  520   PRINT I, BIN$(-1,I)
  530 NEXT I
  999 END

