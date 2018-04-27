REM ------------------------------------------------------------
REM   PURPOSE: TEST THE PARAMETER RANGE CHECKS
REM    AUTHOR: HOWARD WULF, AF5NE
REM GENERATED: 02/21/2015 14:08:34
REM  FILENAME: F_STRING_X_A_S.BAS
REM    SYNTAX: S$ = STRING$( X, A$ )
REM  UNIQUEID: F_STRING_X_A_S
REM  FUNCTION: STRING$
REM 
REM	VARIABLE	DESCRIPTION
REM	I	CURRENT  TESTCASE NUMBER
REM	E	EXPECTED ERROR FLAG
REM	F	ACTUAL   ERROR FLAG
REM	R$	EXPECTED FUNCTION RESULT
REM	S$	ACTUAL   FUNCTION RESULT
REM	X	ACTUAL   FUNCTION PARAMETER
REM	A$	ACTUAL   FUNCTION PARAMETER
REM 
RESTORE 2000
REM GET TESTCASE #
READ  I
WHILE I > 0
REM GET TESTCASE DATA
READ  E, R$, X, A$
REM DISPLAY TESTCASE DATA
PRINT "TESTCASE #";I,"E=";E,"R$=[";R$;"]", "X=";X, "A$=[";A$;"]"
REM EXECUTE TESTCASE
N =  0 
S$ = ""
ON ERROR GOTO 9000
F = 0
999 S$ = STRING$( X, A$ )
ERROR 0
REM DISPLAY TESTCASE RESULTS
PRINT ,"F=";F,"S$=[";S$;"]"
REM VERIFY EXPECTED ERROR
IF E <> F THEN
  PRINT "*** TEST FAILED, EXCEPTION MISMATCH ***"
  STOP
