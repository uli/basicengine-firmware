// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#ifndef _H3GFX_H
#define _H3GFX_H

#include "ttconfig.h"
#include "bgengine.h"
#include "colorspace.h"

#include <string.h>
#include <display.h>
#include <mmu.h>
#include <tve.h>
#include <spinlock.h>

#define SC_DEFAULT 13

#define H3_SCREEN_MODES 20

#define LOCK_SPRITES ;	// XXX
#define UNLOCK_SPRITES ;

class H3GFX : public BGEngine {
public:
  void begin(bool interlace = false, bool lowpass = false, uint8_t system = 0);

  inline pixel_t& pixelText(uint16_t x, uint16_t y) {
    return m_pixels[y][x];
  }
  inline pixel_t& pixelComp(uint16_t x, uint16_t y) {
    return m_bgpixels[y][x];
  }

  inline int textPitch() {
    return m_current_mode.x + m_current_mode.left * 2;
  }
  inline int offscreenPitch() {
    return m_current_mode.x;
  }
  inline int compositePitch() {
    return m_current_mode.x + m_current_mode.left * 2;
  }

  void cleanCache() {
    mmu_flush_dcache();
  }

  inline uint16_t width() {
    return m_current_mode.x;
  }
  inline uint16_t height() {
    return m_current_mode.y;
  }

  int numModes() { return H3_SCREEN_MODES; }
  bool setMode(uint8_t mode);
  int modeFromSize(int &w, int &h);
  void modeSize(int m, int &w, int &h);

  inline void setColorSpace(uint8_t palette) {
    Video::setColorSpace(palette);
    if (palette < 2) {
      uint8_t *pal = csp.paletteData(palette);
      for (int i = 0; i < 256; ++i) {
        m_current_palette[i] = (pixel_t)((0xff << 24) | (pal[i * 3] << 16) |
                                         (pal[i * 3 + 1] << 8) | pal[i * 3 + 2]);
      }
    }
  }

  inline pixel_t colorFromRgb(uint8_t r, uint8_t g, uint8_t b) {
    return (pixel_t)(0xff << 24 | r << 16 | g << 8 | b);
  }

  inline pixel_t colorFromRgba(uint8_t r, uint8_t g, uint8_t b, uint32_t a) {
    return (pixel_t)(a << 24 | r << 16 | g << 8 | b);
  }

  inline void rgbaFromColor(pixel_t p, uint8_t &r, uint8_t &g, uint8_t &b,
                            uint8_t &a) {
    r = (p >> 16) & 0xff;
    g = (p >> 8) & 0xff;
    b = p & 0xff;
    a = p >> 24;
  }

  inline uint8_t alphaFromColor(pixel_t p) {
    return p >> 24;
  }

  bool blockFinished() {
    return true;
  }

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
  inline uint16_t borderWidth() {
    return m_current_mode.x + m_current_mode.left * 2;
  }

  inline void setPixel(uint16_t x, uint16_t y, pixel_t c) {
    m_pixels[y][x] = c;
    m_textmode_buffer_modified = true;
  }
  inline void setPixelIndexed(uint16_t x, uint16_t y, ipixel_t c) {
    if (csp.getColorSpace() == 2) {
      setPixel(x, y, c);
      return;
    }
    m_pixels[y][x] = m_current_palette[c];
    m_textmode_buffer_modified = true;
  }
  void setPixelRgb(uint16_t xpos, uint16_t ypos, uint8_t r, uint8_t g, uint8_t b);
  inline pixel_t getPixel(uint16_t x, uint16_t y) {
    return m_pixels[y][x];
  }

  void blitRect(uint16_t x_src, uint16_t y_src, uint16_t x_dst, uint16_t y_dst,
                uint16_t width, uint16_t height);
  void blitRectAlpha(uint16_t x_src, uint16_t y_src, uint16_t x_dst, uint16_t y_dst,
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

  inline void setPixels(pixel_t *address, pixel_t *data, uint32_t len) {
    memcpy(address, data, len * sizeof(pixel_t));
    m_textmode_buffer_modified = true;
  }
  inline void setPixelsIndexed(pixel_t *address, ipixel_t *data, uint32_t len) {
    if (csp.getColorSpace() == 2) {
      setPixels(address, data, len);
      return;
    }
    for (uint32_t i = 0; i < len; ++i)
      *address++ = m_current_palette[*data++];
    m_textmode_buffer_modified = true;
  }

  inline pixel_t *pixelAddr(int x, int y) {
    return &m_pixels[y][x];
  }
  inline pixel_t *piclineByteAddress(int line) {
    return &m_pixels[line][0];
  }

  void render();

protected:
  void updateStatus() override;

private:
  void drawBg(bg_t *bg);
  void drawSprite(sprite_t *s);

  inline void blitBuffer(pixel_t *dst, pixel_t *buf);
  void resetLinePointers(pixel_t **pixels, pixel_t *buffer);

  static const struct video_mode_t modes_pal[];
  bool m_display_enabled;
  bool m_force_filter;
  bool m_engine_enabled;
  bool m_textmode_buffer_modified;
  spinlock_t m_buffer_lock;

  // Used by text mode, pixel graphics functions. Points to
  // m_textmode_buffer's pixels when BG engine is on, and display device
  // frame buffer when BG engine is off
  pixel_t **m_pixels;
  // Used by BG engine; points to currently hidden device framebuffer.
  pixel_t **m_bgpixels;

  pixel_t *m_textmode_buffer;  // text-mode pixel memory used when BG engine is on
  pixel_t *m_offscreenbuffer;  // off-screen pixel memory

  pixel_t m_current_palette[256];

  friend void ::hook_display_vblank(void);
};

extern H3GFX vs23;

static inline bool blockFinished() { return true; }

#endif
