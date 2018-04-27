REM ------------------------------------------------------------
REM   PURPOSE: TEST THE PARAMETER RANGE CHECKS
REM    AUTHOR: HOWARD WULF, AF5NE
REM GENERATED: 02/21/2015 14:08:36
REM  FILENAME: F_VAL_A_N.BAS
REM    SYNTAX: N  = VAL( A$ )
REM  UNIQUEID: F_VAL_A_N
REM  FUNCTION: VAL
REM 
REM	VARIABLE	DESCRIPTION
REM	I	CURRENT  TESTCASE NUMBER
REM	E	EXPECTED ERROR FLAG
REM	F	ACTUAL   ERROR FLAG
REM	R	EXPECTED FUNCTION RESULT
REM	N	ACTUAL   FUNCTION RESULT
REM	A$	ACTUAL   FUNCTION PARAMETER
REM 
RESTORE 2000
REM GET TESTCASE #
READ  I
WHILE I > 0
REM GET TESTCASE DATA
READ  E, R, A$
REM DISPLAY TESTCASE DATA
PRINT "TESTCASE #";I,"E=";E,"R=";R, "A$=[";A$;"]"
REM EXECUTE TESTCASE
N =  0 
S$ = ""
ON ERROR GOTO 9000
F = 0
999 N  = VAL( A$ )
ERROR 0
REM DISPLAY TESTCASE RESULTS
PRINT ,"F=";F,"N=";N
REM VERIFY EXPECTED ERROR
IF E <> F THEN
  PRINT "*** TEST FAILED, EXCEPTION MISMATCH ***"
  STOP
END IF
REM VERIFY EXPECTED RESULT
IF R > 0 THEN
  LET RMIN = 0.99 * R
  IF R > 1.79769e+308 / 1.012 THEN
    LET RMAX = 1.79769e+308
  ELSE
    LET RMAX = 1.01 * R
  END IF
END IF
IF R < 0 THEN
  IF R < - 1.79769e+308 / 1.012 THEN
    LET RMIN = -1.79769e+308
  ELSE
    LET RMIN = 1.01 * R
  END IF
  LET RMAX = 0.99 * R
END IF
IF R = 0 THEN
  LET RMIN = -.01
  LET RMAX = +.01
END IF
IF N < RMIN THEN
  PRINT "*** TEST FAILED, RESULT MISMATCH ***"
  STOP
END IF
IF N > RMAX THEN
  PRINT "*** TEST FAILED, RESULT MISMATCH ***"
  STOP
