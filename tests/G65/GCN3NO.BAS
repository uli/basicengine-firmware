  10 PRINT " A", " B", " C", "GCD"
  20 READ A, B, C
  30 LET X = A
  40 LET Y = B
  50 GOSUB 200
  60 LET X = G
  70 LET Y = C
  80 GOSUB 200
  90 PRINT A, B, C, G
 100 GOTO 20
 110 DATA 60, 90, 120
 120 DATA 38465, 64872, 98765
 130 DATA 32, 384, 72
 200 LET Q = INT(X/Y)
 210 LET R = X - Q*Y
 220 IF R = 0 THEN 300
 230 LET X = Y
 240 LET Y = R
 250 GOTO 200
 300 LET G = Y
 310 RETURN
 320 END
9900 REM ------------------------------------------------------------------------
9910 REM Purpose: GCN3NO example 
9920 REM  Author: General Electric
9930 REM    Date: 1965-12-20
9940 REM    Note: OPTION VERSION MARK-I
9950 REM  Origin: 202026A_BASIC_Language_Reference_Manual_Jan1967.pdf
9960 REM    Page: 29
9970 REM ------------------------------------------------------------------------
