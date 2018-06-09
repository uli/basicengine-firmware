void tv_init(int16_t ajst, uint8_t vmode=SC_DEFAULT);
void tv_end();
void    tv_write(uint8_t x, uint8_t y, uint8_t c);
void    tv_drawCurs(uint8_t x, uint8_t y);
void    tv_clerLine(uint16_t l, int from = 0) ;
void    tv_insLine(uint16_t l);
void    tv_cls();
void    tv_scroll_up();
void    tv_scroll_down();
uint8_t tv_get_cwidth();
uint8_t tv_get_cheight();
uint8_t tv_get_win_cwidth();
uint8_t tv_get_win_cheight();
void    tv_write(uint8_t c);
void    tv_tone(int16_t freq, int16_t tm);
void    tv_notone();
void    tv_NTSC_adjust(int16_t ajst);
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
