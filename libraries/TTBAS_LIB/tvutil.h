#include "../../ttbasic/video.h"

void tv_init(int16_t ajst, uint8_t vmode=SC_DEFAULT);
void tv_end();
void    tv_write(uint8_t x, uint8_t y, uint8_t c);
void    tv_write_color(uint8_t x, uint8_t y, uint8_t c, uint8_t fg, uint8_t bg);
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

extern uint16_t f_width;
extern uint16_t f_height;
extern uint8_t cursor_color;

inline int tv_font_height()
{
  return f_height;
}
inline int tv_font_width()
{
  return f_width;
}

inline void tv_setcursorcolor(uint8_t cc)
{
  cursor_color = cc;
}

void tv_flipcolors();

extern uint16_t fg_color;
extern uint16_t bg_color;

extern int colmem_fg_x, colmem_fg_y;
extern int colmem_bg_x, colmem_bg_y;