END IF
REM VERIFY EXPECTED RESULT
IF R$ <> S$ THEN
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
DATA  1 ,  1 , "",  0 , ""
DATA  2 ,  0 , " ",  1 , " "
DATA  3 ,  1 , "", -1 , "3"
DATA  4 ,  0 , "",  .4 , "z"
DATA  5 ,  0 , "", -.4 , "  "
DATA  6 ,  0 , "",  .6 , " 3"
DATA  7 ,  0 , "", -.6 , " z"
DATA  8 ,  0 , "3333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333333",  100 , "3 "
DATA  9 ,  0 , "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz",  100 , "z "
DATA  12 ,  0 , "", -.227229 , " LLLL"
DATA  13 ,  0 , "", -.239157 , "    ''    "
DATA  14 ,  0 , "", -.213586 , "   oo     "
DATA  15 ,  0 , "",  .274675 , " 00  "
DATA  16 ,  0 , "",  .169766 , "   \\\\  "
DATA  17 ,  0 , "",  .318766 , " lllll  "
DATA  18 ,  0 , "",  2.06658E-2 , "    uuuu  "
DATA  19 ,  0 , "",  .188257 , "  a  "
DATA  20 ,  0 , "", -.277537 , "       "
DATA  21 ,  0 , "", -.481303 , " hh     "
DATA  22 ,  0 , "",  .43556 , "  h   "
DATA  23 ,  0 , "", -.210971 , " )"
DATA  24 ,  0 , "", -.455216 , "    5   "
DATA  25 ,  1 , "", -110.907 , "  2 "
DATA  26 ,  0 , "                                                                                                         ",  105.632 , " -   "
DATA  27 ,  1 , "", -114.341 , "dd "
DATA  28 ,  0 , "                                                                                                             ",  109.706 , "  tttt   "
DATA  29 ,  0 , "                                                                                                             ",  109.198 , "  ::::   "
DATA  30 ,  0 , "                                                                                     ",  85.8773 , "   ___   "
DATA  31 ,  1 , "", -58.3548 , "     "
DATA  32 ,  0 , "                             ",  29.6397 , "       "
DATA  33 ,  1 , "", -108.36 , " _____  "
DATA  34 ,  1 , "", -46.5199 , " S "
DATA  35 ,  0 , "                                                                                                                ",  112.521 , "  FFFF "
DATA  36 ,  0 , "                                             ",  45.4151 , "    kkk"
DATA  37 ,  0 , "                                                                                                     ",  101.129 , "    NNNN  "
DATA  38 ,  0 , "                                                                 ",  65.0473 , "   mmmm  "
DATA  39 ,  0 , "                                                ",  48.4012 , "    ,,,,, "
DATA  40 ,  0 , "OOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO",  122.294 , "OO   "
DATA  41 ,  1 , "", -56.1094 , "   HHHH  "
DATA  42 ,  1 , "", -121.087 , " < "
DATA  43 ,  1 , "", -123.517 , "   j    "
DATA  44 ,  1 , "", -97.6301 , "    yyyy    "
DATA  45 ,  1 , "", -64.0001 , "  +++ "
DATA  46 ,  1 , "", -25.6888 , "o  "
DATA  47 ,  0 , "              ",  14.8006 , "    (("
DATA  48 ,  1 , "", -61.0877 , "     uu     "
DATA  49 ,  1 , "", -66.6381 , "    00     "
DATA  50 ,  1 , "", -5476.5 , "     HH  "
DATA  51 ,  1 , "", -21210.3 , "     "
DATA  52 ,  1 , "", -28757.2 , "  G   "
DATA  53 ,  1 , "", -28732.2 , "    {{{{ "
DATA  55 ,  1 , "", -1830.69 , "CCC"
DATA  59 ,  1 , "", -28930.6 , "     FFFF  "
DATA  60 ,  1 , "", -6014.53 , " n   "
DATA  62 ,  1 , "", -10538.3 , "555    "
DATA  63 ,  1 , "", -17536.3 , "    YYYY "
DATA  64 ,  1 , "", -8856.62 , "    w     "
DATA  65 ,  1 , "", -5140.12 , "MMMM "
DATA  66 ,  1 , "", -8595.31 , " JJJJ    "
DATA  67 ,  1 , "", -3382.87 , "    %%    "
DATA  68 ,  1 , "", -5002.07 , "     *  "
DATA  69 ,  1 , "", -5800.72 , "    @@@   "
DATA  75 ,  1 , "", -4.2981E+9 , "    '     "
DATA  76 ,  1 , "", -7.05453E+8 , "  dd    "
DATA  77 ,  1 , "", -3.10179E+9 , " TT    "
DATA  78 ,  1 , "", -4.86159E+9 , "  vvv "
DATA  79 ,  1 , "", -8.62908E+8 , "    X "
DATA  80 ,  1 , "", -3.4101E+9 , "    %   "
DATA  81 ,  1 , "",  3.3889E+9 , "  "
DATA  82 ,  1 , "",  2.49826E+9 , " 5555   "
DATA  83 ,  1 , "",  2.3491E+9 , "FFFF   "
DATA  84 ,  1 , "",  1.94745E+8 , "     %%  "
DATA  85 ,  1 , "",  2.02475E+9 , "     "
DATA  86 ,  1 , "", -2.26748E+9 , "  ~~~~    "
DATA  87 ,  1 , "", -2.10587E+9 , "   ||    "
DATA  88 ,  1 , "",  4.23585E+9 , "     SSS     "
DATA  89 ,  1 , "", -2.66524E+9 , "   6666 "
DATA  90 ,  1 , "",  4.01052E+9 , "    ww   "
DATA  91 ,  1 , "", -4.87912E+9 , "  1111   "
DATA  92 ,  1 , "",  1.77556E+9 , " K "
DATA  93 ,  1 , "",  2.87705E+8 , "  ttttt "
DATA  94 ,  1 , "",  1.99977E+8 , " OOOOO   "
DATA  95 ,  1 , "", -3.39941E+9 , " E  "
DATA  96 ,  1 , "", -3.49119E+9 , " PPPPP   "
DATA  97 ,  1 , "", -2.86324E+9 , "    == "
DATA  98 ,  1 , "",  4.20418E+9 , " VVVV    "
DATA  99 ,  1 , "", -3.1518E+9 , " c   "
DATA  100 ,  1 , "",  4.60615E+9 , "    yyyy     "
DATA 0
9000 REM ERROR HANDLER
IF RET(1) = 999 THEN
  F = ABS(SGN(RET(0)))
  ON ERROR GOTO 9000:RESUME 'NEXT
END IF
PRINT "*** UNEXPECTED ERROR ON LINE ";RET(1);" ***"
9999 END
