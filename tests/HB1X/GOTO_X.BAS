100 rem Purpose: Verify GOTO X
110 rem  Author: Howard Wulf, AF5NE
120 rem    Date: 2015-05-22
130 rem    Note: Value in X must be a valid line number
140 rem
150 let X=200
160 goto X
170 print "GOTO X FAILED"
180 stop
190 rem
200 print "GOTO X PASSED"
210 end
