  100 REM Purpose: Verify CHAIN program$, parameter$
  120 REM  Author: Howard Wulf, AF5NE
  130 REM    Date: 2015-12-03
  140 REM    Note: OPTION VERSION SYSTEM-360
  150 REM  
  200 print "This is is calling program"
  210 let X$ = "some parameter value"
  220 chain "I70/USE.BAS":rem , X$
  230 print "Still in the calling program"
  999 end
