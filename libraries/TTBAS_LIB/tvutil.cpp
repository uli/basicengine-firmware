//
// NTSC TV表示デバイスの管理
// 作成日 2017/04/11 by たま吉さん
// 修正日 2017/04/13, 横文字数算出ミス修正(48MHz対応)
// 修正日 2017/04/15, 行挿入の追加
// 修正日 2017/04/17, bitmap表示処理の追加
// 修正日 2017/04/18, cscroll,gscroll表示処理の追加
// 修正日 2017/04/25, gscrollの機能修正, gpeek,ginpの追加
// 修正日 2017/04/28, tv_NTSC_adjustの追加
// 修正日 2017/05/19, tv_getFontAdr()の追加
// 修正日 2017/05/30, SPI2(PA7 => PB15)に変更
// 修正日 2017/06/14, SPI2時のPB13ピンのHIGH設定対策
// 修正日 2017/07/25, tv_init()の追加
// 修正日 2017/07/29, スクリーンモード変更対応

#include "../../ttbasic/ttconfig.h"

#include <stdint.h>
#include <string.h>
#include "../../ttbasic/ntsc.h"
#include "../../ttbasic/vs23s010.h"
#include "tTVscreen.h"

extern uint8_t* ttbasic_font;

const uint8_t* tvfont = 0;     // 利用フォント
uint16_t c_width;    // 横文字数
uint16_t c_height;   // 縦文字数
#if USE_VS23 == 0
uint8_t* vram;       // VRAM先頭
uint8_t *b_adr;     // フレームバッファビットバンドアドレ
#endif
uint16_t f_width;    // フォント幅(ドット)
uint16_t f_height;   // フォント高さ(ドット)
uint16_t g_width;    // 画面横ドット数(ドット)
uint16_t g_height;   // 画面縦ドット数(ドット)
uint16_t win_x, win_y, win_width, win_height;
uint16_t win_c_width, win_c_height;
int clrline_x, clrline_y;

uint16_t fg_color = 0xf;
uint16_t bg_color = 0;

// フォント利用設定
void SMALL tv_fontInit() {
  if (!tvfont)
    tvfont = console_font_6x8;

  f_width  = pgm_read_byte(tvfont+0);             // 横フォントドット数
  f_height = pgm_read_byte(tvfont+1);             // 縦フォントドット数  
  c_width  = g_width  / f_width;       // 横文字数
  c_height = g_height / f_height;      // 縦文字数
  win_c_width = win_width / f_width;
  win_c_height = win_height / f_height;
}

void tv_setFont(const uint8_t *font)
{
    tvfont = font;
    tv_fontInit();
}

// NTSC 垂直同期信号補正
void tv_NTSC_adjust(int16_t ajst) {
  Serial.println("unimp tv_NTSC_adjust");
}

//
// NTSC表示の初期設定
// 
void tv_init(int16_t ajst, uint8_t* extmem, uint8_t vmode) { 
  vs23.setMode(vmode);
  g_width  = vs23.width();           // 横ドット数
  g_height = vs23.height();          // 縦ドット数

  // XXX: assumes font height == 8
  // We don't want to allocate this last because it will be in the way when
  // allocating a larger font later.
  vs23.allocBacking(g_width / 2, 8, clrline_x, clrline_y);

  win_x = 0;
  win_y = 0;
  win_width = g_width;
  win_height = g_height;
	
  tv_fontInit();

  tv_NTSC_adjust(ajst);
}

void tv_window_set(uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
  win_x = x * f_width;
  win_y = y * f_height;
  win_width = w * f_width;
  win_height = h * f_height;
  win_c_width  = win_width  / f_width;
  win_c_height = win_height / f_height;
}

void tv_window_reset()
{
  win_x = 0;
  win_y = 0;
  win_width = g_width;
  win_height = g_height;
  win_c_width  = win_width  / f_width;
  win_c_height = win_height / f_height;
}

void tv_window_get(int &x, int &y, int &w, int &h)
{
  x = win_x / f_width;
  y = win_y / f_height;
  w = win_width / f_width;
  h = win_height / f_height;
}

//
// NTSC表示の終了
// 
void tv_end() {
#if USE_VS23 == 0
  TV.end();
#endif
}

// フォントアドレス取得
const uint8_t* tv_getFontAdr() {
  return tvfont;
}

#if USE_VS23 == 0
// GVRAMアドレス取得
uint8_t* tv_getGVRAM() {
  return vram;
}
#endif

