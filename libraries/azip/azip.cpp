
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

#include "../../ttbasic/basic.h"
#include "azip.h"
#include "ztypes.h"

void AZIP::load(const char *file_name)
{
  cache_blocks = (struct cache_block *)calloc(CACHE_BLOCKS, sizeof(*cache_blocks));
  for (int i = 0; i < CACHE_BLOCKS; ++i) 
    cache_blocks[i].addr = -1;

  open_story( file_name );
  
  // put your setup code here, to run once:
  configure( V1, V8 );

  initialize_screen(  );

  z_restart(  );
}

void AZIP::run()
{
  // put your main code here, to run repeatedly:
  interpret( );  
  free(cache_blocks);
  sc0.setWindow(0, 0, sc0.getScreenWidth(), sc0.getScreenHeight());
}

