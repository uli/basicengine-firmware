#include "ttconfig.h"

#if USE_VS23 == 1

#include "vs23s010.h"
#include "ntsc.h"
#include "lock.h"

void VS23S010::setPixel(uint16_t x, uint16_t y, uint8_t c)
{
  uint32_t byteaddress = PICLINE_BYTE_ADDRESS(y) + x;
  SpiRamWriteByte(byteaddress, c);
}

void VS23S010::adjust(int16_t cnt)
{
  // XXX: Huh?
}

void VS23S010::begin(uint8_t mode, uint8_t spino, uint8_t *extram)
{
  setMode(mode);
}

void VS23S010::end()
{
}

void VS23S010::setMode(uint8_t mode)
{
  timer0_detachInterrupt();
  currentMode = &modes[mode];
  SpiRamVideoInit();
  calibrateVsync();
  setSyncLine(currentMode->y);
}

void VS23S010::calibrateVsync()
{
  uint32_t now;
  while (currentLine() != 100) {};
  now = ESP.getCycleCount();
  while (currentLine() == 100) {};
  while (currentLine() != 100) {};
  cyclesPerFrame = ESP.getCycleCount() - now;
}

void ICACHE_RAM_ATTR VS23S010::vsyncHandler(void)
{
  uint32_t now = ESP.getCycleCount();
  uint32_t next = now + vs23.cyclesPerFrame;
  uint16_t line;
  if (!SpiLocked()) {
    line = vs23.currentLine();
    if (line < vs23.syncLine) {
      next += (vs23.cyclesPerFrame / 262) * (vs23.syncLine-line);
      vs23.cyclesPerFrame+=10;
    } else if (line > vs23.syncLine) {
      next -= (vs23.cyclesPerFrame / 262) * (line-vs23.syncLine);
      vs23.cyclesPerFrame-=10;
    }
#ifdef DEBUG
    if (vs23.syncLine != line) {
      Serial.print("deviation ");
      Serial.print(line-vs23.syncLine);
      Serial.print(" at ");
      Serial.println(millis());
    }
#endif
  }
#ifdef DEBUG
  else
    Serial.println("spilocked");
#endif
  // See you next frame:
  timer0_write(next);
#ifndef ESP8266_NOWIFI
  // Any attempt to disable the software watchdog is subverted by the SDK
  // by re-enabling it as soon as it gets the chance. This is the only
  // way to avoid watchdog resets if you actually want to do anything
  // with your system that is not wireless bullshit.
  system_soft_wdt_feed();
#endif
}

void VS23S010::setSyncLine(uint16_t line)
{
  syncLine = line;
  timer0_isr_init();
  timer0_write(ESP.getCycleCount()+100);
  timer0_attachInterrupt(&vsyncHandler);
}

VS23S010 vs23;
#endif
