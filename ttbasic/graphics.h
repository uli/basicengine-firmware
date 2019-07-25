#pragma once

class Graphics {
public:
  static void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, pixel_t c);
  static void drawLineRgb(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t r, uint8_t g, uint8_t b);
  static void drawRect(int x0, int y0, int w, int h, pixel_t c, int fc);
  static void drawCircle(int x0, int y0, int radius, pixel_t c, int fc);
};

extern Graphics gfx;
