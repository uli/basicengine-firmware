// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "eb_api.h"
#include "eb_conio.h"

#include "basic.h"

void eb_locate(int x, int y) {
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

void eb_window_off(void) {
  sc0.setWindow(0, 0, sc0.getScreenWidth(), sc0.getScreenHeight());
  sc0.setScroll(true);
}

int eb_window(int x, int y, int w, int h) {
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

int eb_font(int idx) {
  if (check_param(idx, 0, NUM_FONTS - 1))
    return -1;
  sc0.setFont(fonts[idx]);
  sc0.forget();
  return 0;
}

int eb_pos_x(void) {
  return sc0.c_x();
}

int eb_pos_y(void) {
  return sc0.c_y();
}

uint16_t eb_char_get(int x, int y) {
  return (x < 0 || y < 0 || x >=sc0.getWidth() || y >=sc0.getHeight()) ? 0 : sc0.vpeek(x, y);
}

void eb_char_set(int x, int y, uint16_t c) {
  if (check_param(x, 0, sc0.getWidth() - 1) ||
      check_param(y, 0, sc0.getHeight() - 1))
    return;
  sc0.write(x, y, c);
}

int eb_cscroll(int x1, int y1, int x2, int y2, int d) {
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

int eb_csize_height(void) {
  return sc0.getHeight();
}

int eb_csize_width(void) {
  return sc0.getWidth();
}

int eb_getch(void) {
    return c_getch();
}

void eb_putch(int c) {
    c_putch(c);
}

void eb_clrtoeol(void) {
    sc0.clerLine(sc0.c_y(), sc0.c_x());
}

void eb_cls(void) {
    sc0.cls();
}

void eb_puts(const char *s) {
    c_puts(s);
}

void eb_show_cursor(int enable) {
    sc0.show_curs(enable);
}

void eb_enable_scrolling(int enable) {
    sc0.setScroll(enable);
}

int eb_kbhit(void) {
    return c_kbhit();
}
