/*
  TOYOSHIKI Tiny BASIC for Arduino
 (C)2012 Tetsuya Suzuki
  2017/03/22 Modified by Tamakichi、for Arduino STM32
 */

#include "ttconfig.h"
#include "lock.h"

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

extern "C" void setSample(uint8_t s);
void SpiRamVideoInit();


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
  Serial.println("Wir sind da, wo unten ist.");
  eboot_command ebcmd;
  ebcmd.action = ACTION_LOAD_APP;
  ebcmd.args[0] = 0x80000;
  //eboot_command_write(&ebcmd);
  Serial.println("\nStarting");Serial.flush();
  for (int i=0; i < 3; ++i) {
    Serial.println("tick");Serial.flush();
    Serial.println(millis());
  }

  SPI.pins(14, 12, 13, 15);
  SPI.setDataMode(SPI_MODE0);
  digitalWrite(15, HIGH);
  pinMode(15, OUTPUT);
  SPI.begin();
  SPI.setFrequency(11000000);

  for (int i=0; i < 10; ++i) {
    digitalWrite(15, LOW);
    SPI.transfer(0x9f);
    Serial.print(SPI.transfer(0), HEX);
    Serial.println(SPI.transfer(0), HEX);
    digitalWrite(15, HIGH);
  }
  SpiUnlock();

  Serial.println("I2S");Serial.flush();
  InitI2S();
  SendI2S();

  // put your main code here, to run repeatedly:
  basic();
}
