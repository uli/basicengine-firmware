
/* Azip Arduino Zcode Interpreter Program
 * --------------------------------------------------------------------
 * Derived from John D. Holder's Jzip V2.1 source code
 * http://jzip.sourceforge.net/
 * --------------------------------------------------------------------
 */
 
 /*
  * Created by Louis Davis April, 2012
  *
  */

#include <basic.h>
#include <SdFat.h>
//#include <SdFatUtil.h>
#include "ztypes.h"

void azipLoad()
{
  open_story( );
  
  // put your setup code here, to run once:
  configure( V1, V8 );

  initialize_screen(  );

  z_restart(  );
}

void azipLoop()
{
  // put your main code here, to run repeatedly:
  interpret( );  
}

