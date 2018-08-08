#ifndef _VIDEO_DRIVER_H
#define _VIDEO_DRIVER_H

#include "ttconfig.h"
#include "GuillotineBinPack.h"
#include <stdint.h>

struct video_mode_t {
  uint16_t x;
  uint16_t y;
  uint16_t top;
  uint16_t left;
  uint8_t vclkpp;
#ifdef USE_VS23
  uint8_t bextra;
  // Maximum SPI frequency for this mode; translated to minimum clock
  // divider on boot.
  uint32_t max_spi_freq;
#endif
};

class Video {
public:
  virtual void reset();

  inline void setLineAdjust(int8_t line_adjust) {
    m_line_adjust = line_adjust;
  }

  inline int lastLine() { return m_last_line; }

  bool allocBacking(int w, int h, int &x, int &y);
  void freeBacking(int x, int y, int w, int h);

#ifdef HOSTED
    uint32_t frame();
#else
    inline uint32_t frame() {
      return m_frame;
    }
#endif

  void setColorSpace(uint8_t palette);

#ifndef HOSTED
protected:
#endif
  uint32_t m_frame;

  struct video_mode_t m_current_mode;
  int m_last_line;
  int8_t m_line_adjust;

  GuillotineBinPack m_bin;
};

#endif	// _VIDEO_DRIVER_H
