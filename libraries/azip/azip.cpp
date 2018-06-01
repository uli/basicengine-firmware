
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


#include <SdFat.h>
#include <SdFatUtil.h>
#include "ztypes.h"

// SD chip select pin
const uint8_t chipSelect = 4;

// file system
SdFat sd;

void setup()
{
  Serial.begin(9600);
  
  Serial.println("Enter any key to start...");
  
  while (!Serial.available()) {}
  Serial.read();
 
  if (!sd.init(SPI_FULL_SPEED, chipSelect)) sd.initErrorHalt();

  open_story( );
  
  // put your setup code here, to run once:
  configure( V1, V8 );

  initialize_screen(  );

  z_restart(  );
}

void loop()
{
  // put your main code here, to run repeatedly:
  interpret( );  
}