// GVRAMサイズ取得
uint16_t tv_getGVRAMSize() {
  return (g_width>>3)*g_height;
}

// 画面文字数横
uint8_t tv_get_cwidth() {
  return c_width;
}

// 画面文字数縦
uint8_t tv_get_cheight() {
  return c_height;
}

uint8_t tv_get_win_cwidth() {
  return win_c_width;
}

uint8_t tv_get_win_cheight() {
  return win_c_height;
}

// 画面グラフィック横ドット数
uint16_t tv_get_gwidth() {
  return g_width;
}

// 画面グラフィック縦ドット数
uint16_t tv_get_gheight() {
  return g_height;
}

//
// カーソル表示
//
void tv_drawCurs(uint8_t x, uint8_t y) {
  uint8_t pix[f_width];
  memset(pix, fg_color, f_width);
  for (int i = 0; i < f_height; ++i) {
    uint32_t byteaddress = vs23.piclineByteAddress(win_y + y*f_height+i)+ win_x + x*f_width;
    SpiRamWriteBytes(byteaddress, pix, f_width);
  }
}

//
// 文字の表示
//
void tv_write(uint8_t x, uint8_t y, uint8_t c) {
#if USE_VS23 == 1
  const uint8_t *chp = tvfont+3+c*f_height;
  for (int i=0;i<f_height;++i) {
    uint8_t pix[f_width];
    uint8_t ch = pgm_read_byte(chp+i);
    for (int j=0;j<f_width;++j) {
      pix[j] = ch&0x80 ? fg_color : bg_color;
      ch <<= 1;
    }
    uint32_t byteaddress = vs23.piclineByteAddress(win_y + y*f_height+i)+ win_x + x*f_width;
    SpiRamWriteBytes(byteaddress, pix, f_width);
  }
#else
  TV.print_char(x * f_width, y * f_height ,c);  
#endif
}

//
// 画面のクリア
//
void tv_cls() {
  for (int i = 0; i < win_c_height; ++i) {
    tv_clerLine(i);
  }
}

//
// 指定行の1行クリア
//
void tv_clerLine(uint16_t l) {
#if USE_VS23 == 1
  // Assumption: Screen data is followed by empty line in memory.
  vs23.MoveBlock(clrline_x, clrline_y, win_x, win_y + l * f_height, win_width/2, f_height, 0);
  vs23.MoveBlock(clrline_x, clrline_y, win_x + win_width/2, win_y + l * f_height, win_width/2, f_height, 0);
#else
  memset(vram + f_height*g_width/8*l, 0, f_height*g_width/8);
#endif
}

//
// 指定行に1行挿入(下スクロール)
//
void tv_insLine(uint16_t l) {
  if (l > win_c_height-1) {
    return;
  } else if (l == win_c_height-1) {
    tv_clerLine(l);
  } else {
    vs23.MoveBlock(win_x + win_width-1, win_y + win_height-f_height-1,
              win_x + win_width-1, win_y + win_height-1,
              win_width/2, win_height-f_height - l * f_height,
              1);
    vs23.MoveBlock(win_x + win_width/2-1, win_y + win_height-f_height-1,
              win_x + win_width/2-1, win_y + win_height-1,
              win_width/2, win_height-f_height - l * f_height,
              1);
    tv_clerLine(l);
  }
}

// 1行分スクリーンのスクロールアップ
void tv_scroll_up() {
#if USE_VS23 == 1
  vs23.MoveBlock(win_x, win_y + f_height, win_x, win_y, win_width/2, win_height-f_height, 0);
  vs23.MoveBlock(win_x + win_width/2, win_y + f_height, win_x + win_width/2, win_y, win_width/2, win_height-f_height, 0);
#else
  TV.shift(*(tvfont+1), UP);
#endif
  tv_clerLine(win_c_height-1);
}

// 1行分スクリーンのスクロールダウン
void tv_scroll_down() {
  vs23.MoveBlock(win_x + win_width-1, win_y + win_height-f_height-1,
            win_x + win_width-1, win_y + win_height-1,
            win_width/2, win_height-f_height,
            1);
  vs23.MoveBlock(win_x + win_width/2-1, win_y + win_height-f_height-1,
            win_x + win_width/2-1, win_y + win_height-1,
            win_width/2, win_height-f_height,
            1);
  tv_clerLine(0);
}

// 点の描画
void tv_pset(int16_t x, int16_t y, uint8_t c) {
  vs23.setPixel(x, y, c);
}
  
