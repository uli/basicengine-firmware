t$="rect1"
screen 14
for i=0 to 15
  rect i*26, i*14, i*26+50, i*14+20, rgb(i*15, 255 - i*15, i*7)
  rect i*26, i*14+60, i*26+50, i*14+60+20, rgb(i*15, i*15, i*15), rgb(255 - i*15, i*15, i*7)
  rect 0, 120, 50, 170, rgb(255, 255, 255)
  rect 60, 120, 110, 170, rgb(255, 255, 255), rgb(255, 255, 255)
next
chain "_check.chn"
