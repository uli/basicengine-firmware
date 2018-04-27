100 rem Purpose: Verify GOSUB X
110 rem  Author: Howard Wulf, AF5NE
120 rem    Date: 2015-05-22
130 rem
135 print "ENTER A NUMBER (-999 TO END)? ";
140 input A
145 if a=-999 then 9999
150 let A=INT(A)
152 if A < 1 then 135
154 if A > 5 then 135
156 A=1000+A*100
158 gosub A
160 goto 135
1100 print "1"
1110 return
1200 print "2"
1210 return
1300 print "3"
1310 return
1400 print "4"
1410 return
1500 print "5"
1510 return
9999 end
