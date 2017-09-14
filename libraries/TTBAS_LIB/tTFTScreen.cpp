//
// 2017/08/12 修正 SPI2を利用に修正
//

#if 0

#include <string.h>
#include "tTFTScreen.h"

#define TFT_FONT_MODE 0 // フォント利用モード 0:TVFONT 1以上 Adafruit_GFX_ASフォント
#define TV_FONT_EX 1    // フォント倍率
extern uint8_t* ttbasic_font;
#if TFT_FONT_MODE == 0
//  #define TV_DISPLAY_FONT font6x8tt  // 利用フォント
//  #include <font6x8tt.h>             // 利用フォントのヘッダファイル
/*
  #define TV_DISPLAY_FONT ichigoFont8x8
  #include <ichigoFont8x8.h>
*/
#endif

//#include <mcurses.h>

//#define TFT_CS      PA0
#define TFT_CS      PB11
//#define TFT_RST     PA1
#define TFT_RST     -1
//#define TFT_DC      PA2
#define TFT_DC      PB12
#define F_BLACK                 0x0100
#define F_RED                   0x0200
#define F_GREEN                 0x0300
#define F_BROWN                 0x0400
#define F_BLUE                  0x0500
#define F_MAGENTA               0x0600
#define F_CYAN                  0x0700
#define F_WHITE                 0x0800
#define F_YELLOW                F_BROWN
#define F_COLOR                 0x0F00

#define B_BLACK                 0x1000
#define B_RED                   0x2000
#define B_GREEN                 0x3000
#define B_BROWN                 0x4000
#define B_BLUE                  0x5000
#define B_MAGENTA               0x6000
#define B_CYAN                  0x7000
#define B_WHITE                 0x8000
#define B_YELLOW                B_BROWN
#define B_COLOR                 0xF000

#define BOT_FIXED_AREA 0 // Number of lines in bottom fixed area (lines counted from bottom of screen)
#define TOP_FIXED_AREA 0 // Number of lines in top fixed area (lines counted from top of screen)
#define ILI9341_VSCRDEF  0x33
#define ILI9341_VSCRSADD 0x37

void setupPS2(uint8_t kb_type);
uint8_t ps2read();
void    endPS2();
/*
//******* mcurses用フック関数の定義(開始)  *****************************************
// シリアル経由1文字出力
static void Arduino_putchar(uint8_t c) {
  Serial.write(c);
}

// シリアル経由1文字入力
static char Arduino_getchar() {
  char c;
  while (!Serial.available());
  return Serial.read();
}
//******* mcurses用フック関数の定義(終了)  *****************************************
*/
// 依存デバイスの初期化
// シリアルコンソール mcursesの設定
void tTFTScreen::INIT_DEV() {
  tft = new Adafruit_ILI9341_STM_TT(TFT_CS, TFT_DC, TFT_RST,2); // Use hardware SPI
  //SPI.setModule(2);
/*
  ::setFunction_putchar(Arduino_putchar);  // 依存関数
  ::setFunction_getchar(Arduino_getchar);  // 依存関数
  ::initscr();                             // 依存関数
 */
  //tft = &_tft;

  tft->begin();
  tft->setRotation(3);
  tft->setCursor(0, 0);
  tft->fillScreen(ILI9341_BLACK);

#if TFT_FONT_MODE == 0
  fontEx = TV_FONT_EX; 
  tftfont  = (uint8_t*)ttbasic_font;
  f_width  = *(tftfont+0)*fontEx;      // 横フォントドット数
  f_height = *(tftfont+1)*fontEx;      // 縦フォントドット数
#endif

#if TFT_FONT_MODE > 0
  tft->setTextColor(ILI9341_WHITE,ILI9341_BLACK);
  tft->setTextSize(TFT_FONT_MODE);
  f_width  = 8*TFT_FONT_MODE;     // 横フォントドット数
  f_height = 8*TFT_FONT_MODE;     // 縦フォントドット数
#endif

  g_width  = tft->width();        // 横ドット数
  g_height = tft->height();       // 縦ドット数
  width  = g_width  / f_width;    // 横文字数
  height = g_height / f_height;   // 縦文字数
  fgcolor = ILI9341_WHITE;
  bgcolor = ILI9341_BLACK;

#if PS2DEV == 1
  setupPS2(0);
#endif

  // シリアルからの制御文字許可
  allowCtrl = true;
}

