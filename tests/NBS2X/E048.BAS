00100 REM ----------------------------------------------------
00110 REM FILENAME:  E048.BAS
00120 REM  PURPOSE:  Verify DELETE <non-existing-line-number>
00130 REM   AUTHOR:  Howard Wulf AF5NE
00140 REM     DATE:  June 12th, 2015
00150 REM ----------------------------------------------------
01000 PRINT "LINE 02000 SHOULD NOT CAUSE A FATAL ERROR"
02000 DELETE 3000
09000 PRINT "*** TEST PASSED ***"
09999 END
