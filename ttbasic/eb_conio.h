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
int eb_last_key_event(void);

#define KEY_EVENT_BREAK 0x0100
#define KEY_EVENT_KEY   0x0200
#define KEY_EVENT_SHIFT 0x0400
#define KEY_EVENT_CTRL  0x0800
#define KEY_EVENT_ALT   0x1000
#define KEY_EVENT_ALTGR 0x2000
#define KEY_EVENT_GUI   0x4000

#ifdef __cplusplus
}
#endif
