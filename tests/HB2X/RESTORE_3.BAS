   10 REM Purpose: Verify "RESTORE label"
   20 REM  Author: Howard Wulf, AF5NE
   30 REM    Date: 2015-10-21
   40 REM    Note: OPTION VERSION BYWATER
   50 REM    Note: OPTION LABELS ON
   60 REM   
  100 restore &fred
  110 read a$
  120 print a$
  200 data "error"
  210 data "error"
  220 data "error"
  230 &fred:
  240 data "ok"
  250 data "error"
  260 data "error"
  299 end
