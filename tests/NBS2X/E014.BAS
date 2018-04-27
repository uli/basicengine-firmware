1000 rem ------------------------------------------
1010 rem Filename: clear1.bas
1020 rem  Purpose: Verify CLEAR
1030 rem ------------------------------------------
1100 rem Assign Values
1110 'COMMON A, B$
1120 A = 1.2345
1130 B$ = "abcdef"
1140 Y = 6.789
1150 Z$ = "wxyz"
1160 CLEAR
1200 rem Display values
1210 print "The value of A  is";A
1220 print "The value of B$ is";B$
1230 print "The value of Y  is";Y
1250 print "The value of Z$ is";Z$
1300 rem Verify values
1310 if A  <> 0  then 
1311 print "A is not zero"
1312 goto &TestFailed
1313 end if
1320 if B$ <> "" then 
1321 print "B$ is not empty"
1322 goto &TestFailed
1323 end if
1330 if Y  <> 0  then 
1331 print "Y is not zero"
1332 goto &TestFailed
1333 end if
1340 if Z$ <> "" then 
1341 print "Z$ is not empty"
1342 goto &TestFailed
1343 end if
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
