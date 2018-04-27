  5 PRINT "X VALUE","SINE","RESOLUTION"
 10 READ D
 20 LET M= -1
 30 FOR X = 0 TO 3 STEP D
 40   IF SIN(X) <= M THEN 80
 50   LET X0=X
 60   LET M = SIN(X)
 80 NEXT X
 85 PRINT X0, M, D
 90 GOTO 10
100 DATA .1, .01, .001
110 END
9900 REM ------------------------------------------------------------------------
9910 REM Purpose: MAXSIN example 
9920 REM  Author: General Electric
9930 REM    Date: 1965-12-17
9940 REM    Note: OPTION VERSION MARK-I
9950 REM  Origin: 202026A_BASIC_Language_Reference_Manual_Jan1967.pdf
9960 REM    Page: 18
9970 REM ------------------------------------------------------------------------
