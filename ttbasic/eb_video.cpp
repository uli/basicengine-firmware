// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "basic.h"
#include "graphics.h"

#include "eb_sys.h"
#include "eb_video.h"
#include "eb_api.h"

extern "C" {

// **** スクリーン管理 *************
static uint8_t scmode = 0;

int eb_screen(int m) {
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
    eb_font(CONFIG.font);
    sc0.locate(0, 0);
    sc0.cls();
    sc0.show_curs(false);
    return 0;
  }

  // NTSCスクリーン設定
  if (!vs23.setMode(m)) {
    E_ERR(IO, "cannot set screen mode");
    return -1;
  }

  sc0.end();
  scmode = m;

  sc0.init(SIZE_LINE, CONFIG.NTSC, m - 1);

  eb_font(CONFIG.font);
  sc0.cls();
  sc0.show_curs(false);
  sc0.draw_cls_curs();
  sc0.locate(0, 0);
  sc0.refresh();

  return 0;
}

int eb_palette(int p, int hw, int sw, int vw, int f) {
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

int eb_border(int y, int uv, int x, int w) {
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

void eb_vsync(unsigned int tm) {
  if (tm == 0)
    tm = vs23.frame() + 1;

  while (vs23.frame() < tm) {
    if (eb_process_events_wait())
      break;
  }
}

unsigned int eb_frame(void) {
  return vs23.frame();
}

ipixel_t eb_rgb_indexed(int r, int g, int b) {
  if (r < 0) r = 0; else if (r > 255) r = 255;
  if (g < 0) g = 0; else if (g > 255) g = 255;
  if (b < 0) b = 0; else if (b > 255) b = 255;

  return csp.indexedColorFromRgb(r, g, b);
}

pixel_t eb_rgb(int r, int g, int b) {
  if (r < 0) r = 0; else if (r > 255) r = 255;
  if (g < 0) g = 0; else if (g > 255) g = 255;
  if (b < 0) b = 0; else if (b > 255) b = 255;

  return csp.colorFromRgb(r, g, b);
}

pixel_t eb_rgb_from_indexed(ipixel_t c) {
  return csp.fromIndexed(c);
}

void eb_color(pixel_t fc, pixel_t bgc) {
  sc0.setColor(fc, bgc);
}

void eb_cursor_color(pixel_t cc) {
  sc0.setCursorColor(cc);
}

int eb_psize_height(void) {
  return sc0.getGHeight();
}

int eb_psize_width(void) {
  return sc0.getGWidth();
}

int eb_psize_lastline(void) {
  return vs23.lastLine();
}

int eb_gscroll(int x1, int y1, int x2, int y2, int d) {
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

pixel_t eb_point(int x, int y) {
  if (check_param(x, 0, sc0.getGWidth() - 1) ||
      check_param(y, 0, vs23.lastLine() - 1))
    return 0;

  return vs23.getPixel(x, y);
}

void eb_pset(int x, int y, pixel_t c) {
  Graphics::setPixelSafe(x, y, c);
}

void eb_line(int x1, int y1, int x2, int y2, pixel_t c) {
  if (c == (pixel_t)-1)
    c = fg_color;

  sc0.line(x1, y1, x2, y2, c);
}

void eb_circle(int x, int y, int r, pixel_t c, pixel_t f) {
  if (r < 0) r = -r;
  sc0.circle(x, y, r, c, f);
}

void eb_rect(int x1, int y1, int x2, int y2, pixel_t c, pixel_t f) {
  sc0.rect(x1, y1, x2 - x1 + 1, y2 - y1 + 1, c, f);
}

static void clampRect(int &x, int &y, int &dx, int &dy, int &w, int &h) {
  if (x + w >= sc0.getGWidth())
    w = sc0.getGWidth() - x;
  if (y + h >= vs23.lastLine())
    h = vs23.lastLine() - y;

  if (dx < 0) {
    w += dx;
    x -= dx;
    dx = 0;
  }
  if (dy < 0) {
    h += dy;
    y -= dy;
    dy = 0;
  }
  if (dx + w >= sc0.getGWidth())
    w = sc0.getGWidth() - dx;
  if (dy + h >= vs23.lastLine())
    h = vs23.lastLine() - dy;
}

int eb_blit(int x, int y, int dx, int dy, int w, int h) {
  if (check_param(x, 0, sc0.getGWidth() - 1) ||
      check_param(y, 0, vs23.lastLine() - 1) ||
      check_param(w, 0, sc0.getGWidth()) ||
      check_param(h, 0, vs23.lastLine()))
    return -1;

  clampRect(x, y, dx, dy, w, h);

  if (w > 0 && h > 0)
    vs23.blitRect(x, y, dx, dy, w, h);

  return 0;
}

int eb_blit_alpha(int x, int y, int dx, int dy, int w, int h) {
  if (check_param(x, 0, sc0.getGWidth() - 1) ||
      check_param(y, 0, vs23.lastLine() - 1) ||
      check_param(w, 0, sc0.getGWidth()) ||
      check_param(h, 0, vs23.lastLine()))
    return -1;

  clampRect(x, y, dx, dy, w, h);

  if (w > 0 && h > 0)
    vs23.blitRectAlpha(x, y, dx, dy, w, h);

  return 0;
}

pixel_t eb_get_fg_color(void) {
  return sc0.getFgColor();
}

pixel_t eb_get_bg_color(void) {
  return sc0.getBgColor();
}

} // extern "C"