void tTFTScreen::reset_kbd(uint8_t kbd_type) {
  endPS2();
  setupPS2(kbd_type);
}

// 文字の出力
void tTFTScreen::putch(uint8_t c) {
  tscreenBase::putch(c);
 if(serialMode == 0) {
    Serial.write(c);       // シリアル出力
  } else if (serialMode == 1) {
    Serial1.write(c);     // シリアル出力  
  }
}

// 改行
void tTFTScreen::newLine() {  
  tscreenBase::newLine();
  if (serialMode == 0) {
    Serial.write(0x0d);
    Serial.write(0x0a);
  } else if (serialMode == 1) {
    Serial1.write(0x0d);
    Serial1.write(0x0a);
  }  
}

// キー入力チェック&キーの取得
uint8_t tTFTScreen::isKeyIn() {
  if(serialMode == 0) {
    if (Serial.available())
      return Serial.read();
  } else if (serialMode == 1) {
    if (Serial1.available())
      return Serial1.read();
  }
#if PS2DEV == 1
 return ps2read();
#endif
}

// 文字入力
uint8_t tTFTScreen::get_ch() {
  char c;
  while(1) {
    if(serialMode == 0) {
      if (Serial.available()) {
        c = Serial.read();
        dev = 1;
        break;
      }
    } else if (serialMode == 1) {
      if (Serial1.available()) {
        c = Serial1.read();
        dev = 2;
        break;
      }
    }
#if PS2DEV == 1
    c = ps2read();
    if (c) {
      dev = 0;
      break;
    }
#endif
  }
  return c;  
}

// シリアルポートスクリーン制御出力
void tTFTScreen::Serial_Ctrl(int16_t ch) {
  char* s=NULL;
  switch(ch) {
    case KEY_BACKSPACE:
     s = "\x08\x1b[P";
     break;
    case SC_KEY_CTRL_L:
     s = "\x1b[2J\x1b[H";
     break;
  }
  if(s) {
    if(serialMode == 0) {
      // Serial.print(s);     // USBシリアル出力
      while(*s) {
        Serial.write(*s);
        s++;
      }  
    } else if (serialMode == 1) {
      //Serial1.print(s);     // シリアル出力  
      while(*s) {
        Serial1.write(*s);
        s++;
      }  
    }  
  }
}


// スクリーンモード設定
void tTFTScreen::setScreen(uint8_t mode) {
  fontEx = mode;
  tftfont  = (uint8_t*)ttbasic_font;
  f_width  = *(tftfont+0)*fontEx;      // 横フォントドット数
  f_height = *(tftfont+1)*fontEx;      // 縦フォントドット数
  width  = g_width  / f_width;         // 横文字数
  height = g_height / f_height;        // 縦文字数
  fgcolor = ILI9341_WHITE;
  bgcolor = ILI9341_BLACK;
  tft->fillScreen(ILI9341_BLACK);  
}

// カーソル表示
uint8_t tTFTScreen::drawCurs(uint8_t x, uint8_t y) {
  uint8_t c;
  c = VPEEK(x, y);
#if TFT_FONT_MODE == 0
  tft->fillRect(x*f_width,y*f_height,f_width,f_height,fgcolor);
 if (fontEx == 1)
    tft->drawBitmap(x*f_width,y*f_height,tftfont+3+((uint16_t)c)*8,f_width,f_height, bgcolor);
 else
	drawBitmap_x2(x*f_width,y*f_height,tftfont+3+((uint16_t)c)*8,f_width/fontEx, f_height/fontEx, bgcolor);
#endif
#if TFT_FONT_MODE > 0
  tft->drawChar(x*f_width,y*f_height, c, ILI9341_BLACK, ILI9341_WHITE, TFT_FONT_MODE);
#endif
}

// カーソルの移動
// pos_x,pos_yは本関数のみでのみ変更可能

// カーソルの表示も行う
void tTFTScreen::MOVE(uint8_t y, uint8_t x) {
  uint8_t c;
  if (flgCur) {
    c = VPEEK(pos_x,pos_y);
    WRITE(pos_x, pos_y, c?c:32);  
    drawCurs(x, y);
  }
  pos_x = x;
  pos_y = y;
};

