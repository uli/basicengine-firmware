1 ' IF cascade
2 CALL ip(1):CALL ip(2):CALL ip(3):END
10 PROC ip(a)
20 IF @a=1 THEN 
30   PRINT "eins"
40 ELSE IF @a=2 THEN 
50   PRINT "zwei"
60 ELSE 
70   PRINT "viele"
80 ENDIF 
90 RETURN
