//
// PS/2キー入力をtty入力コードにコンバートする
// 作成 2017/04/09
// 修正 2017/04/15, 利用ピンの変更(I2Cとの競合のため)

#include <TKeyboard.h>
#include "tscreen.h"
/*
const int IRQpin =  PB7;  // CLK(D+)
const int DataPin = PB8;  // Data(D-) 
*/
const int IRQpin =  PB4;  // CLK(D+)
const int DataPin = PB5;  // Data(D-) 

TKeyboard kb;

// PS/2キーボードのセットアップ
void setupPS2() {
  // PS/2 キーボードの利用開始
  // begin()の引数： CLKピンNo、DATAピンNo、LED制御あり/なし、USキーボードを利用する/しない
  if ( kb.begin(IRQpin, DataPin, true, false) ) {
    // 初期化に失敗時はエラーメッセージ表示
    //Serial.println("PS/2 Keyboard initialize error.");
  }  
}

uint8_t cnv2tty(keyEvent k) {
  int16_t rc = 0;
  if (!k.code || k.BREAK)
      return 0;

  // CTRLとの併用
  if (k.CTRL) {
    //Serial.println(k.code,DEC);
    if(!k.KEY) {
      if (k.code >= 'a' && k.code <= 'z') {
        rc = k.code - 'a' + 1;
      } else if (k.code >= 'A' && k.code <= 'Z') {
        rc = k.code - 'A' + 1;
      } else {
        switch(k.code) {
          case KEY_AT:         rc = 0;    break;
          case KEY_L_brackets: rc = 0x1b; break;
          case KEY_Pipe2:      rc = 0x1c; break;
          case KEY_R_brackets: rc = 0x1d; break;  
          case KEY_Hat:        rc = 0x1e; break;  
          case KEY_Ro:         rc = 0x1f; break;  
        }
      }
      return rc;
    }
  }
  
  // 通常のASCIIコード
  if(!k.KEY) {
    rc = k.code;
    return rc;
  }
  
  // 特殊キーの場合
  switch(k.code) {
    case KEY_Insert:     rc = KEY_IC;       break;
    case KEY_Home:       rc = KEY_HOME;     break;
    case KEY_PageUp:     rc = KEY_PPAGE;    break;
    case KEY_PageDown:   rc = KEY_NPAGE;    break;
    case KEY_End:        rc = KEY_END;      break;
    case KEY_L_Arrow:    rc = KEY_LEFT;     break;
    case KEY_Up_Arrow:   rc = KEY_UP;       break;
    case KEY_R_Arrow:    rc = KEY_RIGHT;    break;
    case KEY_Down_Arrow: rc = KEY_DOWN;     break;
    case KEY_ESC:        rc = KEY_ESCAPE;   break;
    case KEY_Tab:        rc = KEY_TAB;      break;
    case KEY_Space:      rc = 32;           break;
    case KEY_Backspace:  rc = KEY_BACKSPACE;break;
    case KEY_Delete:     rc = KEY_DC;       break;
    case KEY_Enter:      rc = KEY_CR;       break;
    case 112:            rc = SC_KEY_CTRL_L;break;
    case KEY_F5:         rc = SC_KEY_CTRL_R;break;
  }
  return rc;
}


uint8_t ps2read() {
  keyEvent k; // キー入力情報
  k = kb.read(); 
  return cnv2tty(k);
}