// 線の描画
void tv_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t c) {
  vs23.drawLine(x1, y1, x2, y2, c);
}

// 円の描画
void tv_circle(int16_t x, int16_t y, int16_t r, uint8_t c, int8_t f) {
  vs23.drawCircle(x, y, r, c, f);
}

// 四角の描画
void tv_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c, int8_t f) {
  vs23.drawRect(x, y, w, h, c, f);
}

// 指定サイズのドットの描画
inline void tv_dot(int16_t x, int16_t y, int16_t n, uint8_t c) {
#if USE_VS23 == 1
  Serial.println("unimp tv_dot");
#else
  uint8_t *adr;
  uint8_t bipo;
  for (int16_t i = y ; i < y+n; i++) {
    for (int16_t j= x; j < x+n; j++) {
      bipo = (j & 0xf8) + 7 - (j & 7);
      adr = b_adr + g_width*i/8 + bipo/8;
      *adr = (*adr & ~(1 << bipo)) | (c << bipo);
      //b_adr[g_width*i+ (j&0xf8) +7 -(j&7)] = c;
    }
  }
#endif
}

// 指定座標のピクセル取得
int16_t tv_gpeek(int16_t x, int16_t y) {
#if USE_VS23 == 1
  Serial.println("unimp tv_gpeek");
  return 0;
#else
  uint8_t *adr;
  uint8_t bipo;
      bipo = (x & 0xf8) + 7 - (x & 7);
      adr = b_adr + g_width*y/8 + bipo/8;
   return (*adr >> bipo) & 1;//b_adr[g_width*y+ (x&0xf8) +7 -(x&7)];
#endif
}

// 指定座標のピクセル有無のチェック
int16_t tv_ginp(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c) {
#if USE_VS23 == 1
  Serial.println("unimp tv_ginp");
  return 0;
#else
  for (int16_t i = y ; i < y+h; i++) {
    for (int16_t j= x; j < x+w; j++) {
      if (b_adr[g_width*i+ (j&0xf8) +7 -(j&7)] == c) {
          return 1;
      }
    }
  }
  return 0;
#endif
}

void tv_set_gcursor(uint16_t x, uint16_t y) {
#if USE_VS23 == 1
    Serial.println("unimp tv_set_gcursor");
#else
  TV.set_cursor(x,y);
#endif
}

void tv_write(uint8_t c) {
#if USE_VS23 == 1
    Serial.println("unimp tv_write");
#else
  TV.write(c);
#endif
}

// 音の再生
void tv_tone(int16_t freq, int16_t tm) {
  Serial.println("unimp tv_tone");
}

// 音の停止
void tv_notone() {
  Serial.println("unimp tv_notone");
}

// グラフィック横スクロール
void tv_gscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t mode) {
  switch (mode) {
    case 0:	// up
      vs23.MoveBlock(x,	        y + 1, x,         y, w / 2, h - 1, 0);
      vs23.MoveBlock(x + w / 2, y + 1, x + w / 2, y, w / 2, h - 1, 0);
      vs23.drawLine(x, y + h - 1, x + w - 1, y + h - 1, 0);
      break;
    case 1:	// down
      vs23.MoveBlock(x + w / 2 - 1, y + h - 1 - 1, x + w / 2 - 1, y + h - 1, w / 2, h - 1, 1);
      vs23.MoveBlock(x + w - 1,     y + h - 1 - 1, x + w - 1,     y + h - 1, w / 2, h - 1, 1);
      vs23.drawLine(x, y, x + w - 1, y, 0);
      break;
    case 2:	// left
      vs23.MoveBlock(x + 1,     y, x,             y, w / 2 - 1, h, 0);
      vs23.MoveBlock(x + w / 2, y, x + w / 2 - 1, y, w / 2,     h, 0);
      vs23.drawLine(x, y, x, y + h - 1, 0);
      break;
    case 3:	// right
      vs23.MoveBlock(x + w - 1 - 1, y + h - 1, x + w - 1,         y + h - 1, w / 2 - 1, h, 1);
      vs23.MoveBlock(x + w / 2 - 1, y + h - 1, x + w / 2 + 1 - 1, y + h - 1, w / 2,     h, 1);
      vs23.drawLine(x + w - 1, y, x + w - 1, y + h - 1, 0);
      break;
  }
}

void tv_setcolor(uint16_t fc, uint16_t bc)
{
  fg_color = fc;
  bg_color = bc;
}
