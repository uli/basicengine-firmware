   10 REM Purpose:  Verify 'TIM(X)' is elapsed execution time
   20 REM  Author:  Howard Wulf, AF5NE
   30 REM    Date:  2015-10-21
   40 REM    Note:  OPTION VERSION MARK-I
   50 REM    Note:  The execution speed is implementation defined
  100 REM +---------------------------------------------------------------------+
  110 REM |                                                                     |
  120 REM |  Bywater BASIC is slow, so five million loops take about 5 seconds  |
  130 REM |                                                                     |
  140 REM +---------------------------------------------------------------------+
151 t=TICK()
  200 for i = 1 to 5E6
  210 next i
  300 PRINT "On my system, this should be between 4 and 6 seconds: ";int((TICK()-t)/1000)
  999 END
