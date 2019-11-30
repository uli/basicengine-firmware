// SPDX-License-Identifier: MIT
// Copyright (c) 2018, 2019 Ulrich Hecht

#ifndef _ESP32GFX_H
#define _ESP32GFX_H

#include "ttconfig.h"
#include "bgengine.h"

#define SC_DEFAULT 11

#include "SimplePALOutput.h"

class ESP32GFX : public BGEngine {
public:
  void begin(bool interlace = false, bool lowpass = false, uint8_t system = 0);

  inline uint16_t width() {
    return m_current_mode.x;
  }
  inline uint16_t height() {
    return m_current_mode.y;
  }

  int numModes() { return SPO_NUM_MODES; }
  bool setMode(uint8_t mode);
  inline void setColorSpace(uint8_t palette) {
    m_pal.setColorSpace(palette);
    Video::setColorSpace(palette);
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

  void setBorder(uint8_t y, uint8_t uv) {}
  void setBorder(uint8_t y, uint8_t uv, uint16_t x, uint16_t w) {}
  inline uint16_t borderWidth() { return 42; }

  inline void setPixel(uint16_t x, uint16_t y, pixel_t c) {
    m_pixels[y][x] = c;
  }
  inline void setPixelIndexed(uint16_t x, uint16_t y, ipixel_t c) {
    setPixel(x, y, (pixel_t)c);
  }
  void setPixelRgb(uint16_t xpos, uint16_t ypos,
                   uint8_t r, uint8_t g, uint8_t b);
  inline pixel_t getPixel(uint16_t x, uint16_t y) {
    return m_pixels[y][x];
  }

  void blitRect(uint16_t x_src, uint16_t y_src, uint16_t x_dst, uint16_t y_dst,
                uint16_t width, uint16_t height);
  void MoveBlock(uint16_t x_src, uint16_t y_src, uint16_t x_dst, uint16_t y_dst,
                 uint16_t width, uint16_t height, uint8_t dir);

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

  inline void writeBytes(uint32_t address, uint8_t *data, uint32_t len) {
    memcpy((void *)address, data, len);
  }
  inline void setPixels(uint32_t address, pixel_t *data, uint32_t len) {
    writeBytes(address, (uint8_t *)data, len * sizeof(pixel_t));
  }
  inline void setPixelsIndexed(uint32_t address, ipixel_t *data, uint32_t len) {
    writeBytes(address, (uint8_t *)data, len);
  }

  inline uint32_t pixelAddr(int x, int y) {  // XXX: uint32_t? ouch...
    return (uint32_t)&m_pixels[y][x];
  }
  inline uint32_t piclineByteAddress(int line) {
    return (uint32_t)&m_pixels[line][0];
  }

  void render();

private:
  static const struct video_mode_t modes_pal[];
  bool m_display_enabled;

  pixel_t **m_pixels;

  SimplePALOutput m_pal;
};

extern ESP32GFX vs23;

static inline bool blockFinished() { return true; }

#endif
