// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "basic.h"
#include "graphics.h"

#include "c_video.h"
#include "c_api.h"

extern "C" {

// **** スクリーン管理 *************
static uint8_t scmode = 0;

void c_locate(int32_t x, int32_t y) {
  if (x >= sc0.getWidth())  // xの有効範囲チェック
    x = sc0.getWidth() - 1;
  else if (x < 0)
    x = 0;
  if (y >= sc0.getHeight())  // yの有効範囲チェック
    y = sc0.getHeight() - 1;
  else if (y < 0)
    y = 0;

  // カーソル移動
  sc0.locate((uint16_t)x, (uint16_t)y);
}

void c_window_off(void) {
  sc0.setWindow(0, 0, sc0.getScreenWidth(), sc0.getScreenHeight());
  sc0.setScroll(true);
}

int c_window(int32_t x, int32_t y, int32_t w, int32_t h) {
  if (check_param(x, 0, sc0.getScreenWidth() - 1) ||
      check_param(y, 0, sc0.getScreenHeight() - 1) ||
      check_param(w, 1, sc0.getScreenWidth() - x) ||
      check_param(h, 1, sc0.getScreenHeight() - y))
    return -1;

  sc0.setWindow(x, y, w, h);
  sc0.setScroll(h > 1 ? true : false);
  sc0.locate(0, 0);
  sc0.show_curs(false);

  return 0;
}

int c_font(int32_t idx) {
  if (check_param(idx, 0, NUM_FONTS - 1))
    return -1;
  sc0.setFont(fonts[idx]);
  sc0.forget();
  return 0;
}

int c_screen(int32_t m) {
#ifdef USE_BG_ENGINE
  // Discard dimensions saved for CONTing.
  restore_bgs = false;
  restore_text_window = false;
#endif

  if (check_param(m, 1, vs23.numModes()))
    return -1;

  vs23.reset();

  if (scmode == m) {
    sc0.reset();
    sc0.setFont(fonts[CONFIG.font]);
    sc0.locate(0, 0);
    sc0.cls();
    sc0.show_curs(false);
    return 0;
  }

  // NTSCスクリーン設定
  if (!vs23.setMode(m - 1)) {
    E_ERR(IO, "cannot set screen mode");
    return -1;
  }

  sc0.end();
  scmode = m;

  sc0.init(SIZE_LINE, CONFIG.NTSC, m - 1);

  sc0.setFont(fonts[CONFIG.font]);
  sc0.cls();
  sc0.show_curs(false);
  sc0.draw_cls_curs();
  sc0.locate(0, 0);
  sc0.refresh();

  return 0;
}

int c_palette(int32_t p, int32_t hw, int32_t sw, int32_t vw, bool f) {
  if (check_param(p, 0, CSP_NUM_COLORSPACES - 1) ||
      check_param(hw, -1, 7) ||
      check_param(sw, -1, 7) ||
      check_param(vw, -1, 7))
    return -1;

  vs23.setColorSpace(p);
  if (hw != -1)
    csp.setColorConversion(p, hw, sw, vw, f);

  return 0;
}

int c_border(int32_t y, int32_t uv, int32_t x, int32_t w) {
  if (check_param(uv, 0, 255) ||
      check_param(y, 0, 255 - 0x66) ||
      check_param(x, -1, vs23.borderWidth()) ||
      check_param(w, -1, vs23.borderWidth() - x))
    return -1;

  if (x != -1)
    vs23.setBorder(y, uv, x, w);
  else
    vs23.setBorder(y, uv);

  return 0;
}

void c_vsync(uint32_t tm) {
  if (tm == 0)
    tm = vs23.frame() + 1;

  while (vs23.frame() < tm) {
    process_events();
    uint16_t c = sc0.peekKey();
    if (process_hotkeys(c)) {
      break;
    }
    yield();
  }
}

uint32_t c_frame(void) {
  return vs23.frame();
}

ipixel_t c_rgb_indexed(int32_t r, int32_t g, int32_t b) {
  if (r < 0) r = 0; else if (r > 255) r = 255;
  if (g < 0) g = 0; else if (g > 255) g = 255;
  if (b < 0) b = 0; else if (b > 255) b = 255;

  return csp.indexedColorFromRgb(r, g, b);
}

pixel_t c_rgb(int32_t r, int32_t g, int32_t b) {
  if (r < 0) r = 0; else if (r > 255) r = 255;
  if (g < 0) g = 0; else if (g > 255) g = 255;
  if (b < 0) b = 0; else if (b > 255) b = 255;

  return csp.colorFromRgb(r, g, b);
}

void c_color(pixel_t fc, pixel_t bgc) {
  sc0.setColor(fc, bgc);
}

void c_cursor_color(pixel_t cc) {
  sc0.setCursorColor(cc);
}

int32_t c_csize_height(void) {
  return sc0.getHeight();
}

int32_t c_csize_width(void) {
  return sc0.getWidth();
}

int32_t c_psize_height(void) {
  return sc0.getGHeight();
}

int32_t c_psize_width(void) {
  return sc0.getGWidth();
}

int32_t c_psize_lastline(void) {
  return vs23.lastLine();
}

int32_t c_pos_x(void) {
  return sc0.c_x();
}

int32_t c_pos_y(void) {
  return sc0.c_y();
}

uint16_t c_char_get(int32_t x, int32_t y) {
  return (x < 0 || y < 0 || x >=sc0.getWidth() || y >=sc0.getHeight()) ? 0 : sc0.vpeek(x, y);
}

void c_char_set(int32_t x, int32_t y, uint16_t c) {
  if (check_param(x, 0, sc0.getWidth() - 1) ||
      check_param(y, 0, sc0.getHeight() - 1))
    return;
  sc0.write(x, y, c);
}

int c_cscroll(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t d) {
  if (x1 < 0 || y1 < 0 || x2 < x1 || y2 < y1 ||
      x2 >= sc0.getWidth() ||
      y2 >= sc0.getHeight()) {
    err = ERR_VALUE;
    return -1;
  }
  if (d < 0 || d > 3)
    d = 0;
  sc0.cscroll(x1, y1, x2 - x1 + 1, y2 - y1 + 1, d);
  return 0;
}

int c_gscroll(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t d) {
  if (x1 < 0 || y1 < 0 || x2 <= x1 || y2 <= y1 ||
      x2 >= sc0.getGWidth() ||
      y2 >= vs23.lastLine()) {
    err = ERR_VALUE;
    return -1;
  }
  if (d < 0 || d > 3)
    d = 0;
  sc0.gscroll(x1, y1, x2 - x1 + 1, y2 - y1 + 1, d);
  return 0;
}

pixel_t c_point(int32_t x, int32_t y) {
  if (check_param(x, 0, sc0.getGWidth() - 1) ||
      check_param(y, 0, vs23.lastLine() - 1))
    return 0;

  return vs23.getPixel(x, y);
}

void c_pset(int32_t x, int32_t y, pixel_t c) {
  Graphics::setPixelSafe(x, y, c);
}

void c_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, pixel_t c) {
  if (c == (pixel_t)-1)
    c = fg_color;

  sc0.line(x1, y1, x2, y2, c);
}

void c_circle(int32_t x, int32_t y, int32_t r, pixel_t c, pixel_t f) {
  if (r < 0) r = -r;
  sc0.circle(x, y, r, c, f);
}

void c_rect(int32_t x1, int32_t y1, int32_t x2, int32_t y2, pixel_t c, pixel_t f) {
  sc0.rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1, c, f);
}

int c_blit(int32_t x, int32_t y, int32_t dx, int32_t dy, int32_t w, int32_t h) {
  if (check_param(x, 0, sc0.getGWidth() - 1) ||
      check_param(y, 0, vs23.lastLine() - 1) ||
      check_param(dx, 0, sc0.getGWidth() - 1) ||
      check_param(dy, 0, vs23.lastLine() - 1) ||
      check_param(w, 0, min(sc0.getGWidth() - x, sc0.getGWidth() - dx)) ||
      check_param(h, 0, min(vs23.lastLine() - y, vs23.lastLine() - dy)))
    return -1;

  vs23.blitRect(x, y, dx, dy, w, h);
  return 0;
}

} // extern "C"
