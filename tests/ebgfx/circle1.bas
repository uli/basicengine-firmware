t$="circle1"
screen 14
for i=0 to 15
  circle i*26, i*14, 50, rgb(i*15, 255 - i*15, i*7)
  circle i*26, i*14+60, 50, rgb(i*15, i*15, i*15), rgb(255 - i*15, i*15, i*7)
  circle 30, 160, 50, rgb(255, 255, 255)
  circle 90, 160, 50, rgb(255, 255, 255), rgb(255, 255, 255)
next
chain "_check.chn"
