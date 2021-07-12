// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#ifdef __cplusplus
extern "C" {
#endif

#include "eb_types.h"

int eb_screen(int mode);
int eb_palette(int p, int hw, int sw, int vw, int f);
int eb_border(int y, int uv, int x, int w);
void eb_vsync(unsigned int tm);
unsigned int eb_frame(void);
pixel_t eb_rgb(int r, int g, int b);
ipixel_t eb_rgb_indexed(int r, int g, int b);
pixel_t eb_rgb_from_indexed(ipixel_t c);
void eb_color(pixel_t fc, pixel_t bgc);
void eb_cursor_color(pixel_t cc);
int eb_psize_height(void);
int eb_psize_width(void);
int eb_psize_lastline(void);
int eb_gscroll(int x1, int y1, int x2, int y2, int d);
pixel_t eb_point(int x, int y);
void eb_pset(int x, int y, pixel_t c);
void eb_line(int x1, int y1, int x2, int y2, pixel_t c);
void eb_circle(int x, int y, int r, pixel_t c, pixel_t f);
void eb_rect(int x1, int y1, int x2, int y2, pixel_t c, pixel_t f);
int eb_blit(int x, int y, int dx, int dy, int w, int h);
int eb_blit_alpha(int x, int y, int dx, int dy, int w, int h);
pixel_t eb_get_fg_color(void);
pixel_t eb_get_bg_color(void);

int eb_mode_from_size(int *w, int *h);
int eb_num_modes(void);
void eb_mode_size(int m, int *w, int *h);

#ifdef __cplusplus
}
#endif
