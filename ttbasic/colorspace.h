// SPDX-License-Identifier: MIT
// Copyright (c) 2018, 2019 Ulrich Hecht

#ifndef _COLORSPACE_H
#define _COLORSPACE_H

#include "video_driver.h"
#include <stdint.h>

#define CSP_NUM_COLORSPACES	2

class Colorspace {
public:
    pixel_t colorFromRgb(uint8_t r, uint8_t g, uint8_t b);
    inline pixel_t colorFromRgb(uint8_t *c) {
      return colorFromRgb(c[0], c[1], c[2]);
    }

    static void setColorConversion(int yuvpal, int h_weight, int s_weight, int v_weight, bool fixup);
    ipixel_t indexedColorFromRgb(uint8_t r, uint8_t g, uint8_t b);
    inline ipixel_t indexedColorFromRgb(uint8_t *c) {
      return indexedColorFromRgb(c[0], c[1], c[2]);
    }

    inline void setColorSpace(uint8_t palette) {
      m_colorspace = palette;
    }
    inline uint8_t getColorSpace() {
      return m_colorspace;
    }
    uint8_t *paletteData(uint8_t colorspace);

    pixel_t fromIndexed(ipixel_t c);

private:
    ipixel_t indexedColorFromRgbSlow(uint8_t r, uint8_t g, uint8_t b);

    uint8_t m_colorspace;
};

extern Colorspace csp;

#endif
