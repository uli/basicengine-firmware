10 REM   TRACKING TOLERANCES
12 REM
14 R=3.14159265/180: REM RAD/DEG
16 K=206265: REM  ARC SEC/RAD
18 INPUT "WITH DRIVE (Y OR N)";Q$
20 IF Q$="Y" THEN 26
22 INPUT "DECLINATION (DEG)";D
24 M=360*COS(D*R): GOTO 30
26 INPUT "MOTION (DEG/DAY)";M
28 IF M=0 THEN 26
30 INPUT "E.F.L. (MM)";F
32 INPUT "FILM GRAIN (F OR C)";A$
34 IF A$<>"F" AND A$<>"C" THEN 32
36 INPUT "ENLARGEMENT FACTOR";E
38 INPUT "PLANNED EXP (MIN)";T
40 S=1/(R*F*E): REM DEG/MM
42 R1=M/(S*24*60): REM MM/MIN
44 IF A$="F" THEN G=K/(F*100)
46 IF A$="C" THEN G=K/(F*20)
48 PRINT
50 PRINT "MOTION IN ";T;" MIN EXP:"
52 PRINT "  ";INT(T*R1*S*3600+0.5);
54 PRINT " ARC SEC, OR"
56 PRINT "  ";T*R1;" MM ON PRINT"
58 PRINT "LIMITS BASED ON GRAIN:"
60 PRINT "   MAX EXPOSURE ";
62 PRINT G/(S*3600*R1);" MIN"
64 PRINT "   RESOLUTION ";
66 PRINT INT(G+0.5);" ARC SEC"
68 END
70 REM  ------------------------
80 REM  APPEARED IN ASTRONOMICAL
90 REM  COMPUTING, SKY & TELE-
95 REM  SCOPE, FEBRUARY, 1986
99 REM  ------------------------
