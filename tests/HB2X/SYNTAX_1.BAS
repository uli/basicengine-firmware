100 REM Purpose: Verify 'Syntax error' with active error handler
110 REM  Author: Howard Wulf, AF5NE
120 REM    Date: 2016-01-20
130 REM    Note: this is to verify the WARN_xxx macros
140 REM
200 ON ERROR GOTO 300
210 LET A
220 PRINT "TEST FAILED"
230 GOTO 999
300 PRINT "ERR=";RET(0),"ERL=";RET(1)
999 END

