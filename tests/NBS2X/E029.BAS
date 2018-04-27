100 rem ------------------------------
110 rem File: if1.bas
120 rem Purpose: Verify IF/THEN/ELSE
130 rem ------------------------------
200 let i = 1
210 if i then 230 else 250
220 error 1
230 print "TEST #1 PASSED"
240 goto 300
250 print "TEST #1 FAILED"
260 error 1
300 let i = 0
310 if i then 330 else 350
320 error 1
330 print "TEST #2 FAILED"
340 error 1
350 print "TEST #2 PASSED"
360 goto 400
400 let i = 1
410 if i then 
      goto 430 
    else
      goto 450
    end if
420 error 1
430 print "TEST #3 PASSED"
440 goto 500
450 print "TEST #3 FAILED"
460 error 1
500 let i = 0
510 if i then
      goto 530
    else
       goto 550
    end if
520 error 1
530 print "TEST #4 FAILED"
540 error 1
550 print "TEST #4 PASSED"
560 goto 600
600 rem
700 rem
800 rem
900 print "*** ALL TESTS PASSED ***"
999 END
