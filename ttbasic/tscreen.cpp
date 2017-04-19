//
// file:tscreen.h
// ターミナルスクリーン制御ライブラリ for Arduino STM32
//  V1.0 作成日 2017/03/22 by たま吉さん
//  修正日 2017/03/26, 色制御関連関数の追加
//  修正日 2017/03/30, moveLineEnd()の追加,[HOME],[END]の編集キーの仕様変更
//  修正日 2017/04/09, PS/2キーボードとの併用可能
//  修正日 2017/04/11, TVout版に移行
//  修正日 2017/04/15, 行挿入の追加(次行上書き防止対策）
//  修正日 2017/04/19, 行確定時の不具合対応
//

#include <string.h>
#include "tscreen.h"

void    tv_init();
void    tv_write(uint8_t x, uint8_t y, uint8_t c);
uint8_t tv_drawCurs(uint8_t x, uint8_t y);
void    tv_clerLine(uint16_t l) ;
void    tv_insLine(uint16_t l);
void    tv_cls();
void    tv_scroll_up();
uint8_t tv_get_cwidth();
uint8_t tv_get_cheight();
uint8_t tv_get_gwidth();
uint8_t tv_get_gheight();
void    tv_pset(int16_t x, int16_t y, uint8_t c);
void    tv_line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t c);
void    tv_circle(int16_t x, int16_t y, int16_t r, uint8_t c, int8_t f);
void    tv_rect(int16_t x, int16_t y, int16_t h, int16_t w, uint8_t c, int8_t f) ;
void    tv_tone(int16_t freq, int16_t tm);
void    tv_notone();

void    setupPS2();
uint8_t ps2read();

#define VPEEK(X,Y)      (screen[width*(Y)+(X)])
#define VPOKE(X,Y,C)    (screen[width*(Y)+(X)]=C)

// USBシリアル利用設定
void tscreen::setSerial(uint8_t flg) {
  flgSirialout = flg;
}

// カーソルの移動
// ※pos_x,pos_yは本関数のみでのみ変更可能
void tscreen::MOVE(uint8_t y, uint8_t x) {
  uint8_t c;
  if (flgCur) {
    c = VPEEK(pos_x,pos_y);
    tv_write(pos_x, pos_y, c?c:32);  
    tv_drawCurs(x, y);  
  }
  pos_x = x;
  pos_y = y;
}

// スクリーンの初期設定
// 引数
//  w  : スクリーン横文字数
//  h  : スクリーン縦文字数
//  l  : 1行の最大長
// 戻り値
//  なし
//void tscreen::init(uint16_t w, uint16_t h, uint16_t l) {
void tscreen::init(uint16_t l) {

  // ビデオ出力設定
  tv_init();
    
  width   = tv_get_cwidth();
  height  = tv_get_cheight();
  gwidth   = tv_get_gwidth();
  gheight  = tv_get_gheight();
  maxllen = l;
  setupPS2();
  if (screen != NULL) 
    free(screen);
  screen = (uint8_t*)malloc( width * height );

  delay(200);
  cls();
  show_curs(true);
  MOVE(0, 0);

  // 編集機能の設定
  flgIns = true;
}

// キー入力チェック&キーの取得
uint8_t tscreen::isKeyIn() {
  if (flgSirialout) {
    if (Serial.available())
      return Serial.read();
  }  
  return ps2read();  
}

// 文字入力
uint8_t tscreen::get_ch() {
  char c;
  while(1) {
    if (flgSirialout) {
      if (Serial.available()) {
        c = Serial.read();
        break;
      }
    }
    c = ps2read();
    if (c)
      break;
  }
  return c;  
}
/*
// キー入力チェック(文字参照)
int16_t tscreen::peek_ch() {
  return Serial.peek();
}
*/
// カーソルの表示/非表示
// flg: カーソル非表示 0、表示 1、強調表示 2
void tscreen::show_curs(uint8_t flg) {
    flgCur = flg;
    if(!flgCur)
      dwaw_cls_curs();
    
}

// カーソルの消去
void tscreen::dwaw_cls_curs() {
  uint8_t c = VPEEK(pos_x,pos_y);
  tv_write(pos_x, pos_y, c?c:32);
}

