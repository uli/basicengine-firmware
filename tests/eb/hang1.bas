1 b=TICK()
5 a=FRAME()
10 ?INKEY$;:IF FRAME()-a>0 THEN a=FRAME():rem PRINT "~";
15 if TICK()>b+500 then PRINT "still alive":END
20 GOTO 10
