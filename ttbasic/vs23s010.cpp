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

void VS23S010::begin()
{
  m_vsync_enabled = false;
}

void VS23S010::end()
{
}

void VS23S010::setMode(uint8_t mode)
{
  setSyncLine(0);
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
  vs23.m_newFrame = true;

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
  if (line == 0) {
    if (m_vsync_enabled)
      timer0_detachInterrupt();
    m_vsync_enabled = false;
  } else {
    syncLine = line;
    timer0_isr_init();
    timer0_write(ESP.getCycleCount()+100);
    m_vsync_enabled = true;
    timer0_attachInterrupt(&vsyncHandler);
  }
}

bool VS23S010::setBg(uint8_t bg_idx, uint16_t width, uint16_t height,
                     uint8_t tile_size_x, uint8_t tile_size_y,
                     uint16_t pat_x, uint16_t pat_y, uint16_t pat_w)
{
  struct bg_t *bg = &m_bg[bg_idx];

  bg->enabled = false;

  if (bg->tiles)
    free(bg->tiles);
  bg->tiles = (uint8_t *)calloc(width * height, 1);
  if (!bg->tiles)
    return true;

  bg->pat_x = pat_x;
  bg->pat_y = pat_y;
  bg->pat_w = pat_w;
  bg->tile_size_x = tile_size_x;
  bg->tile_size_y = tile_size_y;
  bg->w = width;
  bg->h = height;
  bg->scroll_x = bg->scroll_y = 0;
  bg->win_x = bg->win_y = 0;
  bg->win_w = currentMode->x;
  bg->win_h = currentMode->y;
  return false;
}

void VS23S010::enableBg(uint8_t bg)
{
  if (m_bg[bg].tiles)
    m_bg[bg].enabled = true;
}

void VS23S010::disableBg(uint8_t bg)
{
  m_bg[bg].enabled = false;
}

#include <SPI.h>
void VS23S010::updateBg()
{
  if (!m_newFrame || SpiLocked())
    return;
  m_newFrame = false;

  SpiLock();
  for (int i = 0; i < VS23_MAX_BG; ++i) {
    struct bg_t *bg = &m_bg[i];
    if (!bg->enabled)
      continue;

    int tile_start_y = bg->scroll_y / bg->tile_size_y;
    int tile_end_y = tile_start_y + bg->win_h / bg->tile_size_y;
    int tile_start_x = bg->scroll_x / bg->tile_size_x;
    int tile_end_x = tile_start_x + bg->win_w / bg->tile_size_x;

    uint8_t *tt;
    uint32_t tile;
    uint32_t tx, ty;
    uint32_t byteaddress2;
    uint32_t dest_addr, pat_start_addr, win_start_addr;
    uint32_t tsx = bg->tile_size_x;
    uint32_t tsy = bg->tile_size_y;
    uint32_t pw = bg->pat_w;
    SpiRamWriteBM2Ctrl(0x35, PICLINE_LENGTH_BYTES+BEXTRA+1-tsx-1, tsx, tsy-1);
    uint16_t pitch = PICLINE_BYTE_ADDRESS(1) - PICLINE_BYTE_ADDRESS(0);
    pat_start_addr = PICLINE_BYTE_ADDRESS(bg->pat_y)+bg->pat_x;
    win_start_addr = PICLINE_BYTE_ADDRESS(bg->win_y) + bg->win_x;
    for (int yy = tile_start_y; yy < tile_end_y; ++yy) {
      tt = &bg->tiles[yy*bg->w];
      dest_addr = win_start_addr + (yy-tile_start_y) * tsy * pitch;
      for (int xx = tile_start_x; xx < tile_end_x; ++xx, dest_addr += tsx) {
        tile = tt[xx];
        tx = (tile % pw) * tsx;
        ty = (tile / pw) * tsy;
        byteaddress2 = pat_start_addr + ty*pitch + tx;

        //SpiRamWriteBMCtrlFast(0x34, byteaddress2 >> 1, dest_addr >> 1);
        uint8_t req[5] = { 0x34, byteaddress2 >> 9, byteaddress2 >> 1, dest_addr >> 9, dest_addr >> 1 };
        VS23_SELECT;
        SPI.writeBytes(req, 5);
        VS23_DESELECT;

        //SpiRamWriteBM3Ctrl(0x36);
        VS23_SELECT;
        SPI.write(0x36);
        VS23_DESELECT;
      }
    }
  }
  SpiUnlock();
}

VS23S010 vs23;
#endif
