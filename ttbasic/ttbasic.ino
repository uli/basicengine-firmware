/*
  TOYOSHIKI Tiny BASIC for Arduino
 (C)2012 Tetsuya Suzuki
  2017/03/22 Modified by Tamakichi、for Arduino STM32
 */

#include "ttconfig.h"
#include "lock.h"

#ifdef ENABLE_GDBSTUB
#include <GDBStub.h>
#endif

#ifdef HAVE_PROFILE
void NOINS setup(void);
void NOINS loop(void);
#endif

volatile int spi_lock;

#include <SPI.h>
extern "C" {
#ifdef ESP8266_NOWIFI
#include <hw/esp8266.h>
#include <hw/pin_mux_register.h>
#else
#include <eagle_soc.h>
#endif
#include "nosdki2s.h"
};

#include <eboot_command.h>

extern "C" {
#include <user_interface.h>
};

void basic();
uint8_t serialMode;

void setup(void){
  // That does not seem to be necessary on ESP8266.
  //randomSeed(analogRead(PA0));

  // Wait a moment before firing up everything.
  // Not doing this causes undesirable effects when cold booting, such as
  // the keyboard failing to initialize. This does not happen when resetting
  // the powered system. This suggests a power issue, although scoping the
  // supplies did not reveal anything suspicious.
  delay(1000);	// 1s seems to always do the trick; 500ms is not reliable.

  Serial.begin(921600);
  SpiLock();

#ifdef ESP8266_NOWIFI
  //Select_CLKx1();
  //ets_update_cpu_frequency(80);
#endif
}

void loop(void){
  Serial.println(F("\nStarting")); Serial.flush();

  SPI.pins(14, 12, 13, 15);
  SPI.setDataMode(SPI_MODE0);
  // Set up VS23 chip select.
  digitalWrite(15, HIGH);
  pinMode(15, OUTPUT);
  SPI.begin();
  // Default to safe frequency.
  SPI.setFrequency(11000000);

  SpiUnlock();

  // Initialize I2S to default sample rate, start transmission.
  InitI2S(16000);
  SendI2S();

  basic();	// does not return
}

#ifdef HAVE_PROFILE
extern "C" void ICACHE_RAM_ATTR __cyg_profile_func_enter(void *this_fn, void *call_site)
{
}

extern "C" void ICACHE_RAM_ATTR __cyg_profile_func_exit(void *this_fn, void *call_site)
{
}
#endif
