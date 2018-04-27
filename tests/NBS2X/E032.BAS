100 REM File: on1.bas
110 REM Purpose: Verify ON ... GOTO|GOSUB ...
120 REM Note:  for index values outside of ramge
130 REM ----------------------------------------------
140 REM option version "BASIC-80"
1000 let i = 0
1010 on i goto 1100,1200,1300,1400,1500,1600,1700,1800,1900
1020 print "*** TEST PASSED ***"
1030  on i gosub 1100,1200,1300,1400,1500,1600,1700,1800,1900
1040 print "*** TEST PASSED ***"
1050 let i = 10
1060 on i goto 1100,1200,1300,1400,1500,1600,1700,1800,1900
1070 print "*** TEST PASSED ***"
1080 on i gosub 1100,1200,1300,1400,1500,1600,1700,1800,1900
1090 print "*** TEST PASSED ***"
1099 goto 2000
1100 ERROR 1
1200 ERROR 1
1300 ERROR 1
1400 ERROR 1
1500 ERROR 1
1600 ERROR 1
1700 ERROR 1
1800 ERROR 1
1900 ERROR 1
2000 for i = 1 to 9
2010 on i goto 2100,2200,2300,2400,2500,2600,2700,2800,2900
2020 print "*** ERROR FOR INDEX="; i
2030 ERROR 1
2040 next i
2050 print "*** TEST PASSED ***"
2099 end
2100 if i = 1 then 2040
2110 error 1
2200 if i = 2 then 2040
2210 error 1
2300 if i = 3 then 2040
2310 error 1
2400 if i = 4 then 2040
2410 error 1
2500 if i = 5 then 2040
2510 error 1
2600 if i = 6 then 2040
2610 error 1
2700 if i = 7 then 2040
2710 error 1
2800 if i = 8 then 2040
2810 error 1
2900 if i = 9 then 2040
2910 error 1
9999 end

