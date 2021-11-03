10 for i=1 to 10
20 if i=3 then exit for
30 print i
40 next
50 print "end for"
60 for j=1 to 10
70 print j
80 if j=4 then exit for j
90 next j
95 print "end for j"
100 for k=1 to 10
110 for l=1 to 5
120 print l
130 if k=3 then exit for k
140 next
145 print "end for l"
150 next k
160 print "end for k"

do
  print "loop"
  exit do
  print "wtf?"
loop

a=1
while a<5
  print "while ";a;
  a=a+1
  if a=3 then exit while
  print "!"
wend

for m = 1 to 10
for n = 1 to 10
exit for
next m
print "never!"
next
next m
