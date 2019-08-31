#ifndef _DOSGFX_H
#define _DOSGFX_H

#include "ttconfig.h"
#include "bgengine.h"
#include "colorspace.h"

// pc.h defines a function sound() that conflicts with our BasicSound instance.
#define sound __pc_sound
// This is not defined for whatever reason, leading to conflicts with stdint.h.
#define ALLEGRO_HAVE_STDINT_H
#include <allegro.h>
#undef sound

#define SC_DEFAULT 14
#define SC_DEFAULT_SECONDARY 3

#define DOS_SCREEN_MODES 15

extern volatile int retrace_count;

class DOSGFX : public BGEngine {
public:
  void begin(bool interlace = false, bool lowpass = false, uint8_t system = 0);

  inline uint16_t width() {
    return m_current_mode.x;
  }
  inline uint16_t height() {
    return m_current_mode.y;
  }

  int numModes() { return DOS_SCREEN_MODES; }
  bool setMode(uint8_t mode);
  void setColorSpace(uint8_t palette);

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
    _putpixel(screen, x + m_current_mode.left, y + m_current_mode.top, c);
  }
  inline void setPixelIndexed(uint16_t x, uint16_t y, ipixel_t c) {
    _putpixel(screen, x + m_current_mode.left, y + m_current_mode.top, c);
  }
  void setPixelRgb(uint16_t xpos, uint16_t ypos, uint8_t r, uint8_t g, uint8_t b);
  inline pixel_t getPixel(uint16_t x, uint16_t y) {
    return _getpixel(screen, x + m_current_mode.left, y + m_current_mode.top);
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
    uint32_t x = (address >> 16) + m_current_mode.left;
    uint32_t y = (address & 0xffff) + m_current_mode.top;
    for (uint32_t i = 0; i < len; ++i)
    	_putpixel(screen, x+i, y, data[i]);
  }
  inline void setPixelsIndexed(uint32_t address, ipixel_t *data, uint32_t len) {
    setPixels(address, (pixel_t *)data, len);
  }

  inline uint32_t pixelAddr(int x, int y) {	// XXX: uint32_t? ouch...
    return (uint32_t)(x << 16)|(y & 0xffff);
  }
  inline uint32_t piclineByteAddress(int line) {
    return (uint32_t)(line & 0xffff);
  }

  void render();

  inline uint32_t frame() {
    return retrace_count;
  }

private:
  static const struct video_mode_t modes_pal[];
  bool m_display_enabled;
};

extern DOSGFX vs23;

static inline bool blockFinished() { return true; }

#endif
