// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#ifndef _H3GFX_H
#define _H3GFX_H

#include "ttconfig.h"
#include "bgengine.h"
#include "colorspace.h"

#include <string.h>
#include <display.h>
#include <spinlock.h>

#define SC_DEFAULT 13

#define H3_SCREEN_MODES 20

class H3GFX : public BGEngine {
public:
  void begin(bool interlace = false, bool lowpass = false, uint8_t system = 0);

  inline uint16_t width() {
    return m_current_mode.x;
  }
  inline uint16_t height() {
    return m_current_mode.y;
  }

  int numModes() { return H3_SCREEN_MODES; }
  bool setMode(uint8_t mode);
  inline void setColorSpace(uint8_t palette) {
    Video::setColorSpace(palette);
    if (palette < 2) {
      uint8_t *pal = csp.paletteData(palette);
      for (int i = 0; i < 256; ++i) {
        m_current_palette[i] = (pixel_t)((pal[i*3] << 16) | (pal[i*3+1] << 8) | pal[i*3+2]);
      }
    }
  }

  bool blockFinished() { return true; }

#ifdef USE_BG_ENGINE
  void updateBg();
  void updateBgTask();

  inline void setSpriteOpaque(uint8_t num, bool enable) {
    m_sprite[num].p.opaque = enable;
  }

  uint8_t spriteCollision(uint8_t collidee, uint8_t collider);
#endif

  void reset();

  void setBorder(uint8_t y, uint8_t uv) {}
  void setBorder(uint8_t y, uint8_t uv, uint16_t x, uint16_t w) {}
  inline uint16_t borderWidth() { return 42; }

  inline void setPixel(uint16_t x, uint16_t y, pixel_t c) {
    m_pixels[y][x] = c;
    m_dupe_active = true;
  }
  inline void setPixelIndexed(uint16_t x, uint16_t y, ipixel_t c) {
    if (csp.getColorSpace() == 2) {
      setPixel(x, y, c);
      return;
    }
    m_pixels[y][x] = m_current_palette[c];
    m_dupe_active = true;
  }
  void setPixelRgb(uint16_t xpos, uint16_t ypos, uint8_t r, uint8_t g,
                   uint8_t b);
  inline pixel_t getPixel(uint16_t x, uint16_t y) {
    return m_pixels[y][x];
  }

  void blitRect(uint16_t x_src, uint16_t y_src, uint16_t x_dst, uint16_t y_dst,
                uint16_t width, uint16_t height);

  inline void setInterlace(bool interlace) {
  }

  inline void setLowpass(bool lowpass) {
    m_force_filter = lowpass;
    display_enable_filter(m_force_filter);
  }

  void setSpiClock(uint32_t div) {
  }
  void setSpiClockRead() {
  }
  void setSpiClockWrite() {
  }
  void setSpiClockMax() {
  }

  inline void setPixels(uint32_t address, pixel_t *data, uint32_t len) {
    memcpy((pixel_t *)address, data, len * sizeof(pixel_t));
    m_dupe_active = true;
  }
  inline void setPixelsIndexed(uint32_t address, ipixel_t *data, uint32_t len) {
    if (csp.getColorSpace() == 2) {
      setPixels(address, data, len);
      return;
    }
    pixel_t *pa = (pixel_t *)address;
    for (uint32_t i = 0; i < len; ++i)
      *pa++ = m_current_palette[*data++];
    m_dupe_active = true;
  }

  inline uint32_t pixelAddr(int x, int y) {	// XXX: uint32_t? ouch...
    return (uint32_t)&m_pixels[y][x];
  }
  inline uint32_t piclineByteAddress(int line) {
    return (uint32_t)&m_pixels[line][0];
  }

  void render();

protected:
  void updateStatus() override;

private:
  inline void blitBuffer(void *buf);
  void resetLinePointers();

  static const struct video_mode_t modes_pal[];
  bool m_display_enabled;
  bool m_force_filter;
  bool m_engine_enabled;
  bool m_dupe_active;
  spinlock_t m_buffer_lock;
  bool m_frame_ready;

  pixel_t **m_pixels;

  pixel_t m_current_palette[256];
};

extern H3GFX vs23;

static inline bool blockFinished() { return true; }

#endif
