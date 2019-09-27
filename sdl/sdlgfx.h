// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#ifndef _SDLGFX_H
#define _SDLGFX_H

#include "ttconfig.h"
#include "bgengine.h"
#include "colorspace.h"

#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>

#define SC_DEFAULT 14
#define SC_DEFAULT_SECONDARY 3

#define SDL_SCREEN_MODES 20

#define PIXEL(x, y) ( \
  ((pixel_t*)m_surface->pixels)[ \
    x + y * m_surface->pitch / sizeof(pixel_t) \
  ] )

class SDLGFX : public BGEngine {
public:
  void begin(bool interlace = false, bool lowpass = false, uint8_t system = 0);

  inline uint16_t width() {
    return m_current_mode.x;
  }
  inline uint16_t height() {
    return m_current_mode.y;
  }

  int numModes() { return SDL_SCREEN_MODES; }
  bool setMode(uint8_t mode);
  void setColorSpace(uint8_t palette);

  inline pixel_t colorFromRgb(int r, int g, int b) {
    return SDL_MapRGB(m_surface->format, r, g, b);
  }

  bool blockFinished() { return true; }

#ifdef USE_BG_ENGINE
  void updateBg();

  inline void setSpriteOpaque(uint8_t num, bool enable) {
    m_sprite[num].p.opaque = enable;
  }

  uint8_t spriteCollision(uint8_t collidee, uint8_t collider);
#endif

  void reset();

  void setBorder(uint8_t y, uint8_t uv, uint16_t x, uint16_t w);
  inline void setBorder(uint8_t y, uint8_t uv) {
    setBorder(y, uv, 0, borderWidth());
  }

  inline uint16_t borderWidth() { return m_screen->w; }

  inline void setPixel(uint16_t x, uint16_t y, pixel_t c) {
    PIXEL(x, y) = c;
    m_dirty = true;
  }
  inline void setPixelIndexed(uint16_t x, uint16_t y, ipixel_t c) {
    PIXEL(x, y) = m_current_palette[c];
    m_dirty = true;
  }
  void setPixelRgb(uint16_t xpos, uint16_t ypos, uint8_t r, uint8_t g, uint8_t b);
  inline pixel_t getPixel(uint16_t x, uint16_t y) {
    return PIXEL(x, y);
  }

  void MoveBlock(uint16_t x_src, uint16_t y_src, uint16_t x_dst, uint16_t y_dst, uint16_t width, uint16_t height, uint8_t dir);

  inline void setInterlace(bool interlace) {
    //m_interlace = interlace;
  }
  inline void setLowpass(bool lowpass) {
    //m_lowpass = lowpass;
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
    uint32_t x = address >> 16;
    uint32_t y = address & 0xffff;

    for (uint32_t i = 0; i < len; ++i)
      PIXEL(x+i, y) = data[i];

    m_dirty = true;
  }

  inline void setPixelsIndexed(uint32_t address, ipixel_t *data, uint32_t len) {
    uint32_t x = address >> 16;
    uint32_t y = address & 0xffff;

    for (uint32_t i = 0; i < len; ++i)
      PIXEL(x+i, y) = m_current_palette[data[i]];

    m_dirty = true;
  }

  inline uint32_t pixelAddr(int x, int y) {	// XXX: uint32_t? ouch...
    return (uint32_t)(x << 16)|(y & 0xffff);
  }
  inline uint32_t piclineByteAddress(int line) {
    return (uint32_t)(line & 0xffff);
  }

  void render();

  inline uint32_t frame() {
    return m_frame;
  }

private:
  inline pixel_t *screenPixel(int x, int y) {
    return &((pixel_t *)m_screen->pixels)[y * m_screen->pitch/sizeof(pixel_t) + x];
  }

  static Uint32 timerCallback(Uint32 t);

  static const struct video_mode_t modes_pal[];
  bool m_display_enabled;
  SDL_Surface *m_screen;
  SDL_Surface *m_surface;
  bool m_dirty;
  
  pixel_t m_current_palette[256];
};

#undef PIXEL

extern SDLGFX vs23;

static inline bool blockFinished() { return true; }

#endif
