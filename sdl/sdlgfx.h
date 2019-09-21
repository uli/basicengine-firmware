#ifndef _SDLGFX_H
#define _SDLGFX_H

#include "ttconfig.h"
#include "bgengine.h"
#include "colorspace.h"

#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>

extern "C" {
#include <libswscale/swscale.h>
}

#define SC_DEFAULT 14
#define SC_DEFAULT_SECONDARY 3

#define SDL_SCREEN_MODES 15

#define PIXEL(x, y) ( \
  ((pixel_t*)m_surface->pixels)[ \
    x + m_current_mode.left + (y + m_current_mode.top) * m_surface->pitch / sizeof(pixel_t) \
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
    PIXEL(x, y) = c;
  }
  inline void setPixelIndexed(uint16_t x, uint16_t y, ipixel_t c) {
    PIXEL(x, y) = m_current_palette[c];
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
    uint32_t x = (address >> 16) + m_current_mode.left;
    uint32_t y = (address & 0xffff) + m_current_mode.top;
    for (uint32_t i = 0; i < len; ++i)
    	setPixel(x + i, y, data[i]);
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
    return SDL_GetTicks()/17;//m_frame;
  }

private:
  static const struct video_mode_t modes_pal[];
  bool m_display_enabled;
  SDL_Surface *m_screen;
  SDL_Surface *m_surface;
  
  SwsContext *m_resize;
  uint8_t *m_src_pix[1];
  int m_src_stride[1];
  uint8_t *m_dst_pix[1];
  int m_dst_stride[1];
  
  pixel_t m_current_palette[256];
};

#undef PIXEL

extern SDLGFX vs23;

static inline bool blockFinished() { return true; }

#endif
