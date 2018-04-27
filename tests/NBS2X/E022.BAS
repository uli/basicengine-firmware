
10 rem DATA1.BAS -- Test DATA, READ, and RESTORE Statements 
20 print "DATA1.BAS -- Test DATA, READ, and RESTORE Statements" 
30 DATA "Ted", 56.789 
40 REM just to see if it advances correctly 
50 DATA "Dale", 45.678 
	60 READ N$, NUMBER, ANOTHER$ 
        61 if N$ <> "Ted" then goto &TestFailed
        62 if NUMBER <> 56.789 then goto &TestFailed
        63 if ANOTHER$ <> "Dale" then goto &TestFailed
	70 READ ANUMBER 
        71 if ANUMBER <> 45.678 then goto &TestFailed
	80 PRINT "Data read: ";N$;" ";NUMBER;" ";ANOTHER$;" ";ANUMBER 
90 RESTORE 30 
	100 READ ANOTHER$ 
        101 if ANOTHER$ <> "Ted" then goto &TestFailed
	110 READ ANUMBER, N$,NUMBER 
        111 if ANUMBER <> 56.789 then goto &TestFailed
        112 if N$ <> "Dale" then goto &TestFailed
        113 if NUMBER <> 45.678 then goto &TestFailed
	120 PRINT "After RESTORE:" 
	130 PRINT "Data read: ";ANOTHER$;" ";ANUMBER;" ";N$;" ";NUMBER 
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

