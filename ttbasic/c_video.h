// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include <stdint.h>

extern "C" {

void c_locate(int32_t x, int32_t y);

void c_window_off(void);
int c_window(int32_t x, int32_t y, int32_t w, int32_t h);
int c_font(int32_t idx);
int c_screen(int32_t mode);
int c_palette(int32_t p, int32_t hw, int32_t sw, int32_t vw, bool f);
int c_border(int32_t y, int32_t uv, int32_t x, int32_t w);
void c_vsync(uint32_t tm);
uint32_t c_frame(void);
pixel_t c_rgb(int32_t r, int32_t g, int32_t b);
ipixel_t c_rgb_indexed(int32_t r, int32_t g, int32_t b);
void c_color(pixel_t fc, pixel_t bgc);
void c_cursor_color(pixel_t cc);
int32_t c_csize_height(void);
int32_t c_csize_width(void);
int32_t c_psize_height(void);
int32_t c_psize_width(void);
int32_t c_psize_lastline(void);
int32_t c_pos_x(void);
int32_t c_pos_y(void);
uint16_t c_char_get(int32_t x, int32_t y);
void c_char_set(int32_t x, int32_t y, uint16_t c);
int c_cscroll(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t d);
int c_gscroll(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t d);
pixel_t c_point(int32_t x, int32_t y);
void c_pset(int32_t x, int32_t y, pixel_t c);
void c_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, pixel_t c);
void c_circle(int32_t x, int32_t y, int32_t r, pixel_t c, pixel_t f);
void c_rect(int32_t x1, int32_t y1, int32_t x2, int32_t y2, pixel_t c, pixel_t f);
int c_blit(int32_t x, int32_t y, int32_t dx, int32_t dy, int32_t w, int32_t h);

}
