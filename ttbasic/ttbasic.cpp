/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2017-2018 Ulrich Hecht.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *****************************************************************************/

#include "ttconfig.h"
#include <Arduino.h>
#include "lock.h"
#include "audio.h"

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
#elif defined(ESP8266)
#include <eagle_soc.h>
#endif
#ifdef ESP8266
#include "nosdki2s.h"
#endif
};

#ifdef ESP8266
#include <eboot_command.h>

extern "C" {
#include <user_interface.h>
};
#endif

#ifdef ESP32
#include <soc/rtc.h>
#endif

#include "basic.h"

uint8_t serialMode;

void setup() {
#ifdef ESP32
  rtc_clk_cpu_freq_set(RTC_CPU_FREQ_240M);
#endif
  // That does not seem to be necessary on ESP8266.
  //randomSeed(analogRead(PA0));

  // Wait a moment before firing up everything.
  // Not doing this causes undesirable effects when cold booting, such as
  // the keyboard failing to initialize. This does not happen when resetting
  // the powered system. This suggests a power issue, although scoping the
  // supplies did not reveal anything suspicious.
  delay(1000);	// 1s seems to always do the trick; 500ms is not reliable.

  Serial.begin(115200);
  SpiLock();

#ifdef ESP8266_NOWIFI
  //Select_CLKx1();
  //ets_update_cpu_frequency(80);
#endif
}

void loop(void){
  Serial.println(F("\nStarting")); Serial.flush();

#ifdef ESP8266
  SPI.pins(14, 12, 13, 15);
#endif
  SPI.setDataMode(SPI_MODE0);

#ifdef USE_VS23
  // Set up VS23 chip select.
  digitalWrite(15, HIGH);
  pinMode(15, OUTPUT);
#endif

  SPI.begin();
  // Default to safe frequency.
  SPI.setFrequency(11000000);

  SpiUnlock();

  // Initialize I2S to default sample rate, start transmission.
  audio.init(16000);

  bc = new Basic;
  memset(bc, 0, sizeof(*bc));
  bc->basic();	// does not return
}

#ifdef HAVE_PROFILE
extern "C" void ICACHE_RAM_ATTR __cyg_profile_func_enter(void *this_fn, void *call_site)
{
}

extern "C" void ICACHE_RAM_ATTR __cyg_profile_func_exit(void *this_fn, void *call_site)
{
}
#endif
