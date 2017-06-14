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

#include <TTVout.h>
#include "tscreen.h"
#define NTSC_VIDEO_SPI 2
/*
#define TV_DISPLAY_FONT font6x8
#include <font6x8.h>
*/

#define TV_DISPLAY_FONT font6x8tt
#include <font6x8tt.h>

/*
#define TV_DISPLAY_FONT font8x8
#include <font8x8.h>
*/

/*
#define TV_DISPLAY_FONT ichigoFont8x8 
#include <ichigoFont8x8.h>
*/

TTVout TV;
uint8_t* tvfont;     // 利用フォント
uint16_t c_width;    // 横文字数
uint16_t c_height;   // 縦文字数
uint8_t* vram;       // VRAM先頭
uint16_t f_width;    // フォント幅(ドット)
uint16_t f_height;   // フォント高さ(ドット)
uint16_t g_width;    // 画面横ドット数(ドット)
uint16_t g_height;   // 画面縦ドット数(ドット)
uint32_t *b_adr;     // フレームバッファビットバンドアドレ

// NTSC 垂直同期信号補正
void tv_NTSC_adjust(int16_t ajst) {
  TNTSC.adjust(ajst);  
}

//
// NTSC表示の初期設定
// 
void tv_init(int16_t ajst) {
  tvfont   = (uint8_t*)TV_DISPLAY_FONT;
  f_width  = *(tvfont+0);             // 横フォントドット数
  f_height = *(tvfont+1);             // 縦フォントドット数
  
  TNTSC.adjust(ajst);
  TV.begin(SC_DEFAULT,NTSC_VIDEO_SPI); // SPI2を利用

#if NTSC_VIDEO_SPI == 2
  // SPI2 SCK2(PB13ピン)が起動直後にHIGHになっている修正
   pinMode(PB13, INPUT);  
#endif

  TV.select_font(tvfont);
  g_width  = TNTSC.width();           // 横ドット数
  g_height = TNTSC.height();          // 縦ドット数
  
  c_width  = g_width  / f_width;       // 横文字数
  c_height = g_height / f_height;      // 縦文字数
  vram = TV.VRAM();                    // VRAM先頭
  
  b_adr =  (uint32_t*)(BB_SRAM_BASE + ((uint32_t)vram - BB_SRAM_REF) * 32);
}

// フォントアドレス取得
uint8_t* tv_getFontAdr() {
  return tvfont;
}

// GVRAMアドレス取得
uint8_t* tv_getGVRAM() {
  return vram;
}

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
  memset(vram + f_height*g_width/8*l, 0, f_height*g_width/8);
}

//
// 指定行に1行挿入(下スクロール)
//
void tv_insLine(uint16_t l) {
  if (l > c_height-1) {
    return;
  } else if (l == c_height-1) {
    tv_clerLine(l);
  } else {
    uint8_t* src = vram + f_height*g_width/8*l;      // 移動元
    uint8_t* dst = vram + f_height*g_width/8*(l+1) ; // 移動先
    uint16_t sz = f_height*g_width/8*(c_height-1-l);   // 移動量
    memmove(dst, src, sz);
    tv_clerLine(l);
  }
}

// 1行分スクリーンのスクロールアップ
void tv_scroll_up() {
  TV.shift(*(tvfont+1), UP);
  tv_clerLine(c_height-1);
}

// 1行分スクリーンのスクロールダウン
void tv_scroll_down() {
  TV.shift(*(tvfont+1), DOWN);
  tv_clerLine(0);
}

// 点の描画
void tv_pset(int16_t x, int16_t y, uint8_t c) {
  TV.set_pixel(x,y,c);
}
  
// 線の描画
void tv_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t c) {
  TV.draw_line(x1,y1,x2,y2,c);
}

// 円の描画
void tv_circle(int16_t x, int16_t y, int16_t r, uint8_t c, int8_t f) {
  if (f==0) f=-1;
  TV.draw_circle(x, y, r, c, f);
}

// 四角の描画
void tv_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c, int8_t f) {
  if (f==0) f=-1;
  TV.draw_rect(x, y, w, h, c, f);
}

