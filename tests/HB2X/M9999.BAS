  100 rem Purpose: Determine maximum recursion
  110 rem  Author: Howard Wulf, AF5NE
  120 rem    Date: 2015-10-03
  130 rem
  131 GOTO 300
  200 PROC a( X )
  210   rem print "X=";@X
  220   RETURN FN a( @X + 1 )
  300 let x = fna( 1 )
  999 end

