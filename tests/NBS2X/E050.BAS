REM verify structured FUNCTION - END FUNCTION
REM HOWARD WULF, AF5NE
REM 
PRINT "BEFORE FUNCTION DEFINITIONS"
goto &main
PROC TOM( X )
  PRINT "ENTER TOM"
  PRINT "X=";@X
  PRINT "EXIT TOM"
  RETURN 1

PROC JERRY( X )
  PRINT "ENTER JERRY"
  PRINT "X=";@X
  PRINT "EXIT JERRY"
  RETURN 2

&main:
PRINT "AFTER FUNCTION DEFINITIONS"
REM verify use as a function
LET X = FN TOM( 11 )
PRINT "X=";X
IF X <> 1 THEN
  PRINT "*** TEST FAILED ***"
  STOP
END IF
REM verify use as a subroutine
CALL TOM(12)
REM verify use as a function
LET X = FN JERRY( 13 )
PRINT "X=";X
IF X <> 2 THEN
  PRINT "*** TEST FAILED ***"
  STOP
END IF
REM verify use as a subroutine
CALL JERRY(14)
REM verify use as a function
LET X = FN TOM( 15 ) + FN JERRY( 16 ) 
PRINT "X=";X
IF X <> 3 THEN
  PRINT "*** TEST FAILED ***"
  STOP
END IF
REM verify use as a subroutine with function
CALL TOM(FN JERRY( 17 ))
REM verify use as a subroutine with function
CALL JERRY(FN TOM( 18 ))
REM verify use as a function
LET X = FN TOM( FN JERRY( 19 )  ) 
PRINT "X=";X
IF X <> 1 THEN
  PRINT "*** TEST FAILED ***"
  STOP
END IF
REM verify use as a function
LET X = FN JERRY( FN TOM( 20 )  ) 
PRINT "X=";X
IF X <> 2 THEN
  PRINT "*** TEST FAILED ***"
  STOP
END IF
REM verify use as a subroutine calling itself as a parameter
CALL TOM(FN TOM( 21 ))
REM verify use as a subroutine calling itself as a parameter
CALL JERRY(FN JERRY( 22 ))
REM verify use as a function calling itself as a parameter
LET X = FN TOM( FN TOM( 23 ) )
IF X <> 1 THEN
  PRINT "*** TEST FAILED ***"
  STOP
END IF
REM verify use as a function calling itself as a parameter
LET X = FN JERRY( FN JERRY( 24 ) )
IF X <> 2 THEN
  PRINT "*** TEST FAILED ***"
  STOP
END IF
REM 
PRINT "*** TEST PASSED ***"
END
