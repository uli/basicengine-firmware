#ifdef ESP32

#include <Arduino.h>
#include <soc/rtc.h>
#include "esp32gfx.h"
#include "colorspace.h"

ESP32GFX vs23;

static void pal_core(void *data)
{
  for (;;)
    vs23.render();
}

void ESP32GFX::render()
{
  m_pal.sendFrame(m_pixels);
  m_frame++;
}

void ESP32GFX::begin(bool interlace, bool lowpass, uint8_t system)
{
  m_current_mode.x = 320;
  m_current_mode.y = 240;
  m_current_mode.top = 0;	// XXX: use real value
  m_current_mode.left = 0;	// ditto

  m_bin.Init(0, 0);

  m_frame = 0;
  m_pal.init();
  for (int i = 0; i < LAST_LINE; ++i) {
    m_pixels[i] = (uint8_t *)malloc(XRES);
    memset(m_pixels[i], i, XRES);
  }
  reset();
  xTaskCreatePinnedToCore(pal_core, "c", 1024, NULL, 1, NULL, 0);
}

void ESP32GFX::reset()
{
  for (int i = 0; i < LAST_LINE; ++i)
    memset(m_pixels[i], 0, XRES);
  m_bin.Init(m_current_mode.x, LAST_LINE - m_current_mode.y);
  setColorSpace(0);
}

void ESP32GFX::MoveBlock(uint16_t x_src, uint16_t y_src, uint16_t x_dst, uint16_t y_dst, uint8_t width, uint8_t height, uint8_t dir)
{
}

// XXX: duplicate code in VS23S010
bool ESP32GFX::allocBacking(int w, int h, int &x, int &y)
{
  Rect r = m_bin.Insert(w, h, false, GuillotineBinPack::RectBestAreaFit, GuillotineBinPack::Split256);
  x = r.x; y = r.y + m_current_mode.y;
  return r.height != 0;
}

void ESP32GFX::freeBacking(int x, int y, int w, int h)
{
  Rect r;
  r.x = x; r.y = y - m_current_mode.y; r.width = w; r.height = h;
  m_bin.Free(r, true);
}

void ESP32GFX::setMode(uint8_t mode)
{
}

#endif
