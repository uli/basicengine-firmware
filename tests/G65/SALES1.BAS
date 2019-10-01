5 DIM P(3)
6 DIM S(3,5)
  10 FOR I = 1 TO 3
  20   READ P(I)
  30 NEXT I
  40 FOR I = 1 TO 3
  50   FOR J = 1 TO 5
  60     READ S(I,J)
  70   NEXT J
  80 NEXT I
  90 FOR J = 1 TO 5
 100   LET S = 0
 110   FOR I = 1 TO 3
 120     LET S = S + P(I) * S(I,J)
 130   NEXT I
 140   PRINT "TOTAL SALES FOR SALESMAN ";J,"$"; S
 150 NEXT J
 900 DATA 1.25, 4.30, 2.50
 910 DATA 40, 20, 37, 29, 42
 920 DATA 10, 16, 3, 21, 8
 930 DATA 35, 47, 29, 16, 33
 999 END
9900 REM ------------------------------------------------------------------------
9910 REM Purpose: SALES1 example 
9920 REM  Author: General Electric
9930 REM    Date: 1965-12-17
9940 REM    Note: OPTION VERSION MARK-I
9950 REM  Origin: 202026A_BASIC_Language_Reference_Manual_Jan1967.pdf
9960 REM    Page: 12
9970 REM ------------------------------------------------------------------------
