#ifndef _ESP32GFX_H
#define _ESP32GFX_H

#include "SimplePALOutput.h"
#include "GuillotineBinPack.h"

#define SC_DEFAULT 0
#define XRES 320
#define YRES 240
#define LAST_LINE 480

struct esp32gfx_mode_t {
  uint16_t x;
  uint16_t y;
  uint8_t top;
  uint8_t left;
};

class ESP32GFX {
public:
  void begin(bool interlace = false, bool lowpass = false, uint8_t system = 0);

  inline uint16_t width() {
    return XRES;
  }
  inline uint16_t height() {
    return YRES;
  }

  int numModes() { return 1; }
  void setMode(uint8_t mode);
  void setColorSpace(uint8_t palette) {}
  inline int lastLine() { return LAST_LINE; }

  bool blockFinished() { return true; }

  bool allocBacking(int w, int h, int &x, int &y);
  void freeBacking(int x, int y, int w, int h);

  void reset();

  void setBorder(uint8_t y, uint8_t uv) {}
  void setBorder(uint8_t y, uint8_t uv, uint16_t x, uint16_t w) {}
  inline uint16_t borderWidth() { return 42; }

  inline void setPixel(uint16_t x, uint16_t y, uint8_t c) {
    m_pixels[y][x] = c;
  }
  void setPixelRgb(uint16_t xpos, uint16_t ypos, uint8_t r, uint8_t g, uint8_t b);
  inline uint8_t getPixel(uint16_t x, uint16_t y) {
    return m_pixels[y][x];
  }

  void MoveBlock(uint16_t x_src, uint16_t y_src, uint16_t x_dst, uint16_t y_dst, uint8_t width, uint8_t height, uint8_t dir);

  inline void setInterlace(bool interlace) {
    //m_interlace = interlace;
  }
  inline void setLowpass(bool lowpass) {
    //m_lowpass = lowpass;
  }
  inline void setLineAdjust(int8_t line_adjust) {
    //m_line_adjust = line_adjust;
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
  inline uint32_t pixelAddr(int x, int y) {	// XXX: uint32_t? ouch...
    return (uint32_t)&m_pixels[y][x];
  }
  inline uint32_t piclineByteAddress(int line) {
      return (uint32_t)&m_pixels[line][0];
  }

  inline uint32_t frame() { return m_frame; }

  void render();

private:
  struct esp32gfx_mode_t m_current_mode;
  GuillotineBinPack m_bin;

  uint32_t m_frame;
  uint8_t *m_pixels[LAST_LINE];

  SimplePALOutput m_pal;
};

extern ESP32GFX vs23;

static inline bool blockFinished() { return true; }

#endif
