rem ----------------------------------------------------
rem Filename: call1.bas
rem  Purpose: Verify CALL
rem ----------------------------------------------------

Print "CallSub.BAS -- Test BASIC Call and Sub Statements"
Print "The next printed line should be from the Subroutine."
Print
x = 5
a$ = "Hello"
y = 17
b$ = "There"
z = 77
Call TestSub( x, a$, y)

Print
Print "This is back at the main program. "
Print "The value of variable <testvar> is now "; testvar

Print "Did it work?"

rem Verify SUB parameters are local to the subroutine
if x <> 5 then 
    print "Subroutine modified x"
    goto &TestFailed
end if

if a$ <> "Hello" then 
    print "Subroutine modified a$"
    goto &TestFailed
end if

if y <> 17 then 
    print "Subroutine modified y"
    goto &TestFailed
end if

rem Verify GLOBAL variable parameters are updated

if b$ <> "Fromsub" then 
    print "Subroutine did not modify b$"
    goto &TestFailed
end if

if z <> 21 then 
    print "Subroutine did not modify z"
    goto &TestFailed
end if


goto &TestPassed

rem ----------------------------------------------------
rem Subroutine TestSub
rem ----------------------------------------------------

Proc TestSub( xarg, yarg$, tvar )
   Print "This is written from the Subroutine."
   Print "The value of variable <xarg>  is"; @xarg
   Print "The value of variable <yarg$> is "; @yarg$
   Print "The value of variable <tvar>  is"; @tvar

   rem Verify expected parameter values
   if @xarg <> 5 then 
     print "@xarg is wrong"; @xarg
    goto &TestFailed
   end if

   if @yarg$ <> "Hello" then 
     print "yarg$ is wrong [";@yarg$;"]"
    goto &TestFailed
   end if

   if @tvar <> 17 then
     print "tvar is wrong"; @tvar, @tvar - 17
    goto &TestFailed
   end if
   
   REM exit sub

   rem Assign values to subroutine parameters
   @xarg = -1
   @yarg$ = "N/A"
   @tvar = 99

   Print "The value of variable <tvar> is reset to "; @tvar

    rem Modify GLOBAL variables
   b$ = "Fromsub" 
   z = 21 

    print "Exiting Subroutine"
return

print "*** END SUB FAILED ***"
goto &TestFailed
STOP
8000 rem ------------------------------------------
8010 &TestPassed:
8020 print "*** ALL TESTS PASSED ***"
8030 goto &TheEnd
8500 rem ------------------------------------------
8510 &TestFailed:
8520 print "*** TEST FAILED ***"
8530 goto &TheEnd
9000 rem ------------------------------------------
9010 &TheEnd:
9999 end