// 文字色指定
void tscreen::setColor(uint16_t fc, uint16_t bc) {
/*  
  static const uint16_t tbl_fcolor[]  =
     { F_BLACK,F_RED,F_GREEN,F_BROWN,F_BLUE,F_MAGENTA,F_CYAN,A_NORMAL,F_YELLOW};
  static const uint16_t tbl_bcolor[]  =
     { B_BLACK,B_RED,B_GREEN,B_BROWN,B_BLUE,B_MAGENTA,B_CYAN,B_WHITE,B_YELLOW};

  if ( (fc >= 0 && fc <= 8) && (bc >= 0 && bc <= 8) )
     attrset(tbl_fcolor[fc]|tbl_bcolor[bc]);
*/
}

// 文字属性
void tscreen::setAttr(uint16_t attr) {
 /*
  static const uint16_t tbl_attr[]  =
    { A_NORMAL, A_UNDERLINE, A_REVERSE, A_BLINK, A_BOLD };
  
  if (attr >= 0 && attr <= 4)
     attrset(tbl_attr[attr]);
 */    
}

// 指定行の1行分クリア
void tscreen::clerLine(uint16_t l) {
  memset(screen+width*l, 0, width);
  tv_clerLine(l);
  MOVE(pos_y, pos_x);
}

// スクリーンのクリア
void tscreen::cls() {
  tv_cls();
  memset(screen, 0, width*height);
}

// スクリーンリフレッシュ表示
void tscreen::refresh() {
  uint8_t c;
  for (uint16_t i = 0; i < height; i++) {
    for (uint16_t j = 0; j < width; j++) { 
      c = VPEEK(j,i);
      tv_write(j,i,c?c:32);
    }
  }
  MOVE(pos_y, pos_x);
}

// 行のリフレッシュ表示
void tscreen::refresh_line(uint16_t l) {
  uint8_t c;
  for (uint16_t x = 0; x < width; x++) {
    c = VPEEK(x,l);
    tv_write(x,l,c?c:32);
  } 
}

// 1行分スクリーンのスクロールアップ
void tscreen::scroll_up() {
  memmove(screen, screen + width, (height-1)*width);
  dwaw_cls_curs();
  tv_scroll_up();
  clerLine(height-1);
  MOVE(pos_y, pos_x);
}

// 指定行に空白行挿入
void tscreen::Insert_newLine(uint16_t l) {
  if (l < height-1) {
    memmove(screen+(l+2)*width, screen+(l+1)*width, width*(height-1-l-1));
  }
  memset(screen+(l+1)*width, 0, width);
  tv_insLine(l+1);
}

// 現在のカーソル位置の文字削除
void tscreen::delete_char() {
  uint8_t* start_adr = &VPEEK(pos_x,pos_y);
  uint8_t* top = start_adr;
  uint16_t ln = 0;

  if (!*top) // 0文字削除不能
    return;
  
  while( *top ) { ln++; top++; } // 行端,長さ調査
  if ( ln > 1 ) {
    memmove(start_adr, start_adr + 1, ln-1); // 1文字詰める
  }
  *(top-1) = 0; 
  for (uint8_t i=0; i < (pos_x+ln)/width+1; i++)
    refresh_line(pos_y+i);   
  MOVE(pos_y,pos_x);
  return;
}

// 文字の出力
void tscreen::putch(uint8_t c) {
 VPOKE(pos_x, pos_y, c);    // VRAMに文字セット
 if(flgSirialout)
    Serial.write(c);       // シリアル出力
 tv_write(pos_x, pos_y, c); // 画面表示
 movePosNextNewChar();
}