// 指定サイズのドットの描画
inline void tv_dot(int16_t x, int16_t y, int16_t n, uint8_t c) {
  for (int16_t i = y ; i < y+n; i++) {
    for (int16_t j= x; j < x+n; j++) {
      b_adr[g_width*i+ (j&0xf8) +7 -(j&7)] = c;
    }
  }
}

// 指定座標のピクセル取得
int16_t tv_gpeek(int16_t x, int16_t y) {
   return b_adr[g_width*y+ (x&0xf8) +7 -(x&7)];
}

// 指定座標のピクセル有無のチェック
int16_t tv_ginp(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c) {
  for (int16_t i = y ; i < y+h; i++) {
    for (int16_t j= x; j < x+w; j++) {
      if (b_adr[g_width*i+ (j&0xf8) +7 -(j&7)] == c) {
          return 1;
      }
    }
  }
  return 0;
}


// ビットマップ表示
void tv_bitmap(int16_t x, int16_t y, uint8_t* adr, uint16_t index, uint16_t w, uint16_t h, uint16_t n) {
    uint8_t  nb = (w+7)/8; // 横バイト数の取得
    uint8_t  d;
    int16_t xx, yy, b;
    adr+= nb*h*index;
  
  if (n == 1) {
    // 1倍の場合
    TV.bitmap(x, y, adr  , 0, w, h);
  } else {
    // N倍の場合
    yy = y;
    for (uint16_t i = 0; i < h; i++) {
      xx = x;
      for (uint16_t j = 0; j < nb; j++) {
        d = adr[nb*i+j];
        b = (int16_t)w-8*j-8>=0 ? 8:w % 8;
        for(uint8_t k = 0; k < b; k++) {
          tv_dot(xx, yy, n, d>>(7-k) & 1);
          xx+= n;
        }
      }
      yy+=n;
    }
  }
}

void tv_set_gcursor(uint16_t x, uint16_t y) {
  TV.set_cursor(x,y);
}

void tv_write(uint8_t c) {
  TV.write(c);
}

// 音の再生
void tv_tone(int16_t freq, int16_t tm) {
  TV.tone(freq, tm);  
}

// 音の停止
void tv_notone() {
  TV.noTone();    
}

// グラフィック横スクロール
void tv_gscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t mode) {
  uint8_t* bmp = vram+(g_width>>3)*y; // フレームバッファ参照位置 
  uint16_t bl = (g_width+7)>>3;       // 横バイト数
  uint16_t sl = (w+7)>>3;             // 横スクロールバイト数
  uint16_t xl = (x+7)>>3;             // 横スクロール開始オフセット(バイト)
  
  uint16_t addr;                      // データアドレス
  uint8_t prv_bit;                    // 直前のドット
  uint8_t d;                          // 取り出しデータ
   
  switch(mode) {
      case 0: // 上
        addr=xl;   
        for (int16_t i=0; i<h-1;i++) {
          memcpy(&bmp[addr],&bmp[addr+bl], sl);
          addr+=bl;
        }
        memset(&bmp[addr], 0, sl);
        break;                   
      case 1: // 下
        addr=bl*(h-1)+xl;
        for (int16_t i=0; i<h-1;i++) {
          memcpy(&bmp[addr],&bmp[addr-bl], sl);
          addr-=bl;
        }
        memset(&bmp[addr], 0, sl);
       break;                          
     case 2: // 右
      addr=xl;
      for (int16_t i=0; i < h;i++) {
        prv_bit = 0;
        for (int16_t j=0; j < sl; j++) {
          d = bmp[addr+j];
          bmp[addr+j]>>=1;
          if (j>0)
            bmp[addr+j] |= prv_bit;
          prv_bit=d<<7;        
        }
        addr+=bl;
      } 
      break;                              
     case 3: // 左
        addr=xl;
        for (int16_t i=0; i < h;i++) {
          prv_bit = 0;
          for (int16_t j=0; j < sl; j++) {
            d = bmp[addr+sl-1-j];
            bmp[addr+sl-1-j]<<=1;
            if (j>0)
              bmp[addr+sl-1-j] |= prv_bit;
            prv_bit=d>>7;
          }
          addr+=bl;
        }
       break;              
   }
}

