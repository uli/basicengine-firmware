10 REM   Program to locate television satellites for a given latitude
20 REM           and longitude.  This program is useful for TVRO owners.
30 REM   Additional satellites can be placed in the data table.
40 REM
50 REM        P.G. Wohlmut   Copyright 1983    July 8, 1983
60 REM
70 PRINT
100 PI=3.141592654000001
110 DIM SAT$(50),ANGLE(50),CITY$(100),LAT(100,3),LONG(100,3)
120 FOR I= 1 TO 50:READ SAT$(I):IF SAT$(I)="end" THEN 181
130 READ ANGLE(I):NEXT I
140 DATA "Satcom F5",143,"Satcom 1R",139,"Satcom F1",135,"Satcom F3",131,"Comstar D4",127
150 DATA "Westar 5",123,"Satcom F2",119,"Anik A3",114,"Anik B",109,"Anik D1",104
160 DATA "Westar 4",99,"Comstar D1/D2",95,"Westar 3",91,"Comstar D3",87
170 DATA "Satcom F4",83,"Westar 1/2",79
180 DATA "end"
181 T_OPS=I-1
185 FOR I= 1 TO 100:READ CITY$(I):IF CITY$(I)="end" THEN 187 
186 FOR J=1 TO 3:READ LAT(I,J):NEXT J:FOR J=1 TO 3:READ LONG(I,J):NEXT J:NEXT I
187 CIT=I-1
190 RESTORE
210 PRINT C_LS$:FOR I= 1 TO CIT STEP 20:FOR J= 1 TO 20:K=I+J-1:IF CITY$(K)="end" THEN 212 
211 PRINT USING "###: \                               \";J,CITY$(K):NEXT J
212 INPUT "Which city # (#,m=more) ";Z$
213 IF Z$="m" OR Z$="M" THEN CLS:GOTO 218 ELSE Z=VAL(Z$):IF Z<1 OR Z>20 THEN 218
214 Z=Z+I-1:DT=LAT(Z,1):MT=LAT(Z,2):ST=LAT(Z,3):DG=LONG(Z,1):MG=LONG(Z,2):SG=LONG(Z,3):CIT$=CITY$(Z):GOTO 240
218 NEXT I:LOCATE 24,1:PRINT "No more cities :";
220 INPUT"Your Latitude (dd,mm,ss) ";DT,MT,ST
230 INPUT"Your Longitude (dd,mm,ss) ";DG,MG,SG
240 SITELONG=DG+MG/60+SG/3600:SITELAT=DT+MT/60+ST/3600
250 SITELONG = SITELONG*PI/180:SITELAT=SITELAT*PI/180
260 PRINT C_LS$:PRINT "======================================="
270     PRINT ":  SATELLITE  : ELEVATION :  AZIMUTH  :"
280     PRINT ":             :  DD'MM'SS :  DD'MM'SS :"
290     PRINT "======================================="
300 FOR NUM=1 TO T_OPS
310 SATLONG=ANGLE(NUM)*PI/180:GOSUB 420
320 ELD=INT(EL):ELM1=EL-ELD:ELM=INT(ELM1*60):ELS=INT(ELM1*3600-ELM*60+.5)
330 AZD=INT(AZ):AZM1=AZ-AZD:AZM=INT(AZM1*60):AZS=INT(AZM1*3600-AZM*60+.5)
340 PRINT USING ":\           \: ###'##'## : ###'##'## :";SAT$(NUM),ELD,ELM,ELS,AZD,AZM,AZS
350 NEXT NUM
360     PRINT "======================================="
370 PRINT USING "Latitude    ###'##'##";DT,MT,ST;
380 PRINT USING "Longitude   ###'##'##";DG,MG,SG;
382 PRINT
384 PRINT CIT$;
386 PRINT
390 PRINT "Another location? ";
400 INPUT Z$:IF Z$="" THEN 400 ELSE IF Z$="Y" OR Z$="y" THEN PRINT Z$;:GOTO 210
410 END
420 ' calculate azimuth & elevation of satellite at site.
430 EL = ATN((COS(SATLONG-SITELONG)*COS(SITELAT)-.15126)/SQR(1-(COS(SATLONG-SITELONG)*COS(SITELAT))^2))*180/PI
440 AZ=180+ATN(TAN(SATLONG-SITELONG)/SIN(SITELAT))*180/PI
450 RETURN
1000 DATA "Albuquerque",35,0,0,106,40,0
1100 DATA "Anchorage",61,10,0,150,0,0
1200 DATA "Atlanta",33,30,0,84,20,0
1300 DATA "Bakersfield",35,30,0,120,10,0
1400 DATA "Baltimore",39,15,0,76,40,0
1500 DATA "Bangor",44,45,0,68,45,0
1600 DATA "Birmingham",33,30,0,86,45,0
1700 DATA "Bismarck",46,45,0,100,45,0
1800 DATA "Boston",42,20,0,71,5,0
1900 DATA "Buffallo",43,0,0,78,50,0
2000 DATA "Butte",46,0,0,112,30,0
2100 DATA "Charleston",38,20,0,81,35,0
2200 DATA "Chicago",41,10,0,87,40,0
2300 DATA "Cleveland",41,20,0,81,40,0
2400 DATA "Dallas",33,0,0,97,0,0
2500 DATA "Denver",39,45,0,105,0,0
2600 DATA "Des Moines",41,30,0,93,40,0
2700 DATA "Detroit",42,30,0,83,0,0
2800 DATA "Elko",40,50,0,115,40,0
2900 DATA "Eureka",40,45,0,124,10,0
3000 DATA "Flagstaff",35,25,0,111,30,0
3100 DATA "Fresno",36,55,0,119,45,0
3200 DATA "Hartford",41,45,0,72,40,0
3300 DATA "Honolulu",21,20,0,157,50,0
3400 DATA "Houston",29,50,0,95,10,0
3500 DATA "Indianapolis",39,45,0,86,10,0
3600 DATA "Jackson",32,15,0,90,10,0
3700 DATA "Jacksonville",30,15,0,81,40,0
3800 DATA "Kansas City",39,0,0,94,30,0
3900 DATA "Las Vegas",36,10,0,115,10,0
4000 DATA "Little Rock",34,45,0,92,15,0
4100 DATA "Los Angeles",34,0,0,118,15,0
4200 DATA "Louisville",38,10,0,85,45,0
4300 DATA "Miami",25,45,0,80,15,0
4400 DATA "Milwaukee",43,0,0,88,0,0
4500 DATA "Minneapolis",45,0,0,93,15,0
4600 DATA "Nashville",36,15,0,86,45,0
4700 DATA "New Orleans",30,0,0,90,0,0
4800 DATA "New York",40,45,0,74,0,0
4900 DATA "Omaha",41,15,0,96,0,0
5000 DATA "Orlando",28,35,0,81,30,0
5100 DATA "Philadelphia",40,0,0,75,0,0
5200 DATA "Phoenix",33,30,0,112,5,0
5300 DATA "Portland",45,30,0,122,35,0
5400 DATA "Providence",41,45,0,71,15,0
5500 DATA "Reno",39,30,0,119,45,0
5600 DATA "Sacramento",38,40,0,121,30,0
5700 DATA "Saint Louis",38,40,0,90,10,0
5800 DATA "Salt Lake City",40,50,0,112,0,0
5900 DATA "San Diego",33,0,0,117,10,0
6000 DATA "Seattle",47,30,0,122,15,0
6100 DATA "San Francisco",37,40,0,122,30,0
6200 DATA "San Jose",37,15,0,121,50,0
6300 DATA "Tampa",28,0,0,82,30,0
6400 DATA "Tucson",32,10,0,111,35,0
6500 DATA "Tulsa",36,10,0,96,0,0
6600 DATA "Washington DC",38,55,0,77,5,0
6700 DATA "Winston Salem",36,10,0,80,15,0
6800 DATA "end"
7000 REM --------------------------
7010 REM Fixed overlapping structured commands
7020 REM Changed 400 Z$=INKEY$ to 400 INPUT Z$