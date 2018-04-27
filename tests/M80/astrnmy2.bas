10 '============================================================== 
20 ' ADAPTED FROM ASTRONMY.LBR, author unknown
30 ' Adaption: 21 Jul 87, Jim Lill
40 ' Combining the 3 individual programs into one with menu
50 ' Other changes: - Added better PI value technique
60 '                - Added C_LS$
70 '                - correct SOP error in DEC-RA (line 1560)
80 '===============================================================
100 LAT = 43.5 : ' LAT is your Latitude in Decimal Degrees 
110 '==============================================================
120  PI = 4 * ATN(1) : RAD = 180/PI 
130 '==============================================================
140 ' The following are used in menu 2 but are placed here for convenience 
150 ' K1,K2, AND K3 ARE CONSTANTS. K1 CHANGES EACH YEAR.
160 ' LONG IS YOUR LONGITUDE IN DECIMAL HOURS
170 ' TZ IS THE DIFFERENCE IN HOURS BETWEEN YOUR
180 ' TIME ZONE AND GREENWICH.
190 ' GMST IS GREENWICH MEAN SIDEREAL TIME
200 ' LMST IS LOCAL MEAN SIDEREAL TIME
210 '
220  K1 = 6.606550000000003 : ' This is  1987
230  K2 =  .06570982320000008     
240  K3 = 1.0027379093 
250  LONG = 5.1234
260  TZ = 5
270 '================================================================
280 ' Start of actual program........
290 ' MENU:
300 PRINT C_LS$
310 PRINT "Astronomy Programs":PRINT
320 PRINT "1- Find Altitude and Azimuth, given Right Ascension and Declination"
330 PRINT "2- Find Local Sidereal Time, given Day of Year and Local Time"
340 PRINT "3- Find Right Ascension and Declination, given ALT, AZ and LST"
350 PRINT "4- QUIT Program, Return to MBASIC":PRINT
360 INPUT "Enter Choice ==> ";CHOICE
370 IF CHOICE = 1  THEN GOTO 420
380 IF CHOICE = 2  THEN GOTO 950
390 IF CHOICE = 3  THEN GOTO 1270                         
400 IF CHOICE = 4  THEN STOP
410 GOTO 360
420 ' ************************ MENU CHOICE 1 ***********************
430 ' ------ALT/AZ CONVERSION -----------
440 '
450 ' -------INPUT OBJECT'S POSITION ---------
460 '
470 PRINT C_LS$
480 PRINT "Find Altitude and Azimuth":PRINT:PRINT
490 PRINT "RA (HOURS)";
500 INPUT RH
510 PRINT "RA (MINUTES)";
520 INPUT RM
530 PRINT "RA (SECONDS)";
540 INPUT RS
550  RA = RH + (RM/60) + (RS/3600)
560 PRINT
570 PRINT "DEC (DEGREES)";
580 INPUT DD
590 PRINT "DEC (MINUTES)";
600 INPUT DM
610 PRINT "DEC (SECONDS)";
620 INPUT DS
630  DEC = DD + (DM/60) + (DS/3600)
640 '
650 ' -------INPUT LMST & GET HA -----------
660 '
670 PRINT
680 PRINT "LMST (DECIMAL HOURS)";
690 INPUT LMST
700 PRINT
710  HA = LMST - RA
720  HAD =HA *15
730 '
740 ' ------ CONVERT TO RADIANS ----------
750 '
760  L = LAT / RAD
770  H = HAD / RAD
780  D = DEC / RAD
790 '
800 ' ---------- CALCULATE ALT / AZ --------
810 '
820  A = ( SIN (D) * (L)) + (COS (D) *COS (L)*COS (H))
830  AA = ATN (A/SQR(-A * A +1))
840  ALT = AA*RAD
850  AZ = (SIN (D) - (SIN (L)* SIN (A))) / (COS (L) * COS (A))
860  AZA = - ATN(AZ/SQR (-AZ*AZ+1))+1.5708
870  AZM = AZA*RAD
880 IF SIN (H) >0 THEN AZM = 360 - AZM
890 '
900 ' -------- PRINT OUT RESULTS ------
910 '
920 PRINT "     ALTITUDE = " ; ALT
930 PRINT "      AZIMUTH = " ; AZM
940 PRINT:PRINT:INPUT "<cr> to Continue";CR:GOTO 290
950 ' ****************** MENU CHOICE 2 *******************
960 ' ------ LOCAL MEAN SIDEREAL TIME ------
970 '
980 ' The constants were moved from here to beginning of program
990 ' ----- INPUT VARIABLES ------
1000 '
1010 PRINT C_LS$
1020 PRINT "Find Local Sidereal Time, LST":PRINT:PRINT
1030 PRINT "DAY OF YEAR " ;
1040 INPUT D
1050 PRINT "LOCAL TIME (HOURS)";
1060 INPUT H
1070 PRINT "LOCAL TIME (MINUTES)";
1080 INPUT M
1090 PRINT "LOCAL TIME (SECONDS)";
1100 INPUT S
1110 '
1120 ' ------ CALCULATE QUANTITIES ------
1130 '
1140 PRINT
1150  SDT = H +  ( M / 60 ) + (S / 3600)
1160  UT = SDT + TZ
1170  GMST = K1 + (K2*D) + (K3*UT)
1180 IF GMST > 24 THEN GMST = GMST - 24 : GOTO 1180
1190  LMST = GMST - LONG
1200 '
1210 ' ----- PRINT RESULTS ------
1220 '
1230 PRINT "LOCAL MEAN TIME =" ; SDT
1240 PRINT "           GMST = " ; GMST
1250 PRINT "           LMST = " ; LMST
1260 PRINT:PRINT:INPUT "<cr> to Continue";CR:GOTO 290
1270 ' *************** MENU CHOICE 3 *********************
1280 ' ADAPTED FROM RA-DEC.BAS
1290 ' ---------- RA & DEC CALCULATION -----
1300 '
1310 '
1320 PRINT C_LS$
1330 PRINT "Find Right Ascension and Declination":PRINT:PRINT
1340 PRINT "ALTITUDE";
1350 INPUT ALT
1360 PRINT "AZIMUTH";
1370 INPUT AZ
1380 IF AZ > 180 THEN AZ = 360 - AZ :  FLAG = 1
1390 PRINT "LOCAL SIDEREAL TIME (DECIMAL HOURS)";
1400 INPUT LMST
1410 '
1420 ' ------ CONVERT TO RADIANS ------         
1430 '
1440  A = ALT / RAD
1450  AZR = AZ / RAD
1460  L = LAT / RAD
1470 '
1480 ' ------ CALCULATE RA AND DEC ------
1490 '
1500 ' D is sin(declination),  A is altitude, L is latitude, AZR is azimuth
1510  D = (SIN(A)* SIN (L) ) + (COS (A) * COS (L) * COS (AZR) )
1520   DC = ATN (D / SQR ( - D * D + 1 ) )
1530 ' DC is declination
1540  H = (SIN (A) - (SIN (L) * D)) / (COS (L) * COS (DC))
1550 ' H is cos(hour-angle) 
1560  HC = -ATN (H / SQR (-H * H + 1)) + PI/2
1570 ' HC is the hour-angle 
1580  DEC = DC * RAD
1590  HA = HC * RAD
1600 IF FLAG = 1 GOTO 1620 
1610 IF SIN (HC) >0 THEN HA = 360 - HA
1620  HAH = HA / 15
1630  RA = LMST - HAH
1640 IF RA <0 THEN RA = RA +24: GOTO 1640
1650 '
1660 ' ------ PRINT OUT RESULT ------
1670 '
1680 PRINT
1690 PRINT "RA = ";RA
1700 PRINT "DEC = ";DEC
1710 PRINT:PRINT:INPUT "<cr> to Continue";CR:GOTO 290
