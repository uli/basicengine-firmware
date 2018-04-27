10 REM    ALTITUDE AND AZIMUTH
12 REM
14 P=3.14159265: R1=P/180
16 INPUT "R A (H,M,S) ";A$,A2,A3
18 GOSUB 72: R=A*15*R1
20 INPUT "DEC (D,M,S) ";A$,A2,A3
22 GOSUB 72: D=A*R1
24 INPUT "LAT, LONG   ";B,L
26 B=B*R1: L=L*R1
28 INPUT "GST (H,M,S) ";A$,A2,A3
30 GOSUB 72: T=A*15*R1
32 T5=T-R+L: REM  LHA
34 S1=SIN(B)*SIN(D)
36 S1=S1+COS(B)*COS(D)*COS(T5)
38 C1=1-S1*S1
40 IF C1>0 THEN C1=SQR(C1)
42 IF C1<=0 THEN 46
44 H=ATN(S1/C1): GOTO 48
46 H=SGN(S1)*P/2
48 C2=COS(B)*SIN(D)
50 C2=C2-SIN(B)*COS(D)*COS(T5)
52 S2=-COS(D)*SIN(T5)
54 IF C2=0 THEN A=SGN(S2)*P/2
56 IF C2=0 THEN 62
58 A=ATN(S2/C2)
60 IF C2<0 THEN A=A+P
62 IF A<0 THEN A=A+2*P
64 PRINT
66 PRINT "ALTITUDE: ";H/R1
68 PRINT "AZIMUTH:  ";A/R1
70 END
72 REM  SEXAGESIMAL TO DECIMAL
74 REM
76 S=1: A1=ABS(VAL(A$))
78 IF LEFT$(A$,1)="-" THEN S=-1
80 A=S*(A1+A2/60+A3/3600)
82 RETURN
84 REM  ------------------------
86 REM  APPEARED IN ASTRONOMICAL
88 REM  COMPUTING, SKY & TELE-
90 REM  SCOPE, JUNE, 1984
92 REM  ------------------------