// 現在のカーソル位置に文字を挿入
void tscreen::Insert_char(uint8_t c) {  
  uint8_t* start_adr = &VPEEK(pos_x,pos_y);
  uint8_t* last = start_adr;
  uint16_t ln = 0;  

  // 入力位置の既存文字列長(カーソル位置からの長さ)の参照
  while( *last ) {
    ln++;
    last++;
  }
  if (ln == 0 || flgIns == false) {
     // 文字列長さが0または上書きモードの場合、そのまま1文字表示
    if ( (pos_x + ln >= width-1) && !VPEEK(width-1,pos_y) ) {
       // 画面左端に1文字を書く場合で、次行と連続でない場合は下の行に1行空白を挿入する
       Insert_newLine(pos_y+(pos_x+ln)/width);       
    }
    putch(c);
  } else {
     // 挿入処理が必要の場合  
    if (pos_y + (pos_x+ln+1)/width >= height) {
      // 最終行を超える場合は、挿入前に1行上にスクロールして表示行を確保
      scroll_up();
      start_adr-=width;
      MOVE(pos_y-1, pos_x);
    } else  if ( ((pos_x + ln +1)%width == width-1) && !VPEEK(pos_x + ln , pos_y) ) {
       // 画面左端に1文字を書く場合で、次行と連続でない場合は下の行に1行空白を挿入する
          Insert_newLine(pos_y+(pos_x+ln)/width);
    }
    // 1文字挿入のために1文字分のスペースを確保
    memmove(start_adr+1, start_adr, ln);
    *start_adr=c; // 確保したスペースに1文字表示
    movePosNextNewChar();
    
    // 挿入した行の再表示
    for (uint8_t i=0; i < (pos_x+ln)/width+1; i++)
       refresh_line(pos_y+i);   
    MOVE(pos_y,pos_x);
  }
}

// 改行
void tscreen::newLine() {  
  int16_t x = 0;
  int16_t y = pos_y+1;
 if (y >= height) {
    scroll_up();
    y--;
  }    
  MOVE(y, x);
  if(flgSirialout)
    Serial.println();
}

// カーソルを１文字分次に移動
void tscreen::movePosNextNewChar() {
 int16_t x = pos_x;
 int16_t y = pos_y; 
 x++;
 if (x >= width) {
    x = 0;
    y++;        
   if (y >= height) {
      scroll_up();
      y--;
    }    
 }
 MOVE(y, x);
}

// カーソルを1文字分前に移動
void tscreen::movePosPrevChar() {
  if (pos_x > 0) {
    if ( IS_PRINT(VPEEK(pos_x-1 , pos_y))) {
       MOVE(pos_y, pos_x-1);
    }
  } else {
   if(pos_y > 0) {
      if (IS_PRINT(VPEEK(width-1, pos_y-1))) {
         MOVE(pos_y-1, width - 1);
      } 
   }    
  }
}

// カーソルを1文字分次に移動
void tscreen::movePosNextChar() {
  if (pos_x+1 < width) {
    if ( IS_PRINT( VPEEK(pos_x ,pos_y)) ) {
      MOVE(pos_y, pos_x+1);
    }
  } else {
    if (pos_y+1 < height) {
        if ( IS_PRINT( VPEEK(0, pos_y + 1)) ) {
          MOVE(pos_y+1, 0);
        }
    }
  }
}

// カーソルを次行に移動
void tscreen::movePosNextLineChar() {
  if (pos_y+1 < height) {
    if ( IS_PRINT(VPEEK(pos_x, pos_y + 1)) ) {
      // カーソルを真下に移動
      MOVE(pos_y+1, pos_x);
    } else {
      // カーソルを次行の行末文字に移動
      int16_t x = pos_x;
      while(1) {
        if (IS_PRINT(VPEEK(x, pos_y + 1)) ) 
           break;  
        if (x > 0)
          x--;
        else
          break;
      }      
      MOVE(pos_y+1, x);      
    }
  }
}

// カーソルを前行に移動
void tscreen::movePosPrevLineChar() {
  if (pos_y > 0) {
    if ( IS_PRINT(VPEEK(pos_x, pos_y-1)) ) {
      // カーソルを真上に移動
      MOVE(pos_y-1, pos_x);
    } else {
      // カーソルを前行の行末文字に移動
      int16_t x = pos_x;
      while(1) {
        if (IS_PRINT(VPEEK(x, pos_y - 1)) ) 
           break;  
        if (x > 0)
          x--;
        else
          break;
      }      
      MOVE(pos_y-1, x);      
    }
  }  
}

// カーソルを行末に移動
void tscreen::moveLineEnd() {
  int16_t x = width-1;
  while(1) {
    if (IS_PRINT(VPEEK(x, pos_y)) ) 
       break;  
    if (x > 0)
      x--;
    else
      break;
  }        
  MOVE(pos_y, x);      
}

