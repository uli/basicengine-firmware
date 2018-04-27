Call Main
End

Proc Prior
   Print "This is a subroutine prior to MAIN."
   Print "This should not print."
   Return

Proc Main
   Print "This is the MAIN subroutine."
   Print "This should print."
   Return

Proc Subsequent
   Print "This is a subroutine subsequent to MAIN."
   Print "This should not print."
   Return

