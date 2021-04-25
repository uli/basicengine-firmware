#ifdef __cplusplus
extern "C" {
#endif

int eb_getch(void);
void eb_putch(int c);
void eb_clrtoeol(void);
void eb_cls(void);
void eb_puts(const char *s);
void eb_show_cursor(int enable);
void eb_enable_scrolling(int enable);

void eb_locate(int x, int y);

void eb_window_off(void);
int eb_window(int x, int y, int w, int h);
int eb_font(int idx);

int eb_csize_height(void);
int eb_csize_width(void);

int eb_pos_x(void);
int eb_pos_y(void);

unsigned short eb_char_get(int x, int y);
void eb_char_set(int x, int y, unsigned short c);
int eb_cscroll(int x1, int y1, int x2, int y2, int d);

int eb_kbhit(void);

#ifdef __cplusplus
}
#endif
