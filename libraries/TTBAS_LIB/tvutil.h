#include "../../ttbasic/video.h"

void tv_init(int16_t ajst, uint8_t vmode=SC_DEFAULT);
void tv_end();
void    tv_write(uint8_t x, uint8_t y, uint8_t c);
void    tv_write_color(uint8_t x, uint8_t y, uint8_t c, pixel_t fg, pixel_t bg);
void    tv_drawCurs(uint8_t x, uint8_t y);
void    tv_clerLine(uint16_t l, int from = 0) ;
void    tv_insLine(uint16_t l);
void    tv_cls();
void    tv_scroll_up();
void    tv_scroll_down();
uint8_t tv_get_cwidth();
uint8_t tv_get_cheight();
uint16_t tv_get_gwidth();
uint16_t tv_get_gheight();
uint8_t tv_get_win_cwidth();
uint8_t tv_get_win_cheight();
void    tv_write(uint8_t c);
void	tv_reinit();
void	tv_window_set(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void	tv_window_get(int &x, int &y, int &w, int &h);
void	tv_window_reset();
void	tv_setFont(const uint8_t *font);

uint16_t tv_get_gwidth();
uint16_t tv_get_gheight();
void    tv_pset(int16_t x, int16_t y, pixel_t c);
void    tv_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, pixel_t c);
void    tv_circle(int16_t x, int16_t y, int16_t r, pixel_t c, int f);
void    tv_rect(int16_t x, int16_t y, int16_t h, int16_t w, pixel_t c, int f) ;
void    tv_bitmap(int16_t x, int16_t y, uint8_t* adr, uint16_t index, uint16_t w, uint16_t h, uint16_t d);
void    tv_set_gcursor(uint16_t x, uint16_t y);
void    tv_gscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t mode) ;
void    tv_write(uint8_t c);

extern uint16_t f_width;
extern uint16_t f_height;
extern pixel_t cursor_color;

inline int tv_font_height()
{
  return f_height;
}
inline int tv_font_width()
{
  return f_width;
}

inline void tv_setcursorcolor(pixel_t cc)
{
  cursor_color = cc;
}

void tv_flipcolors();

extern pixel_t fg_color;
extern pixel_t bg_color;

extern int colmem_fg_x, colmem_fg_y;
extern int colmem_bg_x, colmem_bg_y;
