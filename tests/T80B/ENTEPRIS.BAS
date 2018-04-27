150 CLEAR '300:DEFINT A-Z  
160 READ A$:IF A$="END" THEN STOP  
170 IF A$="R" THEN PRINT:GOTO 160  
180 READ B$:IF B$="END" THEN STOP  
190 FOR N=1 TO LEN(B$):C$=MID$(B$,0)  
200 IF C$="Q" THEN B$=MID$(B$,0,N-1)+CHR$(91)+MID$(B$,N)  
210 IF C$="W" THEN B$=MID$(B$,0,N-1)+CHR$(93)+MID$(B$,N)  
220 IF C$="A" THEN B$=MID$(B$,0,N-1)+CHR$(92)+MID$(B$,N)  
230 IF C$="M" THEN B$=MID$(B$,0,N-1)+CHR$(44)+MID$(B$,N)  
240 IF C$="C" THEN B$=MID$(B$,0,N-1)+CHR$(58)+MID$(B$,N)  
250 IF C$="S" THEN B$=MID$(B$,0,N-1)+CHR$(34)+MID$(B$,N)  
260 NEXT N:IF LEFT$(A$,1)="S" THEN 280  
270 PRINT TAB(1+VAL(MID$(A$,1)));B$;:GOTO 290  
280 PRINT STRING$(VAL(MID$(A$,1))," ");B$;  
290 READ A$:IF LEFT$(A$,1)="T" THEN 190  ELSE  170  
1000 DATA "T22","###","R","T21","Q#  ##","R","T22","##  M#","R","T22","#    M#"
1010 DATA "R","T22","#----M#","R","T22","#    !#","R","T22","#    M#","R","T22"
1020 DATA "# QW M#","R","T22","# QW M#","R","T22","# QW M#","R","T22","#----M#"
1030 DATA "R","T22","#    !#","R","T22","#    M#","R","T21","*#    M#","R","T21"
1040 DATA "*#    M#","R","T20","/*#    M#","R","T20"
1050 DATA "* #    M#","R","T19","/* #----M#","R","T19"
1060 DATA "*  #    A#A","R","T19","*  #    M#*","R","T19","* /#    M#*"
1070 DATA "R","T18","/*/ #    M#O*","R","T18","*/  #    M#O*","R","T18"
1080 DATA "*   #    M#O*","R","T18","*  /#    M#O *","R","T18"
1090 DATA "* / #    M#O *","R","T18","*/  #    M#O  *","R","T17"
1100 DATA "END"

