// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#ifdef __cplusplus
extern "C" {
#endif

void eb_locate(int x, int y);

void eb_window_off(void);
int eb_window(int x, int y, int w, int h);
int eb_font(int idx);
int eb_screen(int mode);
int eb_palette(int p, int hw, int sw, int vw, bool f);
int eb_border(int y, int uv, int x, int w);
void eb_vsync(unsigned int tm);
unsigned int eb_frame(void);
pixel_t eb_rgb(int r, int g, int b);
ipixel_t eb_rgb_indexed(int r, int g, int b);
void eb_color(pixel_t fc, pixel_t bgc);
void eb_cursor_color(pixel_t cc);
int eb_csize_height(void);
int eb_csize_width(void);
int eb_psize_height(void);
int eb_psize_width(void);
int eb_psize_lastline(void);
int eb_pos_x(void);
int eb_pos_y(void);
unsigned short eb_char_get(int x, int y);
void eb_char_set(int x, int y, unsigned short c);
int eb_cscroll(int x1, int y1, int x2, int y2, int d);
int eb_gscroll(int x1, int y1, int x2, int y2, int d);
pixel_t eb_point(int x, int y);
void eb_pset(int x, int y, pixel_t c);
void eb_set_alpha(int x, int y, unsigned char alpha);
void eb_line(int x1, int y1, int x2, int y2, pixel_t c);
void eb_circle(int x, int y, int r, pixel_t c, pixel_t f);
void eb_rect(int x1, int y1, int x2, int y2, pixel_t c, pixel_t f);
int eb_blit(int x, int y, int dx, int dy, int w, int h);
int eb_blit_alpha(int x, int y, int dx, int dy, int w, int h);

#ifdef __cplusplus
}
#endif
