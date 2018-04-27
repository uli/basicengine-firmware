
rem ----------------------------------------------------
rem CallFunc.BAS
rem ----------------------------------------------------

Print "CallFunc.BAS -- Test BASIC User-defined Function Statements"
Print "The next printed line should be from the Function."
Print
testvar = 17

x = FN TestFnc( 5, "Hello", testvar )

Print
Print "This is back at the main program."
Print "The value of variable <testvar> is now "; testvar
Print "The returned value from the function is "; x

Print "Did it work?"
End

rem ----------------------------------------------------
rem Subroutine TestFnc
rem ----------------------------------------------------

Proc TestFnc( xarg, yarg$, tvar )
   Print "This is written from the Function."
   Print "The value of variable <xarg> is "; @xarg
   Print "The value of variable <yarg$> is "; @yarg$
   Print "The value of variable <tvar> is "; @tvar
   @tvar = 99
   Print "The value of variable <tvar> is reset to "; @tvar
   @Result = @xarg + @tvar
   rem
   rem The following is considered a recursive call:
   rem Print "The Function should return "; TestFnc
   rem Instead of using the above, use the following:
   Print "The Function should return "; @Result
   return @Result
