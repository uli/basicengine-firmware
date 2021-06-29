2rem Checks if all tokens are compiled and decompiled correctly.
3rem XXX: not complete yet...
4rem This code does is not supposed to be run, it merely serves to
6rem test toktoi() and putlist().
9list 10-:end
10goto 40:gosub 200:return
20for i=1to 20step 3:next i:step3=2
30if a=down then do:loop
40if a=up then
45if a=upthen then print printbesser
50else:endif:end'foo
60&bla:data"hans"
70read a$:restore&bla
80clt:wait 20:pokew 10,20:poked 50,60:poke 70,80:print"fertig"
90?"echt?"
100bla$=input$(1,2):input a:cls:color 10
110locate a,b:redraw:cscroll 5,5,6,6,1:pset 42,23,12:line 20,20,30,30
120rect c,d,e,f,0,-1:circle 20,f,pi,33,4,-1:
130blit hier,thereto to tosomewhere,somethere size sizex,sizeyup up
140gprint 1,towo,gprintstring$:gscroll vier,5,6,sieben_,2
150gpout blow,upp
160mkdir"mkdir":d$="mkdir":rmdir d$
170rename d$ to"nothing":copy"nofile"to nothere$
180type typ$+".txt"
190dim viel(40)
200dimmehr=23
210palette 0
220palette0=42
230viel=fnviel(23)
240on error goto home
250onerror=40
260?a$
270stop:stopp$="collaborate"
280tron:tronn=40:troff
290append ~top$,"ende"
300prepend ~list$,"anfang"
310search"text"
320@b=char(@c,4)
330vpoke 20,30
340sys tem
350systm=1
360while whiler=4:?error$(42):wend
370print something$+chr$(asc("!"))
380x=sin(pi)+sins(666)
390x=argc(argctype)*arg(0)+abs(1)-sin(2)/cos(coss)^exp(42)+atn2(2,3)+atn(1):y=sqr(4)
400zztop=tan(tannen)mod modulo<<sgn(sign)>>map(3,3,5,x,8)/free()
410leta=letter:let a=rnd(2)+inkey()*len(cwd$())+tick()+peekw(23)
420call procedure(peekd(200), peekw(300), peekd(400))
430pointer=point(pointerx,pointery)+pad(0)+pad(padd)+key(keyup+up)
440ptr=rgb(red,green,1)+csize(0)+psize(1)+frame()
450bg0=0
460bg bg0 tiles tilesw,tilesh pattern patternx,patterny,patternw size sizex,sizey window windowx,windowy,windoww,windowh prio priority on
470beep beep30:beep 30:play music$+"ccc":play playchan,"cdefg"
480sound 0,10,20,30:date
490load pcxprogram$:load pcx"picture.pcx":load pcx pcxfile$
490save pcxprogram$:save pcx"picture.pcx":save pcx pcxfile$
500merge "program.bas":merge merger$
510bload bloadfile$,addr:bload"bload.bin",addr,length:bload "bload.bin",0
510bsave bloadfile$,addr:bsave"bload.bin",addr,length:bsave "bload.bin",0
520clear:new:proc procedure(eins,zwei,torei):return @eins+@zwei+@torei
530chain chainloader$:chain"program.bas"
540files:files filesss$:files"*.bas"
550config config1,value1:config 0,0
560sysinfo:screen 1:screen 2^2:screen mode
570window window1,window2,window3,window4
580font font1:font 1
590sprite sprite0 pattern patternx,patterny,sin(42) size sizex,sizey frame 4,framey flags flags1 key key1 prio priority_s off
600move sprite 0 to tox,toy
610plot bg0,tix,tiy,0:plot 0,2,2,tile$
620plot 0 map fromtile to totile
630edit"file.txt":edit file$
640 border wall,berlin
650on pad pad0 call procedure:stop
660chdir"/":chdir cwd$()+"/subdir"
670open openfile$ for input as openfileno
672open "file" for output as #3
674 open"file" for output as 3
676 open cwd$() for directory as 12
680close openfileno
690renum 0,renumberstep+42
700run runline:cont
710delete 100-zweihundert
720flash"bang.bin"
730profile on:profile off
740vsync frame()+10
750boot bootpage
760frameskip atn2(3,4):frameskip 4
770resume:vreg $33,10
780network=net connect()
790exec executable$:exec"program.bas":exec "program.bas"
800cmd 4:cmd4=0
810setdate=5:set date y2k+19,10,setdate,0,0,0
820seek openfileno,position
830~liste=[1,2,3,4]:print ~liste(0)
840@locstring$="foo":print@locstring$
850dim a$(30) :a$(20)="zwanzig"
860~stringlst$=["eins","zwei"]:print ~stringlst$(0)
870append ~liste,42
880credits:xyzzy"zork.z3":loadmod"sam"
890?arg$(0)+bin$($67)+hex$(23)+str$(69)+left$("links",3)+right$("rechts",4)
900?mid$("mittwoch",3,4)+cwd$()+dir$(dirhandle)+inkey$+inkeydinky$
910print popf$(~stringlst$);popb$(~stringlst$)
915print popf(~liste);popb$(~liste)
920print inst$(10)+ret$(1);string$(strings,stringstring$);ret(rett)+argc(0)
930print arg(argnum)+arg(1);abs(absolute);len(length$(10))+int(-42.3)
940?gpin(inpin)+gpin(32)+ana()
950?tilecoll(spritenum,bgnum,tilenum);sprcoll(sprite1,sprite2)
960?sprx(sprite1);sprx(2);spry(sprite2);spry(4)
970?bscrx(bg0);bscry(bg1);bscrx(2);bscry(4)
980?val(str$(1)),pos(posaxis),pos(0)
990?vpeek(567),vpeek(vpeekaddr)
1000?eof(eofno),eof(0),lof(openfileno),lof(left),loc(locno),loc(right)
1010print instr(cwd$(),"/")
1020print compare("eins",zwei$),compare(compare1$,compare2$);
1030do:LOOP until fndead(meat)
1040if a>=1or b<=a or c<>d or d><c or 0not 1 or not1<>not0 eor 42 then print "w0t?";using"###";-7
1050gET time h,min,sec_:?tab(10);h
1060if a=1 then
1070elseif a=3 then
1080else if a=4 then
1090endif
