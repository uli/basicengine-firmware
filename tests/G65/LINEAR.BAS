  10 READ A, B, D, E
  15 LET G = A * E - B * D
  20 IF G = 0 THEN 65
  30 READ C, F
  37 LET X = ( C * E - B * F ) / G
  42 LET Y = ( A * F - C * D ) / G
  55 PRINT X, Y
  60 GOTO 30
  65 PRINT "NO UNIQUE SOLUTION"
  70 DATA 1, 2, 4
  80 DATA 2, -7, 5
  85 DATA 1, 3, 4, -7
  90 END
9900 REM ------------------------------------------------------------------------
9910 REM Purpose: LINEAR example 
9920 REM  Author: General Electric
9930 REM    Date: 1965-12-17
9940 REM    Note: OPTION VERSION MARK-I
9950 REM  Origin: 202026A_BASIC_Language_Reference_Manual_Jan1967.pdf
9960 REM    Page: 5
9970 REM ------------------------------------------------------------------------
