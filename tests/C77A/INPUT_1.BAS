  100 REM Purpose: Verify INPUT statment
  110 REM  Author: Howard Wulf, AF5NE
  120 REM
  130 REM
  200 LET N = 0
  210 LET A$ = "123456789012345678901234567890"

  300 PRINT "INPUT B$"
     :INPUT B$
     :GOSUB 8000

  340 PRINT "INPUT ; B$"
     :INPUT ; B$
     :GOSUB 8000

  360 PRINT "INPUT prompt ; B$"
     :INPUT "prompt" ; B$
     :GOSUB 8000

  7999 GOTO 9999
  8000 REM -------------------------------
  8010 REM PRINT RESULTS
  8020 REM -------------------------
  8030 LET N = N + 1
  8040 PRINT "TESTCASE #";N
  8050 PRINT "LENGTH", "[";A$;"]"
  8060 PRINT LEN(B$) , "[";B$;"]"
  8070 PRINT
  8080 RETURN
  9999 END

