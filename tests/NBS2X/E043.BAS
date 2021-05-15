00100 REM ----------------------------------------------------
00110 REM FILENAME:  merge1.bas
00120 REM  PURPOSE:  Verify MERGE <file-name>
00130 REM   AUTHOR:  Howard Wulf AF5NE
00140 REM     DATE:  October 10th, 2014
00160 REM ----------------------------------------------------
01000 let f$ = "NBS2X/E043.MRG"
01010 delete 2000-2090
01020 merge f$, 1030
01030 REM ----------------------------------------------------
01040 REM Lines 02000 thru 02090 are replaced by the merged file
01050 REM ----------------------------------------------------
02000 ERROR 1
02010 ERROR 1
02020 ERROR 1
02030 ERROR 1
02040 ERROR 1
02050 ERROR 1
02060 ERROR 1
02070 ERROR 1
02080 ERROR 1
02090 ERROR 1
03000 REM ----------------------------------------------------
03010 REM If we get to here, then the merge failed
03020 REM ----------------------------------------------------
03030 ERROR 1
03040 STOP
09900 REM ----------------------------------------------------
09910 PRINT "*** ALL TESTS PASSED ***"
09920 END
