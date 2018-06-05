10 testdir$="/sd/tests"
15 ttal=0:failed=0:passed=0
16 IF RET$(0)<>"" THEN
17   CALL proc_dir(RET$(0))
18   GOTO 105
19 ENDIF
20 OPEN testdir$ FOR DIRECTORY AS #12
30 d$=DIR$(12)
40 DO 
50   IF RET(1)=1 THEN 
55     PRINT "==DIR ";d$
60     CALL proc_dir(d$)
70   ENDIF 
80   d$=DIR$(12)
90 LOOP UNTIL d$=""
100 CLOSE 12
105 PRINT "Total ";ttal,"Passed ";passed,"Failed ";failed
110 END
120 PROC proc_dir(d$)
135 IF @d$="GAMES" THEN RETURN 
140 @d$=testdir$+"/"+@d$
150 OPEN @d$ FOR DIRECTORY AS #13
160 @f$=DIR$(13)
170 DO 
180   @i=INSTR(@f$,".bas")
190   IF @i=-1 THEN @i=INSTR(@f$,".BAS")
200   IF @i>=0 THEN 
210     PRINT "  ";@f$,
212     REM IF LEFT$ (@f$,1)<>"w"THEN 380
230     @of$=@d$+"/"+LEFT$ (@f$,@i)+".out"
232     @rn$=@d$+"/"+LEFT$ (@f$,@i)+".run"
240     OPEN @of$ FOR OUTPUT AS #14
260     @if$=@d$+"/"+LEFT$ (@f$,@i)+".inp"
269     ON ERROR GOTO 279
270     OPEN @if$ FOR INPUT AS #15
274     CMD INPUT 15
279     CMD OUTPUT 14
285     @tmp=RND(-1) 'seed RNG for reproducible runs
288     ON ERROR GOTO 330
289     @fr=FREE()
290     EXEC @d$+"/"+@f$
291     TROFF:@fr2=FREE()
300     CMD OFF 
310     CLOSE 14:REM CLOSE 15
312     REM close15
320     IF 0=1 THEN 
330       err=RET(0)
340       erl=RET(1)
345       errsub=RET(2)
350       PRINT ERROR$ (err);" (";err;")@";errsub
355       CMD OFF 
356       CLOSE 14:REM CLOSE 15
360     ENDIF 
361     ttal=ttal+1
362     IF COMPARE (@of$,@rn$)=0 THEN 
363       PRINT " \f9cPASS\f0f"
364       REMOVE @of$
365       passed=passed+1
366     ELSE 
367       PRINT " \fe3FAIL\f0f"
368       failed=failed+1
369     ENDIF 
370   ENDIF 
380   @f$=DIR$(13)
381   REM ?FREE(),@fr2-@fr
382   REM IF @fr2<@fr THEN PRINT "BASTARD!":STOP
390 LOOP UNTIL @f$=""
400 CLOSE 13
420 RETURN 
430 CHDIR "tests/B15A"
