// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void eb_locate(int32_t x, int32_t y);

void eb_window_off(void);
int eb_window(int32_t x, int32_t y, int32_t w, int32_t h);
int eb_font(int32_t idx);
int eb_screen(int32_t mode);
int eb_palette(int32_t p, int32_t hw, int32_t sw, int32_t vw, bool f);
int eb_border(int32_t y, int32_t uv, int32_t x, int32_t w);
void eb_vsync(uint32_t tm);
uint32_t eb_frame(void);
pixel_t eb_rgb(int32_t r, int32_t g, int32_t b);
ipixel_t eb_rgb_indexed(int32_t r, int32_t g, int32_t b);
void eb_color(pixel_t fc, pixel_t bgc);
void eb_cursor_color(pixel_t cc);
int32_t eb_csize_height(void);
int32_t eb_csize_width(void);
int32_t eb_psize_height(void);
int32_t eb_psize_width(void);
int32_t eb_psize_lastline(void);
int32_t eb_pos_x(void);
int32_t eb_pos_y(void);
uint16_t eb_char_get(int32_t x, int32_t y);
void eb_char_set(int32_t x, int32_t y, uint16_t c);
int eb_cscroll(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t d);
int eb_gscroll(int32_t x1, int32_t y1, int32_t x2, int32_t y2, int32_t d);
pixel_t eb_point(int32_t x, int32_t y);
void eb_pset(int32_t x, int32_t y, pixel_t c);
void eb_line(int32_t x1, int32_t y1, int32_t x2, int32_t y2, pixel_t c);
void eb_circle(int32_t x, int32_t y, int32_t r, pixel_t c, pixel_t f);
void eb_rect(int32_t x1, int32_t y1, int32_t x2, int32_t y2, pixel_t c, pixel_t f);
int eb_blit(int32_t x, int32_t y, int32_t dx, int32_t dy, int32_t w, int32_t h);

#ifdef __cplusplus
}
#endif