// 文字の表示
void tTFTScreen::WRITE(uint8_t x, uint8_t y, uint8_t c) {
#if TFT_FONT_MODE == 0
  tft->fillRect(x*f_width,y*f_height,f_width,f_height, bgcolor);
  if (fontEx == 1)
    tft->drawBitmap(x*f_width,y*f_height,tftfont+3+((uint16_t)c)*8,f_width,f_height, fgcolor);
  else
    drawBitmap_x2(x*f_width,y*f_height,tftfont+3+((uint16_t)c)*8,f_width/fontEx,f_height/fontEx, fgcolor);

#endif

#if TFT_FONT_MODE > 0
  tft->drawChar(x*f_width,y*f_height, c, fgcolor, bgcolor, TFT_FONT_MODE);
#endif

}
    
void tTFTScreen::CLEAR() {
  tft->fillScreen( bgcolor);
}

// 行の消去
void tTFTScreen::CLEAR_LINE(uint8_t l) {
  tft->fillRect(0,l*f_height,g_width,f_height,bgcolor);
}

void tTFTScreen::scrollFrame(uint16_t vsp) {
  tft->writecommand(ILI9341_VSCRSADD);
  tft->writedata(vsp >> 8);
  tft->writedata(vsp);
}

// スクロールアップ
void tTFTScreen::SCROLL_UP() {
 refresh();
}

// スクロールダウン
void tTFTScreen::SCROLL_DOWN() {
  INSLINE(0);
}

// 指定行に1行挿入(下スクロール)
void tTFTScreen::INSLINE(uint8_t l) {
 refresh();
}
/*
// キー入力チェック
uint8_t tTFTScreen::isKeyIn() {
  return Serial.available();
}
*/

// 文字入力
//uint8_t tTFTScreen::get_ch() {
//  return getch();

/*  
  char c;
  while (!Serial.available());
  return Serial.read();
*/
//}
/*
// キー入力チェック(文字参照)
int16_t tTFTScreen::peek_ch() {
  return Serial.peek();
}
*/
// カーソルの表示/非表示
// flg: カーソル非表示 0、表示 1
void tTFTScreen::show_curs(uint8_t flg) {
    flgCur = flg;
    if(flgCur)
      drawCurs(pos_x, pos_y);
    else 
      draw_cls_curs();
}

// カーソルの消去
void tTFTScreen::draw_cls_curs() {  
    uint8_t c = VPEEK(pos_x,pos_y);
    WRITE(pos_x, pos_y, c?c:32); 
}

// 文字色指定
void tTFTScreen::setColor(uint16_t fc, uint16_t bc) {

  static const uint16_t tbl_color[]  =
     {  ILI9341_BLACK, ILI9341_RED, ILI9341_GREEN, ILI9341_MAROON, ILI9341_BLUE, ILI9341_MAGENTA, ILI9341_CYAN, ILI9341_WHITE, ILI9341_YELLOW};

  static const uint16_t tbl_bcolor[]  =
     { B_BLACK,B_RED,B_GREEN,B_BROWN,B_BLUE,B_MAGENTA,B_CYAN,B_WHITE,B_YELLOW};

/*
  if ( (fc >= 0 && fc <= 8) && (bc >= 0 && bc <= 8) )
     attrset(tbl_color[fc]|tbl_color[bc]);  // 依存関数
*/
   fgcolor = tbl_color[fc];
   bgcolor = tbl_color[bc];
}

// 文字属性
void tTFTScreen::setAttr(uint16_t attr) {
/*  
  static const uint16_t tbl_attr[]  =
    { A_NORMAL, A_UNDERLINE, A_REVERSE, A_BLINK, A_BOLD };
  
  if (attr >= 0 && attr <= 4)
     attrset(tbl_attr[attr]);  // 依存関数
*/
}

void tTFTScreen::drawBitmap_x2(int16_t x, int16_t y,const uint8_t *bitmap, int16_t w, int16_t h,uint16_t color) {
  int16_t i, j;
  for(j=0; j<h; j++) {
    for(i=0; i<w; i++ ) {
      if(*(bitmap + j + i / 8) & (128 >> (i & 7))) {
          tft->fillRect(x+i*fontEx, y+j*fontEx, fontEx,fontEx, color);
      }
    }
  }  
}

#endif
