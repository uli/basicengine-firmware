REM ------------------------------------------------------------
REM   PURPOSE: TEST THE PARAMETER RANGE CHECKS
REM    AUTHOR: HOWARD WULF, AF5NE
REM GENERATED: 02/21/2015 14:08:25
REM  FILENAME: F_LEFT_A_X_S.BAS
REM    SYNTAX: S$ = LEFT$( A$, X )
REM  UNIQUEID: F_LEFT_A_X_S
REM  FUNCTION: LEFT$
REM 
REM	VARIABLE	DESCRIPTION
REM	I	CURRENT  TESTCASE NUMBER
REM	E	EXPECTED ERROR FLAG
REM	F	ACTUAL   ERROR FLAG
REM	R$	EXPECTED FUNCTION RESULT
REM	S$	ACTUAL   FUNCTION RESULT
REM	A$	ACTUAL   FUNCTION PARAMETER
REM	X	ACTUAL   FUNCTION PARAMETER
REM 
RESTORE 2000
REM GET TESTCASE #
READ  I
WHILE I > 0
REM GET TESTCASE DATA
READ  E, R$, A$, X
REM DISPLAY TESTCASE DATA
PRINT "TESTCASE #";I,"E=";E,"R$=[";R$;"]", "A$=[";A$;"]", "X=";X
REM EXECUTE TESTCASE
N =  0 
S$ = ""
ON ERROR GOTO 9000
F = 0
999 S$ = LEFT$( A$, X )
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
DATA  1 ,  0 , "", "",  0 
DATA  2 ,  0 , " ", " ",  1 
DATA  3 ,  1 , "", "3", -1 
DATA  4 ,  0 , "", "z",  .4 
DATA  5 ,  0 , "", "  ", -.4 
DATA  6 ,  0 , "", " 3",  .6 
DATA  7 ,  0 , "", " z", -.6 
DATA  8 ,  0 , "3 ", "3 ",  254 
DATA  9 ,  0 , "z ", "z ",  255 
DATA  10 ,  0 , " 3 ", " 3 ",  256 
DATA  11 ,  0 , " z ", " z ",  32000 
DATA  12 ,  0 , "", " LLLL", -.227229 
DATA  13 ,  0 , "", "    ''    ", -.239157 
DATA  14 ,  0 , "", "   oo     ", -.213586 
DATA  15 ,  0 , "", " 00  ",  .274675 
DATA  16 ,  0 , "", "   \\\\  ",  .169766 
DATA  17 ,  0 , "", " lllll  ",  .318766 
DATA  18 ,  0 , "", "    uuuu  ",  2.06658E-2 
DATA  19 ,  0 , "", "  a  ",  .188257 
DATA  20 ,  0 , "", "       ", -.277537 
DATA  21 ,  0 , "", " hh     ", -.481303 
DATA  22 ,  0 , "", "  h   ",  .43556 
DATA  23 ,  0 , "", " )", -.210971 
DATA  24 ,  0 , "", "    5   ", -.455216 
DATA  25 ,  1 , "", "  2 ", -110.907 
DATA  26 ,  0 , " -   ", " -   ",  105.632 
DATA  27 ,  1 , "", "dd ", -114.341 
DATA  28 ,  0 , "  tttt   ", "  tttt   ",  109.706 
DATA  29 ,  0 , "  ::::   ", "  ::::   ",  109.198 
DATA  30 ,  0 , "   ___   ", "   ___   ",  85.8773 
DATA  31 ,  1 , "", "     ", -58.3548 
DATA  32 ,  0 , "       ", "       ",  29.6397 
DATA  33 ,  1 , "", " _____  ", -108.36 
DATA  34 ,  1 , "", " S ", -46.5199 
DATA  35 ,  0 , "  FFFF ", "  FFFF ",  112.521 
DATA  36 ,  0 , "    kkk", "    kkk",  45.4151 
DATA  37 ,  0 , "    NNNN  ", "    NNNN  ",  101.129 
DATA  38 ,  0 , "   mmmm  ", "   mmmm  ",  65.0473 
DATA  39 ,  0 , "    ,,,,, ", "    ,,,,, ",  48.4012 
DATA  40 ,  0 , "OO   ", "OO   ",  122.294 
DATA  41 ,  1 , "", "   HHHH  ", -56.1094 
DATA  42 ,  1 , "", " < ", -121.087 
DATA  43 ,  1 , "", "   j    ", -123.517 
DATA  44 ,  1 , "", "    yyyy    ", -97.6301 
DATA  45 ,  1 , "", "  +++ ", -64.0001 
DATA  46 ,  1 , "", "o  ", -25.6888 
DATA  47 ,  0 , "    ((", "    ((",  14.8006 
DATA  48 ,  1 , "", "     uu     ", -61.0877 
DATA  49 ,  1 , "", "    00     ", -66.6381 
DATA  50 ,  1 , "", "     HH  ", -5476.5 
DATA  51 ,  1 , "", "     ", -21210.3 
DATA  52 ,  1 , "", "  G   ", -28757.2 
DATA  53 ,  1 , "", "    {{{{ ", -28732.2 
DATA  54 ,  0 , "    ]]]     ", "    ]]]     ",  12730.3 
DATA  55 ,  1 , "", "CCC", -1830.69 
DATA  56 ,  0 , "     44  ", "     44  ",  5486.39 
DATA  57 ,  0 , "^^  ", "^^  ",  22405.9 
DATA  58 ,  0 , "    QQ    ", "    QQ    ",  30809.8 
DATA  59 ,  1 , "", "     FFFF  ", -28930.6 
DATA  60 ,  1 , "", " n   ", -6014.53 
DATA  61 ,  0 , "    hh  ", "    hh  ",  17414.6 
DATA  62 ,  1 , "", "555    ", -10538.3 
DATA  63 ,  1 , "", "    YYYY ", -17536.3 
DATA  64 ,  1 , "", "    w     ", -8856.62 
DATA  65 ,  1 , "", "MMMM ", -5140.12 
DATA  66 ,  1 , "", " JJJJ    ", -8595.31 
DATA  67 ,  1 , "", "    %%    ", -3382.87 
DATA  68 ,  1 , "", "     *  ", -5002.07 
DATA  69 ,  1 , "", "    @@@   ", -5800.72 
DATA  70 ,  0 , "I ", "I ",  29686.4 
DATA  71 ,  0 , "    ss  ", "    ss  ",  2571. 
DATA  72 ,  0 , "  f ", "  f ",  31091. 
DATA  73 ,  0 , "   D  ", "   D  ",  14819. 
DATA  74 ,  0 , "###    ", "###    ",  3790.9 
DATA  75 ,  1 , "", "    '     ", -4.2981E+9 
DATA  76 ,  1 , "", "  dd    ", -7.05453E+8 
DATA  77 ,  1 , "", " TT    ", -3.10179E+9 
DATA  78 ,  1 , "", "  vvv ", -4.86159E+9 
DATA  79 ,  1 , "", "    X ", -8.62908E+8 
DATA  80 ,  1 , "", "    %   ", -3.4101E+9 
DATA  81 ,  0 , "  ", "  ",  3.3889E+9 
DATA  82 ,  0 , " 5555   ", " 5555   ",  2.49826E+9 
DATA  83 ,  0 , "FFFF   ", "FFFF   ",  2.3491E+9 
DATA  84 ,  0 , "     %%  ", "     %%  ",  1.94745E+8 
DATA  85 ,  0 , "     ", "     ",  2.02475E+9 
DATA  86 ,  1 , "", "  ~~~~    ", -2.26748E+9 
DATA  87 ,  1 , "", "   ||    ", -2.10587E+9 
DATA  88 ,  0 , "     SSS     ", "     SSS     ",  4.23585E+9 
DATA  89 ,  1 , "", "   6666 ", -2.66524E+9 
DATA  90 ,  0 , "    ww   ", "    ww   ",  4.01052E+9 
DATA  91 ,  1 , "", "  1111   ", -4.87912E+9 
DATA  92 ,  0 , " K ", " K ",  1.77556E+9 
DATA  93 ,  0 , "  ttttt ", "  ttttt ",  2.87705E+8 
DATA  94 ,  0 , " OOOOO   ", " OOOOO   ",  1.99977E+8 
DATA  95 ,  1 , "", " E  ", -3.39941E+9 
DATA  96 ,  1 , "", " PPPPP   ", -3.49119E+9 
DATA  97 ,  1 , "", "    == ", -2.86324E+9 
DATA  98 ,  0 , " VVVV    ", " VVVV    ",  4.20418E+9 
DATA  99 ,  1 , "", " c   ", -3.1518E+9 
DATA  100 ,  0 , "    yyyy     ", "    yyyy     ",  4.60615E+9 
DATA 0
9000 REM ERROR HANDLER
IF RET(1) = 999 THEN
  F = ABS(SGN(RET(0)))
  ON ERROR GOTO 9000:RESUME 'NEXT
END IF
PRINT "*** UNEXPECTED ERROR ON LINE ";RET(1);" ***"
9999 END