END IF
PRINT "*** TEST PASSED ***"
REM GET NEXT TESTCASE #
READ  I
WEND
PRINT "*** ALL TESTS PASSED ***"
GOTO 9999
1999 REM TESTCASE, EXPECTED ERROR, EXPECTED RESULT, PARAMETERS...
DATA  1 ,  0 ,  0 , ""
DATA  2 ,  0 ,  0 , " "
DATA  3 ,  0 ,  3 , "3"
DATA  4 ,  0 ,  0 , "z"
DATA  5 ,  0 ,  0 , "  "
DATA  6 ,  0 ,  3 , " 3"
DATA  7 ,  0 ,  0 , " z"
DATA  8 ,  0 ,  3 , "3 "
DATA  9 ,  0 ,  0 , "z "
DATA  10 ,  0 ,  3 , " 3 "
DATA  11 ,  0 ,  0 , " z "
DATA  12 ,  0 ,  0 , " LLLL"
DATA  13 ,  0 ,  0 , "    ''    "
DATA  14 ,  0 ,  0 , "   oo     "
DATA  15 ,  0 ,  0 , " 00  "
DATA  16 ,  0 ,  0 , "   \\\\  "
DATA  17 ,  0 ,  0 , " lllll  "
DATA  18 ,  0 ,  0 , "    uuuu  "
DATA  19 ,  0 ,  0 , "  a  "
DATA  20 ,  0 ,  0 , "       "
DATA  21 ,  0 ,  0 , " hh     "
DATA  22 ,  0 ,  0 , "  h   "
DATA  23 ,  0 ,  0 , " )"
DATA  24 ,  0 ,  5 , "    5   "
DATA  25 ,  0 ,  2 , "  2 "
DATA  26 ,  0 ,  0 , " -   "
DATA  27 ,  0 ,  0 , "dd "
DATA  28 ,  0 ,  0 , "  tttt   "
DATA  29 ,  0 ,  0 , "  ::::   "
DATA  30 ,  0 ,  0 , "   ___   "
DATA  31 ,  0 ,  0 , "     "
DATA  32 ,  0 ,  0 , "       "
DATA  33 ,  0 ,  0 , " _____  "
DATA  34 ,  0 ,  0 , " S "
DATA  35 ,  0 ,  0 , "  FFFF "
DATA  36 ,  0 ,  0 , "    kkk"
DATA  37 ,  0 ,  0 , "    NNNN  "
DATA  38 ,  0 ,  0 , "   mmmm  "
DATA  39 ,  0 ,  0 , "    ,,,,, "
DATA  40 ,  0 ,  0 , "OO   "
DATA  41 ,  0 ,  0 , "   HHHH  "
DATA  42 ,  0 ,  0 , " < "
DATA  43 ,  0 ,  0 , "   j    "
DATA  44 ,  0 ,  0 , "    yyyy    "
DATA  45 ,  0 ,  0 , "  +++ "
DATA  46 ,  0 ,  0 , "o  "
DATA  47 ,  0 ,  0 , "    (("
DATA  48 ,  0 ,  0 , "     uu     "
DATA  49 ,  0 ,  0 , "    00     "
DATA  50 ,  0 ,  0 , "     HH  "
DATA  51 ,  0 ,  0 , "     "
DATA  52 ,  0 ,  0 , "  G   "
DATA  53 ,  0 ,  0 , "    {{{{ "
DATA  54 ,  0 ,  0 , "    ]]]     "
DATA  55 ,  0 ,  0 , "CCC"
DATA  56 ,  0 ,  44 , "     44  "
DATA  57 ,  0 ,  0 , "^^  "
DATA  58 ,  0 ,  0 , "    QQ    "
DATA  59 ,  0 ,  0 , "     FFFF  "
DATA  60 ,  0 ,  0 , " n   "
DATA  61 ,  0 ,  0 , "    hh  "
DATA  62 ,  0 ,  555 , "555    "
DATA  63 ,  0 ,  0 , "    YYYY "
DATA  64 ,  0 ,  0 , "    w     "
DATA  65 ,  0 ,  0 , "MMMM "
DATA  66 ,  0 ,  0 , " JJJJ    "
DATA  67 ,  0 ,  0 , "    %%    "
DATA  68 ,  0 ,  0 , "     *  "
DATA  69 ,  0 ,  0 , "    @@@   "
DATA  70 ,  0 ,  0 , "I "
DATA  71 ,  0 ,  0 , "    ss  "
DATA  72 ,  0 ,  0 , "  f "
DATA  73 ,  0 ,  0 , "   D  "
DATA  74 ,  0 ,  0 , "###    "
DATA  75 ,  0 ,  0 , "    '     "
DATA  76 ,  0 ,  0 , "  dd    "
DATA  77 ,  0 ,  0 , " TT    "
DATA  78 ,  0 ,  0 , "  vvv "
DATA  79 ,  0 ,  0 , "    X "
DATA  80 ,  0 ,  0 , "    %   "
DATA  81 ,  0 ,  0 , "  "
DATA  82 ,  0 ,  5555 , " 5555   "
DATA  83 ,  0 ,  0 , "FFFF   "
DATA  84 ,  0 ,  0 , "     %%  "
DATA  85 ,  0 ,  0 , "     "
DATA  86 ,  0 ,  0 , "  ~~~~    "
DATA  87 ,  0 ,  0 , "   ||    "
DATA  88 ,  0 ,  0 , "     SSS     "
DATA  89 ,  0 ,  6666 , "   6666 "
DATA  90 ,  0 ,  0 , "    ww   "
DATA  91 ,  0 ,  1111 , "  1111   "
DATA  92 ,  0 ,  0 , " K "
DATA  93 ,  0 ,  0 , "  ttttt "
DATA  94 ,  0 ,  0 , " OOOOO   "
DATA  95 ,  0 ,  0 , " E  "
DATA  96 ,  0 ,  0 , " PPPPP   "
DATA  97 ,  0 ,  0 , "    == "
DATA  98 ,  0 ,  0 , " VVVV    "
DATA  99 ,  0 ,  0 , " c   "
DATA  100 ,  0 ,  0 , "    yyyy     "
DATA 0
9000 REM ERROR HANDLER
IF RET(1) = 999 THEN
  F = ABS(SGN(RET(0)))
  ON ERROR GOTO 9000:RESUME 'NEXT
END IF
PRINT "*** UNEXPECTED ERROR ON LINE ";RET(1);" ***"
9999 END
