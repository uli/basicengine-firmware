1000 rem ------------------------------------------
1010 rem Filename: chr1.bas
1020 rem  Purpose: Verify CHR()
1100 rem ------------------------------------------
1110 let R$ = ""
1120 for I = 0 to 255
1130   let A$ = CHR$( I )
1140   let B = ASC( A$ )
1150   if B <> I then goto &TestFailed
1160   print "*** TEST #"; I ; " PASSED ***"
1180 next I
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

