1000 rem ------------------------------------------
1010 rem Filename: asc1.bas
1020 rem  Purpose: Verify ASC()
1100 rem ------------------------------------------
1120 X$ = "ABCDEF"
1130 RESULT = ASC( X$ )
1140 print "The ASCII value of "; X$; " is"; RESULT
1150 if RESULT <> 65 then goto &TestFailed
1160 print "*** TEST #1 PASSED ***"
1200 rem ------------------------------------------
1220 X$ = "abcdef"
1230 RESULT = ASC( X$ )
1240 print "The ASCII value of "; X$; " is"; RESULT
1250 if RESULT <> 97 then goto &TestFailed
1260 print "*** TEST #2 PASSED ***"
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
