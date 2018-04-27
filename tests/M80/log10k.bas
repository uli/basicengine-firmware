10 '**************************************
20 ' T.A. Elkins Log(10k!) Benchmark
30 ' -------------------------------
40 '
50 ' Computes number of decimal digits in
60 ' 10,000 factorial.  (Exact answer is
70 ' 35660.)
80 '
90 ' See COMPUTERWORD, April 20, 1987, for
100 ' more information.
110 '
120 ' Translated to CP/M MBASIC 5.0
130 ' Jim Lill 19 July 87
140 '
150 '**************************************
160 '
170 ' IBM PC Results:
180 '
190 ' Compiler/     Code  Time
200 ' Intrepreter   Size  (sec)  Error
210 ' ------------  ----  -----  -----
220 ' TrueBASIC      82K    3.5
230 ' Better BASIC   48K   10
240 ' Quick BASIC    27K   56
250 ' GW-BASIC       n/a  149
260 '
270 ' (Error results were not given.)
280 '
290 ' Amiga Results:
300 '
310 ' AmigaBASIC     n/a  103    0.5457
320 '
330 ' CP/M Results:
340 '
350 ' MBASIC 5.0 \
360 ' @ 10MHz     \  n/a  110    0.5462
370 ' @  6MHz     /  n/a  184    0.5462
380 '**************************************
390 'DEFINT I
400 'DEFDBL E,X,Y,Z
410 Z=10
420 X=0
430 EXACT = 35660
440 PRINT "Elkins Log(10k!) Benchmark"
450 PRINT
460 PRINT "Note the Start Time!";CHR$(7)
470 FOR I=2 TO 10000
480  Y=I
490  X=X+LOG(Y)
500 NEXT I
510 PRINT "Note the End Time!";CHR$(7)
520 PRINT
530 PRINT "Error: ";EXACT - X/LOG(Z)
540 END
