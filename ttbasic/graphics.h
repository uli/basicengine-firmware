#pragma once

class Graphics {
public:
  static void drawLine(int x1, int y1, int x2, int y2, pixel_t c);
  static void drawLineRgb(int x1, int y1, int x2, int y2, uint8_t r, uint8_t g, uint8_t b);
  static void drawRect(int x0, int y0, int w, int h, pixel_t c, int fc);
  static void drawCircle(int x0, int y0, int radius, pixel_t c, int fc);
};

extern Graphics gfx;
