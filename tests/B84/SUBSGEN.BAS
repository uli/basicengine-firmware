  100 REM Filename: SUBSGEN.BAS
  110 REM  Purpose: Generate sub-string assignment tests
  120 REM   Author: Howarsd Wulf, AF5NE
  130 REM     Date: 2017-06-23
  140 REM    Notes: OPTION VERSION "BYWATER"
  150 REM
  160 REM
  170 REM
  180 REM
  190 LET Q$ = CHR$(34)
  200 REM --------------------------------------------------
  210 LET L = 1000
  220 LET A$ = "ABCDEFG"
  230 LET B$ = "WXYZ"
  240 GOSUB 900
  300 REM --------------------------------------------------
  310 LET L = 3000
  320 LET A$ = "ABCDEFG"
  330 LET B$ = ""
  340 GOSUB 900
  400 REM --------------------------------------------------
  410 LET L = 5000
  420 LET A$ = ""
  430 LET B$ = "WXYZ"
  440 GOSUB 900
  500 REM --------------------------------------------------
  510 LET L = 7000
  520 LET A$ = ""
  530 LET B$ = ""
  540 GOSUB 900
  600 REM --------------------------------------------------
  610 LET L = 7500
  620 LET A$ = "A"
  630 LET B$ = "W"
  640 GOSUB 900
  700 REM --------------------------------------------------
  710 LET L = 8000
  720 LET A$ = "A"
  730 LET B$ = ""
  740 GOSUB 900
  800 REM --------------------------------------------------
  810 LET L = 8500
  820 LET A$ = ""
  830 LET B$ = "W"
  840 GOSUB 900
  850 REM --------------------------------------------------
  860 GOTO 999
  900 REM --------------------------------------------------
  910 LET A = LEN( A$ )
  920 FOR M = -2 TO A + 2
  930   FOR N = -2 TO A + 2
  940     PRINT L; " DATA "; Q$; A$; Q$; ","; M; ","; N; ","; Q$; B$; Q$
  950     LET L = L + 10
  960   NEXT N
  970 NEXT M
  980 RETURN
  990 REM --------------------------------------------------
  999 END

