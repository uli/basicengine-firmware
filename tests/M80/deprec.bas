10 REM  DEPRECIATION PROGRAM
20 DIM I$(25), DP$(8), Z$(3), D(10)
30 PRINT
40 PRINT "  TAX DEPRECIATION"
50 PRINT
60 REM *** INPUT DATA ***
70 INPUT "NAME OF ITEM"; I$
80 INPUT "ENTER TODAYS DATE (MM/DD/YY)"; DP$
90 INPUT "PURCHASE DATE (MM/DD/YY)"; D$
100 INPUT "COST OF ITEM"; C
110 INPUT "USEFUL LIFE"; L
120 PRINT "DEPRECIATION RATE"
121 INPUT "(NEW=200%; USED=150%)"; R
130 REM *** CALCULATE YEARLY RATE ***
140 YR = R / (100 * L)
150 REM *** CALCULATE EXTRA FIRST YR DEPRECIATION ***
160 IF L >= 6 THEN ED = .2 * C
165 IF L < 6 THEN ED = 0
170 CV = C - ED
180 REM *** CALCULATE INVESTMENT CREDIT ***
190 IF L < 3 THEN IC = 0
200 IF L >= 3 THEN IC = C / 30
210 IF L >= 5 THEN IC = C / 15
220 IF L >= 7 THEN IC = C / 10
230 MM = VAL ( LEFT$ (D$,2))
240 DD = VAL ( MID$ (D$,4,2))
250 YY = VAL ( RIGHT$ (D$,2))
260 IF DD <= 15 THEN FY = 13 - MM
270 IF DD > 15 THEN FY = 12 - MM
275 LY = 12 - FY
280 REM *** FIRST YEAR DEPRECIATION ***
290 CY = 1
300 D(CY) = CV * YR * (FY / 12)
310 CV = CV - D(CY)
320 REM *** MIDDLE YEARS DEPRECIATION ***
330 FOR CY = 2 TO L
340   D(CY) = YR * CV
350   CV = CV - D(CY)
360 NEXT 
370 REM *** LAST YEAR DEPRECIATION ***
380 IF CY = 0 THEN 410
385 CY = L + 1
390 D(CY) = YR * CV * (LY / 12)
400 CV = CV - D(CY)
410 REM *** OUTPUT TO SCREEN ***
420 PRINT : PRINT
430 PRINT TAB(6);"DEPRECIATION ANALYSIS FOR ";I$
440 PRINT TAB(10);"DATE PREPARED; ",DP$
450 PRINT
460 PRINT "ITEM NAME: "; TAB(20);I$
470 PRINT "DATE OF PURCHASE: "; TAB(20); D$
480 PRINT "COST: "; TAB(17);
481 PRINT USING "$$#,###.##"; C
490 PRINT "USEFUL LIFE: "; TAB(20); L; "YEARS"
500 PRINT "DEPRECIATION RATE: "; TAB(20); R; "%"
510 PRINT
520 CY = 1
530 Y = 1899 + YY
540 INPUT "PRESS RETURN TO CONTINUE "; Z$
550 PRINT
560 PRINT " YEAR"; TAB(10);"INVESTMENT CREDIT";TAB(30);"EXTRA FIRST YEAR DEPRECIATION"
570 PRINT Y + CY; TAB(10);
571 PRINT USING "###.##";IC;
572 PRINT TAB(30);
573 PRINT USING "###.##";ED
580 PRINT
590 PRINT " YEAR"; TAB(10); "DEPRECIATION"
600 IF LY <> 0 THEN 620
610 FOR CY = 1 TO L: GOTO 630
630   PRINT Y + CY; TAB(9);
631   PRINT USING "####.##"; D(CY)
640 NEXT
650 IF D(CY) <= 0 THEN 670
660 PRINT
670 PRINT "SALVAGE VALUE AT END OF";CY+Y-1;"IS";
671 PRINT USING "$####.##"; CV
680 REM *** PRINTOUT ROUTINE GOES HERE ***
