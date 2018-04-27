REM ------------------------------------------------------------
REM   PURPOSE: TEST THE PARAMETER RANGE CHECKS
REM    AUTHOR: HOWARD WULF, AF5NE
REM GENERATED: 02/21/2015 14:08:25
REM  FILENAME: F_LEN_A_N.BAS
REM    SYNTAX: N  = LEN( A$ )
REM  UNIQUEID: F_LEN_A_N
REM  FUNCTION: LEN
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
999 N  = LEN( A$ )
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
DATA  2 ,  0 ,  1 , " "
DATA  3 ,  0 ,  1 , "3"
DATA  4 ,  0 ,  1 , "z"
DATA  5 ,  0 ,  2 , "  "
DATA  6 ,  0 ,  2 , " 3"
DATA  7 ,  0 ,  2 , " z"
DATA  8 ,  0 ,  2 , "3 "
DATA  9 ,  0 ,  2 , "z "
DATA  10 ,  0 ,  3 , " 3 "
DATA  11 ,  0 ,  3 , " z "
DATA  12 ,  0 ,  5 , " LLLL"
DATA  13 ,  0 ,  10 , "    ''    "
DATA  14 ,  0 ,  10 , "   oo     "
DATA  15 ,  0 ,  5 , " 00  "
DATA  16 ,  0 ,  9 , "   \\\\  "
DATA  17 ,  0 ,  8 , " lllll  "
DATA  18 ,  0 ,  10 , "    uuuu  "
DATA  19 ,  0 ,  5 , "  a  "
DATA  20 ,  0 ,  7 , "       "
DATA  21 ,  0 ,  8 , " hh     "
DATA  22 ,  0 ,  6 , "  h   "
DATA  23 ,  0 ,  2 , " )"
DATA  24 ,  0 ,  8 , "    5   "
DATA  25 ,  0 ,  4 , "  2 "
DATA  26 ,  0 ,  5 , " -   "
DATA  27 ,  0 ,  3 , "dd "
DATA  28 ,  0 ,  9 , "  tttt   "
DATA  29 ,  0 ,  9 , "  ::::   "
DATA  30 ,  0 ,  9 , "   ___   "
DATA  31 ,  0 ,  5 , "     "
DATA  32 ,  0 ,  7 , "       "
DATA  33 ,  0 ,  8 , " _____  "
DATA  34 ,  0 ,  3 , " S "
DATA  35 ,  0 ,  7 , "  FFFF "
DATA  36 ,  0 ,  7 , "    kkk"
DATA  37 ,  0 ,  10 , "    NNNN  "
DATA  38 ,  0 ,  9 , "   mmmm  "
DATA  39 ,  0 ,  10 , "    ,,,,, "
DATA  40 ,  0 ,  5 , "OO   "
DATA  41 ,  0 ,  9 , "   HHHH  "
DATA  42 ,  0 ,  3 , " < "
DATA  43 ,  0 ,  8 , "   j    "
DATA  44 ,  0 ,  12 , "    yyyy    "
DATA  45 ,  0 ,  6 , "  +++ "
DATA  46 ,  0 ,  3 , "o  "
DATA  47 ,  0 ,  6 , "    (("
DATA  48 ,  0 ,  12 , "     uu     "
DATA  49 ,  0 ,  11 , "    00     "
DATA  50 ,  0 ,  9 , "     HH  "
DATA  51 ,  0 ,  5 , "     "
DATA  52 ,  0 ,  6 , "  G   "
DATA  53 ,  0 ,  9 , "    {{{{ "
DATA  54 ,  0 ,  12 , "    ]]]     "
DATA  55 ,  0 ,  3 , "CCC"
DATA  56 ,  0 ,  9 , "     44  "
DATA  57 ,  0 ,  4 , "^^  "
DATA  58 ,  0 ,  10 , "    QQ    "
DATA  59 ,  0 ,  11 , "     FFFF  "
DATA  60 ,  0 ,  5 , " n   "
DATA  61 ,  0 ,  8 , "    hh  "
DATA  62 ,  0 ,  7 , "555    "
DATA  63 ,  0 ,  9 , "    YYYY "
DATA  64 ,  0 ,  10 , "    w     "
DATA  65 ,  0 ,  5 , "MMMM "
DATA  66 ,  0 ,  9 , " JJJJ    "
DATA  67 ,  0 ,  10 , "    %%    "
DATA  68 ,  0 ,  8 , "     *  "
DATA  69 ,  0 ,  10 , "    @@@   "
DATA  70 ,  0 ,  2 , "I "
DATA  71 ,  0 ,  8 , "    ss  "
DATA  72 ,  0 ,  4 , "  f "
DATA  73 ,  0 ,  6 , "   D  "
DATA  74 ,  0 ,  7 , "###    "
DATA  75 ,  0 ,  10 , "    '     "
DATA  76 ,  0 ,  8 , "  dd    "
DATA  77 ,  0 ,  7 , " TT    "
DATA  78 ,  0 ,  6 , "  vvv "
DATA  79 ,  0 ,  6 , "    X "
DATA  80 ,  0 ,  8 , "    %   "
DATA  81 ,  0 ,  2 , "  "
DATA  82 ,  0 ,  8 , " 5555   "
DATA  83 ,  0 ,  7 , "FFFF   "
DATA  84 ,  0 ,  9 , "     %%  "
DATA  85 ,  0 ,  5 , "     "
DATA  86 ,  0 ,  10 , "  ~~~~    "
DATA  87 ,  0 ,  9 , "   ||    "
DATA  88 ,  0 ,  13 , "     SSS     "
DATA  89 ,  0 ,  8 , "   6666 "
DATA  90 ,  0 ,  9 , "    ww   "
DATA  91 ,  0 ,  9 , "  1111   "
DATA  92 ,  0 ,  3 , " K "
DATA  93 ,  0 ,  8 , "  ttttt "
DATA  94 ,  0 ,  9 , " OOOOO   "
DATA  95 ,  0 ,  4 , " E  "
DATA  96 ,  0 ,  9 , " PPPPP   "
DATA  97 ,  0 ,  7 , "    == "
DATA  98 ,  0 ,  9 , " VVVV    "
DATA  99 ,  0 ,  5 , " c   "
DATA  100 ,  0 ,  13 , "    yyyy     "
DATA 0
9000 REM ERROR HANDLER
IF RET(1) = 999 THEN
  F = ABS(SGN(ERR))
  ON ERROR GOTO 9000:RESUME 'NEXT
END IF
PRINT "*** UNEXPECTED ERROR ON LINE ";RET(1);" ***"
9999 END
