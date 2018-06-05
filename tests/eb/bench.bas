5 start=TICK()
10 a=0
20 FOR i=1 TO 100000
30 a=(23*a+33-a)MOD 10000
40 NEXT i
50 ?a
60 if TICK()-start<2100 then
70   PRINT "good enough"
80 ELSE
90   PRINT "performance regression"
100 ENDIF
