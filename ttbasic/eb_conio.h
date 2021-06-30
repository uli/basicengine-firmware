// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

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
void eb_enable_escape_codes(int enable);

void eb_locate(int x, int y);

void eb_window_off(void);
int eb_window(int x, int y, int w, int h);
int eb_font(int idx);
int eb_font_by_name(const char *name, int w, int h);
int eb_load_font(const char *file_name);
int eb_font_count(void);

int eb_csize_height(void);
int eb_csize_width(void);

int eb_pos_x(void);
int eb_pos_y(void);

unsigned int eb_char_get(int x, int y);
void eb_char_set(int x, int y, unsigned int c);
int eb_cscroll(int x1, int y1, int x2, int y2, int d);

int eb_kbhit(void);
int eb_last_key_event(void);

int eb_term_getch(void);
void eb_term_putch(char c);

int eb_load_lang_resources(int lang);

char *eb_screened_get_line(void);

void eb_add_output_filter(int (*filter)(int c, void *userdata), void *userdata);

#define KEY_EVENT_BREAK 0x0100
#define KEY_EVENT_KEY   0x0200
#define KEY_EVENT_SHIFT 0x0400
#define KEY_EVENT_CTRL  0x0800
#define KEY_EVENT_ALT   0x1000
#define KEY_EVENT_ALTGR 0x2000
#define KEY_EVENT_GUI   0x4000

// Definition of edit key
#define SC_KEY_TAB       '\t'   // [TAB] key
#define SC_KEY_CR        '\r'   // [Enter] key
#define SC_KEY_BACKSPACE '\b'   // [Backspace] key
#define SC_KEY_ESCAPE    0x1B   // [ESC] key
#define SC_KEY_DOWN      0x180   // [↓] key
#define SC_KEY_UP        0x181   // [↑] key
#define SC_KEY_LEFT      0x182   // [←] key
#define SC_KEY_RIGHT     0x183   // [→] key
#define SC_KEY_HOME      0x184   // [Home] key
#define SC_KEY_DC        0x185   // [Delete] key
#define SC_KEY_IC        0x186   // [Insert] key
#define SC_KEY_NPAGE     0x187   // [PageDown] key
#define SC_KEY_PPAGE     0x188   // [PageUp] key
#define SC_KEY_END       0x189   // [End] key
#define SC_KEY_BTAB      0x18A   // [Back tab] key
#define SC_KEY_F1                  0x18B            // Function key F1
#define SC_KEY_F(n)                (SC_KEY_F1+(n)-1)  // Space for additional 12 function keys

#define SC_KEY_PRINT     0x1A0

#define SC_KEY_SHIFT_DOWN	SC_KEY_DOWN | 0x40
#define SC_KEY_SHIFT_UP	SC_KEY_UP | 0x40

// Definition of control key code
#define SC_KEY_CTRL_L   12  // 画面を消去
#define SC_KEY_CTRL_R   18  // 画面を再表示
#define SC_KEY_CTRL_X   24  // 1文字削除(DEL)
#define SC_KEY_CTRL_C    3  // break
#define SC_KEY_CTRL_D    4  // 行削除
#define SC_KEY_CTRL_N   14  // 行挿入

#ifdef __cplusplus
}
#endif
