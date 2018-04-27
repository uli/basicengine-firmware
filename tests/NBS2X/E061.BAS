REM Purpose: Verify boolean operators
REM  Author: Howard Wulf, AF5NE
REM    Date: 2015-02-20
REM
GOTO &main
PROC TF( N )
  IF @N = 0 THEN
    print "F   ";
  ELSE
    print "T   ";
  END IF
return

&main:
REM
PRINT "                X   X   X   X   X   X   X   X   X   "
PRINT "       NOT NOT AND  OR XOR  <   <=  =   >=  >   <>  "
PRINT "X   Y   X   Y   Y   Y   Y   Y   Y   Y   Y   Y   Y   "
PRINT "=   =   =   =   =   =   =   =   =   =   =   =   =   "
for X = -1 to 0
for Y = -1 to 0
    CALL TF(      X)
    CALL TF(      Y)
    CALL TF(  NOT X)
    CALL TF(  NOT Y)
    CALL TF(X AND Y)
    CALL TF(X  OR Y)
    CALL TF(X EOR Y)
'    CALL TF(X EQV Y)
'    CALL TF(X IMP Y)
'    PRINT "?   ";
    CALL TF(X <   Y)
    CALL TF(X <=  Y)
    CALL TF(X  =  Y)
    CALL TF(X >=  Y)
    CALL TF(X   > Y)
    CALL TF(X  <> Y)
    PRINT
next Y
next X

END
