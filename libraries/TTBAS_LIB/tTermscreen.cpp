//
// file:tTermscreen.cpp
// ターミナルスクリーン制御ライブラリ for Arduino STM32
//  V1.0 作成日 2017/03/22 by たま吉さん
//  修正日 2017/03/26, 色制御関連関数の追加
//  修正日 2017/03/30, moveLineEnd()の追加,[HOME],[END]の編集キーの仕様変更
//  修正日 2017/06/27, 汎用化のための修正
//

#include <string.h>
#include "tTermscreen.h"

//******* mcurses用フック関数の定義(開始)  *****************************************
static tTermscreen* tsc = NULL;

// シリアル経由1文字出力

static void Arduino_putchar(uint8_t c) {
  if (tsc->getSerialMode() == 0)
     Serial.write(c);
  else if (tsc->getSerialMode() == 1)
     Serial1.write(c);   
}

// シリアル経由1文字入力
static char Arduino_getchar() {
  if (tsc->getSerialMode() == 0) {
    while (!Serial.available());
    return Serial.read();
  } else if (tsc->getSerialMode() == 1) {
    while (!Serial1.available());
    return Serial1.read();    
  } else {
    while (!Serial.available());
    return Serial.read();    
  }
}
//******* mcurses用フック関数の定義(終了)  *****************************************

//****** シリアルターミナルデバイス依存のメンバー関数のオーバーライド定義(開始) ****

// カーソルの移動
// pos_x,pos_yは本関数のみでのみ変更可能
// カーソルの表示も行う
void tTermscreen::MOVE(uint8_t y, uint8_t x) {
  ::move(y,x);
  pos_x = x;
  pos_y = y;
};

// 文字の表示
void tTermscreen::WRITE(uint8_t x, uint8_t y, uint8_t c) {
  ::move(y,x);
  ::addch(c);
  ::move(pos_y, pos_x);
}
    
void tTermscreen::CLEAR() {
  ::clear();
}

// 行の消去
void tTermscreen::CLEAR_LINE(uint8_t l) {
  ::move(l,0);  ::clrtoeol();  // 依存関数  
}

// スクロールアップ
void tTermscreen::SCROLL_UP() {
  ::scroll();
}

// スクロールダウン
void tTermscreen::SCROLL_DOWN() {
  INSLINE(0);
}

// 指定行に1行挿入(下スクロール)
void tTermscreen::INSLINE(uint8_t l) {
  ::move(l,0);
  ::insertln();
  ::move(pos_y,pos_x);
}

// 依存デバイスの初期化
// シリアルコンソール mcursesの設定
void tTermscreen::INIT_DEV() {
  // mcursesの設定
  ::setFunction_putchar(Arduino_putchar);  // 依存関数
  ::setFunction_getchar(Arduino_getchar);  // 依存関数
  ::initscr();                             // 依存関数
  ::setscrreg(0,height-1);
  serialMode = 0;
  tsc = this;
}

// キー入力チェック
uint8_t tTermscreen::isKeyIn() {
 if(serialMode == 0) {
	if (Serial.available())
       return get_ch();
    else
       return 0;
  } else if (serialMode == 1) {
	if (Serial1.available())
       return get_ch();
    else
       return 0;
	}
}

// 文字入力
uint8_t tTermscreen::get_ch() {
  uint8_t c = getch();
  switch (c) {
    case KEY_F(1):c=SC_KEY_CTRL_L; break; 
    case KEY_F(2):c=SC_KEY_CTRL_D; break;
    case KEY_F(3):c=SC_KEY_CTRL_N; break;
    case KEY_F(5):c=SC_KEY_CTRL_R; break;
  }
  return c;
}

// キー入力チェック(文字参照)
int16_t tTermscreen::peek_ch() {
 if(serialMode == 0)
    return Serial.peek();
 if(serialMode == 1)
    return Serial1.peek();
}

// カーソルの表示/非表示
// flg: カーソル非表示 0、表示 1、強調表示 2
void tTermscreen::show_curs(uint8_t flg) {
    flgCur = flg;
    ::curs_set(flg);  // 依存関数
}

// カーソルの消去
void tTermscreen::draw_cls_curs() {  

}

// 文字色指定
void tTermscreen::setColor(uint16_t fc, uint16_t bc) {
  static const uint16_t tbl_fcolor[]  =
     { F_BLACK,F_RED,F_GREEN,F_BROWN,F_BLUE,F_MAGENTA,F_CYAN,A_NORMAL,F_YELLOW};
  static const uint16_t tbl_bcolor[]  =
     { B_BLACK,B_RED,B_GREEN,B_BROWN,B_BLUE,B_MAGENTA,B_CYAN,B_WHITE,B_YELLOW};

  if ( (fc >= 0 && fc <= 8) && (bc >= 0 && bc <= 8) )
     attrset(tbl_fcolor[fc]|tbl_bcolor[bc]);  // 依存関数
}

// 文字属性
void tTermscreen::setAttr(uint16_t attr) {
  static const uint16_t tbl_attr[]  =
    { A_NORMAL, A_UNDERLINE, A_REVERSE, A_BLINK, A_BOLD };
  
  if (attr >= 0 && attr <= 4)
     attrset(tbl_attr[attr]);  // 依存関数
}


//****** シリアルターミナルデバイス依存のメンバー関数のオーバーライド定義(終了) ****
