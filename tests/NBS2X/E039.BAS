00100 REM ----------------------------------------------------
00110 REM FILENAME:  chain2.bas
00120 REM  PURPOSE:  Verify CHAIN <file-name>
00130 REM   AUTHOR:  Howard Wulf AF5NE
00140 REM     DATE:  October 10th, 2014
00160 REM ----------------------------------------------------
01000 'COMMON X$
01010 X$ = "abcdef"
01020 let A$ = "/sd/bwbasic-tests/NBS2X/E039.CHN"
01030 chain A$
03000 REM ----------------------------------------------------
03010 REM If we get to here, then the CHAIN failed
03020 REM ----------------------------------------------------
03030 ERROR 1
03040 STOP
