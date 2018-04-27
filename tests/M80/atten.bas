10	'ATTENUATOR DESIGN PROGRAM
12	'Written in MICROSOOFT BASIC 7/31/82 by
14	' Trevor Marshall, SYSOP, Thousand Oaks Technical RBBS
16	'
20	'This program designs T, PI, or minimum loss attenuators
30	'	T		PI	 Min Loss Z1>Z2      Z1<Z2
40	' ---R1----R2--    -----R3-----     ----R1----      ----R1----
50	'       |           |        |              |         |
60	'      R3           R1       R2            R2         R2
70	'       |           |        |              |         |
80	' -------------    ------------      ---------      ----------
90	'
100	INPUT "What attenuator do you want, TEE, PI or MINIMUM LOSS..";A$
105 IF A$ = "QUIT" THEN GOTO 7999
110	PRINT:INPUT "What is the input impedance...";A
120	PRINT:INPUT "What is the output impedance..";B
130	IF LEFT$(A$,1)="M" OR LEFT$(A$,1)="m" THEN GOTO 3000
140	PRINT:INPUT "What is the required loss (in dB)...";X
150	GOSUB 6000
160	IF R2>X THEN GOTO 2600		'Loss too low
170	PRINT "Asymmetrical"
172	IF LEFT$(A$,1)="P" OR LEFT$(A$,1)="p" THEN GOTO 1000
180	PRINT "Tee Section" : PRINT
190	PRINT "--R1-----R2--"
200	PRINT "      |"
210	PRINT "      R3"
220	PRINT "      |"
230	PRINT "-------------" : PRINT : GOTO 2000
1000	PRINT "   PI Section" :PRINT
1010	PRINT "------R3------"
1020	PRINT " |          |"
1030	PRINT " R1         R2"
1040	PRINT " |          |"
1050	PRINT "--------------"
2000	PRINT "Source Impedance=";A;: PRINT ",Terminating Impedance=";B;
2010	PRINT "Loss (dB)=";X
2020	Y=10^(X/10) : IF LEFT$(A$,1)="P" OR LEFT$(A$,1)="p" THEN GOTO 2500
2040	R3=SQR(Y*A*B)*2/(Y-1)
2060	PRINT "R1=";A*((Y+1)/(Y-1))-R3;
2080	PRINT ",R2=";B*((Y+1)/(Y-1))-R3;
2090	PRINT ",R3=";R3 : PRINT : PRINT : GOTO 10
2500	R3=((Y-1)/2)*SQR(A*B/Y)
2510	PRINT "R1=";1/(((Y+1)/(A*(Y-1)))-1/R3);
2520	PRINT ",R2=";1/(((Y+1)/(B*(Y-1)))-1/R3);
2530	PRINT ",R3=";R3 : PRINT : PRINT : GOTO 10
2600	PRINT "Specified loss is too low..adjusting to...."
3000	PRINT "MINIMUM LOSS PAD DESIGN" : PRINT
3010	IF A>B THEN GOTO 5000
3020	PRINT "Pad for Z2 > Z1" : PRINT
3030	PRINT "--------R1----"
3040	PRINT "    |"
3050	PRINT "    R2"
3060	PRINT "    |"
3070	PRINT "--------------" : PRINT
4000	PRINT "Source Impedance=";A;: PRINT ",Terminating Impedance=";B
4010	GOSUB 6000
4020	PRINT "R1=,";R0*SQR(1-R1/R0);: PRINT ",R2=";R1/(SQR(1-R1/R0))
4030	PRINT "Loss (dB)=";R2 : PRINT : PRINT : GOTO 10
5000	PRINT "Pad for Z1 > Z2" : PRINT
5010	PRINT "----R1--------"
5020	PRINT "         |"
5030	PRINT "         R2"
5040	PRINT "         |"
5050	PRINT "--------------"
5060	GOTO 4000
6000	R0=A : R1=B : IF B>A THEN R0=B : IF B>A THEN R1=A
6010	R2=10*LOG((SQR(R0/R1)+SQR(R0/R1-1))^2)/2.30259 : RETURN
7000 REM ----------------------------------------------------------------
7010 REM Updated for bwBASIC 3.0 by Howard Wulf, AF5NE, May 7th 2015:
7020 REM a) Added line 105 for automated testing
7030 REM
7999 END

