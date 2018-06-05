5 a=0
10 &tom:PRINT "tom"
20 GOSUB &jerry
25 a=a+1:IF a>10 THEN END
30 GOTO &tom
100 &jerry:
110 PRINT "jerry"
120 RETURN 
