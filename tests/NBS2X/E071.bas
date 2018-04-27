100 REM Purpose: Verify =>, =<, and ><
110 REM  Author: Howard Wulf, AF5NE
120 REM    Date: 2015-05-09
130 REM    Note: These are non-standard comparison operators
140 REM
150 REM ----------------------------------------------------------

1100 print "positive test of <="
1110 let A = 1
1120 let B = 2
1130 if A <= B then 1160
1140 print "failed"
1150 stop
1160 print "passed"

3100 print "positive test of >="
3110 let A = 2
3120 let B = 1
3130 if A >= B then 3160
3140 print "failed"
3150 stop
3160 print "passed"

5100 print "positive test of <>"
5110 let A = 2
5120 let B = 1
5130 if A <> B then 5160
5140 print "failed"
5150 stop
5160 print "passed"

6100 print "positive test of ><"
6110 let A = 2
6120 let B = 1
6130 if A >< B then 6160
6140 print "failed"
6150 stop
6160 print "passed"

9999 END
