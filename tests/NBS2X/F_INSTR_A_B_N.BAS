REM ------------------------------------------------------------
REM   PURPOSE: TEST THE PARAMETER RANGE CHECKS
REM    AUTHOR: HOWARD WULF, AF5NE
REM GENERATED: 02/21/2015 14:08:23
REM  FILENAME: F_INSTR_A_B_N.BAS
REM    SYNTAX: N  = INSTR( A$, B$ )
REM  UNIQUEID: F_INSTR_A_B_N
REM  FUNCTION: INSTR
REM 
REM	VARIABLE	DESCRIPTION
REM	I	CURRENT  TESTCASE NUMBER
REM	E	EXPECTED ERROR FLAG
REM	F	ACTUAL   ERROR FLAG
REM	R	EXPECTED FUNCTION RESULT
REM	N	ACTUAL   FUNCTION RESULT
REM	A$	ACTUAL   FUNCTION PARAMETER
REM	B$	ACTUAL   FUNCTION PARAMETER
REM 
RESTORE 2000
REM GET TESTCASE #
READ  I
WHILE I > 0
REM GET TESTCASE DATA
READ  E, R, A$, B$
R=R-1 ' Engine BASIC INSTR() is base 0
REM DISPLAY TESTCASE DATA
PRINT "TESTCASE #";I,"E=";E,"R=";R, "A$=[";A$;"]", "B$=[";B$;"]"
REM EXECUTE TESTCASE
N =  0 
S$ = ""
ON ERROR GOTO 9000
F = 0
999 N  = INSTR( A$, B$ )
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
DATA  1 ,  0 ,  1 , "", ""
DATA  2 ,  0 ,  1 , " ", " "
DATA  3 ,  0 ,  1 , "3", "3"
DATA  4 ,  0 ,  1 , "z", "z"
DATA  5 ,  0 ,  1 , "  ", "  "
DATA  6 ,  0 ,  1 , " 3", " 3"
DATA  7 ,  0 ,  1 , " z", " z"
DATA  8 ,  0 ,  1 , "3 ", "3 "
DATA  9 ,  0 ,  1 , "z ", "z "
DATA  10 ,  0 ,  1 , " 3 ", " 3 "
DATA  11 ,  0 ,  1 , " z ", " z "
DATA  12 ,  0 ,  1 , " LLLL", " LLLL"
DATA  13 ,  0 ,  1 , "    ''    ", "    ''    "
DATA  14 ,  0 ,  1 , "   oo     ", "   oo     "
DATA  15 ,  0 ,  1 , " 00  ", " 00  "
DATA  16 ,  0 ,  1 , "   \\\\  ", "   \\\\  "
DATA  17 ,  0 ,  1 , " lllll  ", " lllll  "
DATA  18 ,  0 ,  1 , "    uuuu  ", "    uuuu  "
DATA  19 ,  0 ,  1 , "  a  ", "  a  "
DATA  20 ,  0 ,  1 , "       ", "       "
DATA  21 ,  0 ,  1 , " hh     ", " hh     "
DATA  22 ,  0 ,  1 , "  h   ", "  h   "
DATA  23 ,  0 ,  1 , " )", " )"
DATA  24 ,  0 ,  1 , "    5   ", "    5   "
DATA  25 ,  0 ,  1 , "  2 ", "  2 "
DATA  26 ,  0 ,  1 , " -   ", " -   "
DATA  27 ,  0 ,  1 , "dd ", "dd "
DATA  28 ,  0 ,  1 , "  tttt   ", "  tttt   "
DATA  29 ,  0 ,  1 , "  ::::   ", "  ::::   "
DATA  30 ,  0 ,  1 , "   ___   ", "   ___   "
DATA  31 ,  0 ,  1 , "     ", "     "
DATA  32 ,  0 ,  1 , "       ", "       "
DATA  33 ,  0 ,  1 , " _____  ", " _____  "
DATA  34 ,  0 ,  1 , " S ", " S "
DATA  35 ,  0 ,  1 , "  FFFF ", "  FFFF "
DATA  36 ,  0 ,  1 , "    kkk", "    kkk"
DATA  37 ,  0 ,  1 , "    NNNN  ", "    NNNN  "
DATA  38 ,  0 ,  1 , "   mmmm  ", "   mmmm  "
DATA  39 ,  0 ,  1 , "    ,,,,, ", "    ,,,,, "
DATA  40 ,  0 ,  1 , "OO   ", "OO   "
DATA  41 ,  0 ,  1 , "   HHHH  ", "   HHHH  "
DATA  42 ,  0 ,  1 , " < ", " < "
DATA  43 ,  0 ,  1 , "   j    ", "   j    "
DATA  44 ,  0 ,  1 , "    yyyy    ", "    yyyy    "
DATA  45 ,  0 ,  1 , "  +++ ", "  +++ "
DATA  46 ,  0 ,  1 , "o  ", "o  "
DATA  47 ,  0 ,  1 , "    ((", "    (("
DATA  48 ,  0 ,  1 , "     uu     ", "     uu     "
DATA  49 ,  0 ,  1 , "    00     ", "    00     "
DATA  50 ,  0 ,  1 , "     HH  ", "     HH  "
DATA  51 ,  0 ,  1 , "     ", "     "
DATA  52 ,  0 ,  1 , "  G   ", "  G   "
DATA  53 ,  0 ,  1 , "    {{{{ ", "    {{{{ "
DATA  54 ,  0 ,  1 , "    ]]]     ", "    ]]]     "
DATA  55 ,  0 ,  1 , "CCC", "CCC"
DATA  56 ,  0 ,  1 , "     44  ", "     44  "
DATA  57 ,  0 ,  1 , "^^  ", "^^  "
DATA  58 ,  0 ,  1 , "    QQ    ", "    QQ    "
DATA  59 ,  0 ,  1 , "     FFFF  ", "     FFFF  "
DATA  60 ,  0 ,  1 , " n   ", " n   "
DATA  61 ,  0 ,  1 , "    hh  ", "    hh  "
DATA  62 ,  0 ,  1 , "555    ", "555    "
DATA  63 ,  0 ,  1 , "    YYYY ", "    YYYY "
DATA  64 ,  0 ,  1 , "    w     ", "    w     "
DATA  65 ,  0 ,  1 , "MMMM ", "MMMM "
DATA  66 ,  0 ,  1 , " JJJJ    ", " JJJJ    "
DATA  67 ,  0 ,  1 , "    %%    ", "    %%    "
DATA  68 ,  0 ,  1 , "     *  ", "     *  "
DATA  69 ,  0 ,  1 , "    @@@   ", "    @@@   "
DATA  70 ,  0 ,  1 , "I ", "I "
DATA  71 ,  0 ,  1 , "    ss  ", "    ss  "
DATA  72 ,  0 ,  1 , "  f ", "  f "
DATA  73 ,  0 ,  1 , "   D  ", "   D  "
DATA  74 ,  0 ,  1 , "###    ", "###    "
DATA  75 ,  0 ,  1 , "    '     ", "    '     "
DATA  76 ,  0 ,  1 , "  dd    ", "  dd    "
DATA  77 ,  0 ,  1 , " TT    ", " TT    "
DATA  78 ,  0 ,  1 , "  vvv ", "  vvv "
DATA  79 ,  0 ,  1 , "    X ", "    X "
DATA  80 ,  0 ,  1 , "    %   ", "    %   "
DATA  81 ,  0 ,  1 , "  ", "  "
DATA  82 ,  0 ,  1 , " 5555   ", " 5555   "
DATA  83 ,  0 ,  1 , "FFFF   ", "FFFF   "
DATA  84 ,  0 ,  1 , "     %%  ", "     %%  "
DATA  85 ,  0 ,  1 , "     ", "     "
DATA  86 ,  0 ,  1 , "  ~~~~    ", "  ~~~~    "
DATA  87 ,  0 ,  1 , "   ||    ", "   ||    "
DATA  88 ,  0 ,  1 , "     SSS     ", "     SSS     "
DATA  89 ,  0 ,  1 , "   6666 ", "   6666 "
DATA  90 ,  0 ,  1 , "    ww   ", "    ww   "
DATA  91 ,  0 ,  1 , "  1111   ", "  1111   "
DATA  92 ,  0 ,  1 , " K ", " K "
DATA  93 ,  0 ,  1 , "  ttttt ", "  ttttt "
DATA  94 ,  0 ,  1 , " OOOOO   ", " OOOOO   "
DATA  95 ,  0 ,  1 , " E  ", " E  "
DATA  96 ,  0 ,  1 , " PPPPP   ", " PPPPP   "
DATA  97 ,  0 ,  1 , "    == ", "    == "
DATA  98 ,  0 ,  1 , " VVVV    ", " VVVV    "
DATA  99 ,  0 ,  1 , " c   ", " c   "
DATA  100 ,  0 ,  1 , "    yyyy     ", "    yyyy     "
DATA 0
9000 REM ERROR HANDLER
IF RET(1) = 999 THEN
  F = ABS(SGN(ERR))
  ON ERROR GOTO 9000:RESUME 'NEXT
END IF
PRINT "*** UNEXPECTED ERROR ON LINE ";RET(1);" ***"
9999 END
