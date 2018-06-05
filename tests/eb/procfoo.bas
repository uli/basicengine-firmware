0 GOTO 100
10 PROC foo
20 PRINT "foo"
30 RETURN
40 PROC bar(a,b)
41 @c=@a+@b
50 PRINT "bar";@a;@b
54 PRINT "bar";@c
60 RETURN
70 PROC sbar(a$,b$)
80 PRINT "sbar";@a$;@b$
81 @a$=@a$+"echt"
82 PRINT "sbar";@a$;@b$
83 @c$="zee"
84 PRINT "sbar";@a$;@b$;@c$
90 RETURN
100 PRINT "a"
110 CALL foo()
115 c=23
120 CALL bar(1,2)
123 PRINT "c";c
125 a$="rotz"
126 c$="global"
130 CALL sbar("eins","zwei")
135 PRINT a$
136 PRINT c$
