10 ' make sure CALL parameters that contain function evaluations
20 ' do not overwrite previously evaluated parameters
90 DIM cols(11,20)
100 GOTO 900
270 PROC setfield(of,v)
272 cols((@of+1) MOD 40,@of/40)=@v
276 RETURN 
280 PROC getfield(of)
282 RETURN cols((@of+1) MOD 40,@of/40)
900 FOR x=0 TO 11:FOR y=0 TO 20
910     cols(x,y)=x*1000+y
920   NEXT:NEXT 
1000 FOR x=0 TO 9:FOR yy=640 TO 40 STEP -40
1010     CALL setfield(yy+x,FN getfield(yy+x-40))
1020     IF FN getfield(yy+x-40)<>FN getfield(yy+x) THEN STOP 
1030   NEXT:NEXT 
1040 PRINT "we're fine"