// スクリーン表示の最終表示の行先頭に移動
void tscreen::moveBottom() {
  int16_t y = height-1;
  while(y) {
    if (IS_PRINT(VPEEK(0, y)) ) 
       break;
    y--;
  }
  MOVE(y,0);
}

// カーソルを指定位置に移動
void tscreen::locate(uint16_t x, uint16_t y) {
  if ( x >= width )  x = width-1;
  if ( y >= height)  y = height;
  MOVE(y, x);
}

// カーソル位置の文字コード取得
uint16_t tscreen::vpeek(uint16_t x, uint16_t y) {
  if (x >= width || y >= height) 
     return 0;
  return VPEEK(x,y);
}

// 行データの入力確定
uint8_t tscreen::enter_text() {
  //memset(text, 0, SC_TEXTNUM);

  // 現在のカーソル位置の行先頭アドレス取得
  uint8_t *ptr = &VPEEK(0, pos_y); 
  if (pos_x == 0 && pos_y)
    ptr--;
    
  // ポインタをさかのぼって、前行からの文字列の連続性を調べる
  // その文字列先頭アドレスをtopにセットする
  uint8_t *top = ptr;
  while (top > screen && *top != 0 )
    top--;
  if ( top != screen ) top++;
 text = top;
  return true;
}

// スクリーン編集
uint8_t tscreen::edit() {
  uint8_t ch;  // 入力文字

  do {
    //MOVE(pos_y, pos_x);
    ch = get_ch ();
    switch(ch) {
      case KEY_CR:         // [Enter]キー
      case KEY_ESCAPE:
        return enter_text();
        break;

      case SC_KEY_CTRL_L:  // [CTRL+L] 画面クリア
        cls();
        locate(0,0);
        break;
 
      case KEY_HOME:      // [HOMEキー] 行先頭移動
        locate(0, pos_y);
        break;
        
      case KEY_NPAGE:     // [PageDown] 表示プログラム最終行に移動
        moveBottom();
        break;
        
      case KEY_PPAGE:     // [PageUP] 画面(0,0)に移動
        locate(0, 0);
        break;

      case SC_KEY_CTRL_R: // [CTRL_R(F5)] 画面更新
        refresh();  break;

      case KEY_END:       // [ENDキー] 行の右端移動
         moveLineEnd();
         break;

      case KEY_IC:         // [Insert]キー
        flgIns = !flgIns;
        break;        

      case KEY_BACKSPACE:  // [BS]キー
        movePosPrevChar();
        delete_char();
        break;        

      case KEY_DC:         // [Del]キー
      case SC_KEY_CTRL_X:
        delete_char();
        break;        
      
      case KEY_RIGHT:      // [→]キー
        movePosNextChar();
        break;

      case KEY_LEFT:       // [←]キー
        movePosPrevChar();
        break;

      case KEY_DOWN:       // [↓]キー
        movePosNextLineChar();
        break;
      
      case KEY_UP:         // [↑]キー
        movePosPrevLineChar();
        break;

      case SC_KEY_CTRL_N:  // 行挿入 
        Insert_newLine(pos_y);       
        break;

      case SC_KEY_CTRL_D:  // 行削除
        clerLine(pos_y);
        break;
      
      default:             // その他
      
      if (IS_PRINT(ch)) {
        Insert_char(ch);
      }   
      break;
    }
  } while(1);
}

// ドット描画
void tscreen::pset(int16_t x, int16_t y, uint8_t c) {
  tv_pset(x,y,c);
}

// 線の描画
void tscreen::line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t c) {
 tv_line(x1,y1,x2,y2,c);
}

// 円の描画
void tscreen::circle(int16_t x, int16_t y, int16_t r, uint8_t c, int8_t f) {
  tv_circle(x, y, r, c, f);
}

// 四角の描画
void tscreen::rect(int16_t x, int16_t y, int16_t h, int16_t w, uint8_t c, int8_t f) {
  tv_rect(x, y, h, w, c, f);
}

void tscreen::tone(int16_t freq, int16_t tm) {
  tv_tone(freq, tm);  
}

void tscreen::notone() {
  tv_notone();    
}

