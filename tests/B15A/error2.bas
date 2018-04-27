100 REM Test ERROR( X )
105 REM NOTE: ERROR 0 should NOT cause any error whatsoever (it clears the error status)
110 ON ERROR GOTO 200
120 FOR i=0 TO 59
125   REMprint "i="; i
130   ERROR i
134   REM print "RESUME NEXT should have cleared ERR"; ERR
135   REM if ERR <> 0 goto 999
140 NEXT i
150 GOTO 999
200 PRINT "ERL=";RET(1);", ERR=";RET(0);", ERR$=";ERROR$ (RET(0))
202 IF RET(1)<>130 GOTO 999 
205 IF RET(0)=0 GOTO 999 
206 ON ERROR GOTO 200
210 RESUME 
999 END
