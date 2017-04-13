//
// NTSC TV表示デバイスの管理
//
//

#include <TTVout.h>
#include "tscreen.h"

// NTSC TVout画面表示用フォントの設定


#define TV_DISPLAY_FONT font6x8
#include <font6x8.h>

/*
#define TV_DISPLAY_FONT font8x8
#include <font8x8.h>
*/
/*
#define TV_DISPLAY_FONT ichigoFont8x8 
#include <ichigoFont8x8.h>
*/

// 1文字表示
#define ESC_CMD_INIT     0 // 初期
#define ESC_CMD_ESC      1 // ESC受理
#define ESC_CMD_CMD      2 // '['受理
#define ESC_CMD_PRM1     3 // 引数1受理
#define ESC_CMD_USEPRM2  4 // 要引数2受理
#define ESC_CMD_PRM2     5 // 引数2受理
#define ESC_CMD_CMD_QS   6 // '?'受理

TTVout TV;
uint8_t* tvfont;     // 利用フォント
uint16_t c_width;    // 横文字数
uint16_t c_height;   // 縦文字数
uint8_t* vram;       // VRAM先頭
uint16_t f_width;    // フォント幅(ドット)
uint16_t f_height;   // フォント高さ(ドット)

//
// NTSC表示の初期設定
// 
void tv_init() {
  tvfont   = (uint8_t*)TV_DISPLAY_FONT;
  f_width  = *(tvfont+0);
  f_height = *(tvfont+1);
  TV.begin();
  TV.select_font(tvfont);

  c_width  = TV.hres() / f_width;   // 横文字数
  c_height = TV.vres() / f_height;  // 縦文字数
  vram = TV.VRAM();                 // VRAM先頭
}

// 画面文字数横
uint8_t tv_get_cwidth() {
  return c_width;
}

// 画面文字数縦
uint8_t tv_get_cheight() {
  return c_height;
}

// 画面グラフィック横ドット数
uint8_t tv_get_gwidth() {
  return TV.hres();
}

// 画面グラフィック縦ドット数
uint8_t tv_get_gheight() {
  return TV.vres();
}

//
// カーソル表示
//
uint8_t tv_drawCurs(uint8_t x, uint8_t y) {
  for (uint16_t i = 0; i < f_height; i++)
     for (uint16_t j = 0; j < f_width; j++)
     TV.set_pixel(x*f_width+j, y*f_height+i,2);
}

//
// 文字の表示
//
void tv_write(uint8_t x, uint8_t y, uint8_t c) {
  TV.print_char(x * f_width, y * f_height ,c);  
}

//
// 画面のクリア
//
void tv_cls() {
  TV.cls();
}

//
// 指定行の1行クリア
//
void tv_clerLine(uint16_t l) {
  memset(vram + f_height*TV.hres()/8*l, 0, f_height*TV.hres()/8);
}

// 1行分スクリーンのスクロールアップ
void tv_scroll_up() {
  TV.shift(*(tvfont+1), UP);
  tv_clerLine(c_height-1);
}

// 点の描画
void tv_pset(int16_t x, int16_t y, uint8_t c) {
  TV.set_pixel(x,y,c);
}
  
// 線の描画
void tv_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t c) {
  TV.draw_line(x1,y1,x2,y2,c);
}

//円の描画
void tv_circle(int16_t x, int16_t y, int16_t r, uint8_t c, int8_t f) {
  if (f==0) f=-1;
  TV.draw_circle(x, y, r, c, f);
}

//四角の描画
void tv_rect(int16_t x, int16_t y, int16_t h, int16_t w, uint8_t c, int8_t f) {
  if (f==0) f=-1;
  TV.draw_rect(x, y, w, h, c, f);
}

