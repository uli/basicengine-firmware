#ifdef __cplusplus
extern "C" {
#endif

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

#ifdef __cplusplus
}
#endif
