1000 rem ------------------------------------------
1010 rem Filename: atn1.bas
1020 rem  Purpose: Verify ATN()
1100 rem ------------------------------------------
1120 X = 0
1130 RESULT = ATN( X )
1140 print "The arctangent value of "; X; " is"; RESULT
1150 if RESULT < 0 then goto &TestFailed
1160 if RESULT > 0 then goto &TestFailed
1170 print "*** TEST #1 PASSED ***"
1200 rem ------------------------------------------
1220 X = 0.5
1230 RESULT = ATN( X )
1240 print "The arctangent value of "; X; " is"; RESULT
1250 if RESULT < 0.4636475 then goto &TestFailed
1260 if RESULT > 0.4636477 then goto &TestFailed
1270 print "*** TEST #2 PASSED ***"
1300 rem ------------------------------------------
1320 X = -0.5
1330 RESULT = ATN( X )
1340 print "The arctangent value of "; X; " is"; RESULT
1350 if RESULT > -0.4636475 then goto &TestFailed
1360 if RESULT < -0.4636477 then goto &TestFailed
1370 print "*** TEST #3 PASSED ***"
1400 rem ------------------------------------------
1420 X = 1
1430 RESULT = ATN( X )
1440 print "The arctangent value of "; X; " is"; RESULT
1450 if RESULT < 0.7853981 then goto &TestFailed
1460 if RESULT > 0.7853983 then goto &TestFailed
1470 print "*** TEST #4 PASSED ***"
1500 rem ------------------------------------------
1520 X = -1
1530 RESULT = ATN( X )
1540 print "The arctangent value of "; X; " is"; RESULT
1550 if RESULT > -0.7853981 then goto &TestFailed
1560 if RESULT < -0.7853983 then goto &TestFailed
1570 print "*** TEST #5 PASSED ***"
8000 rem ------------------------------------------
8010 &TestPassed:
8020 print "*** ALL TESTS PASSED ***"
8030 goto &TheEnd
8500 rem ------------------------------------------
8510 &TestFailed:
8520 print "*** TEST FAILED ***"
8530 goto &TheEnd
9000 rem ------------------------------------------
9010 &TheEnd:
9999 end
