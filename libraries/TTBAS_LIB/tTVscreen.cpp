//
// file:tTVscreen.h
// ターミナルスクリーン制御ライブラリ for Arduino STM32
//  V1.0 作成日 2017/03/22 by たま吉さん
//  修正日 2017/03/26, 色制御関連関数の追加
//  修正日 2017/03/30, moveLineEnd()の追加,[HOME],[END]の編集キーの仕様変更
//  修正日 2017/04/09, PS/2キーボードとの併用可能
//  修正日 2017/04/11, TVout版に移行
//  修正日 2017/04/15, 行挿入の追加(次行上書き防止対策）
//  修正日 2017/04/17, bitmaph表示処理の追加
//  修正日 2017/04/18, シリアルポート設定機能の追加
//  修正日 2017/04/19, 行確定時の不具合対応
//  修正日 2017/04/18, シリアルポート設定機能の追加,cscroll,gscroll表示処理の追加
//  修正日 2017/04/24, cscroll()関数 キャラクタ下スクロール不具合修正
//  修正日 2017/04/25, gscrollの機能修正, gpeek,ginpの追加
//  修正日 2017/04/29, キーボード、NTSCの補正対応
//  修正日 2017/05/10, システムクロック48MHz時のtv_get_gwidth()不具合対応
//  修正日 2017/05/28, 上下スクロール編集対応
//  修正日 2017/06/09, シリアルからは全てもコードを通すように修正
//  修正日 2017/06/18, Insert_char()のメモリ領域外アクセスの不具合対応
//  修正日 2017/06/22, シリアル上でBSキー、CTRL-Lが利用可能対応
//

#include "../../ttbasic/ttconfig.h"

#include <string.h>
#include "tTVscreen.h"

//#define DEBUG

void    endPS2();

void setupPS2(uint8_t kb_type);

#define VPEEK(X,Y)      (screen[width*(Y)+(X)])
#define VPOKE(X,Y,C)    (screen[width*(Y)+(X)]=C)

//****************************************************************************
// 差分実装
//***************************************************************************

// カーソルの移動
// ※pos_x,pos_yは本関数のみでのみ変更可能
void tTVscreen::MOVE(uint8_t y, uint8_t x) {
  uint8_t c;
  if (enableCursor && flgCur) {
    c = VPEEK(pos_x,pos_y);
    tv_write(pos_x, pos_y, c?c:32);  
    tv_drawCurs(x, y);
  } 
  m_cursor_count = 0;
  m_cursor_state = false;
  pos_x = x;
  pos_y = y;
}

// 文字の表示
void tTVscreen::WRITE(uint8_t x, uint8_t y, uint8_t c) {
   tv_write(x, y, c); // 画面表示
}

// 画面全消去
void tTVscreen::CLEAR() {
  tv_cls();      
}

// 行の消去
void tTVscreen::CLEAR_LINE(uint8_t l) {
  tv_clerLine(l);  
}

// スクロールアップ
void tTVscreen::SCROLL_UP() {
  tv_scroll_up();  
}

// スクロールダウン
void tTVscreen::SCROLL_DOWN() {
  tv_scroll_down(); 
}

// 指定行に1行挿入(下スクロール)
void tTVscreen::INSLINE(uint8_t l) {
  tv_insLine(l);               
}

//****************************************************************************


// スクリーンの初期設定
// 引数
//  w  : スクリーン横文字数
//  h  : スクリーン縦文字数
//  l  : 1行の最大長
// 戻り値
//  なし
void tTVscreen::init(uint16_t ln, uint8_t kbd_type, int16_t NTSCajst, uint8_t* extmem, uint8_t vmode) {
  
  // ビデオ出力設定
  tv_init(NTSCajst, extmem, vmode);
  if (extmem == NULL) {
    tscreenBase::init(tv_get_cwidth(),tv_get_cheight(), ln);
  } else {
    tscreenBase::init(tv_get_cwidth(),tv_get_cheight(), ln, extmem
#if USE_VS23 == 0
    + getGRAMsize()
#endif
    );
  }	
  tGraphicDev::init();

#if PS2DEV == 1
  setupPS2(kbd_type);
#endif
  m_cursor_count = 0;
  m_cursor_state = false;
}

// スクリーンの利用の終了
void tTVscreen::end() {
  // キーボード利用の終了
  endPS2();

  // ビデオ出力終了
  tv_end();
  tscreenBase::end();
}

void tTVscreen::reset_kbd(uint8_t kbd_type) {
  endPS2();
  setupPS2(kbd_type);
}

// 文字の出力
void tTVscreen::putch(uint8_t c) {
  tscreenBase::putch(c);
#ifdef DEBUG
  Serial.write(c);       // シリアル出力
#endif
}

// 改行
void tTVscreen::newLine() {  
  tscreenBase::newLine();
#ifdef DEBUG
  Serial.write(0x0d);
  Serial.write(0x0a);
#endif
}

// 行のリフレッシュ表示
void tTVscreen::refresh_line(uint16_t l) {
  CLEAR_LINE(l);
  for (uint16_t j = 0; j < width; j++) {
    if( IS_PRINT( VPEEK(j,l) )) { 
      WRITE(j,l,VPEEK(j,l));
    }
  }
}

	
// キー入力チェック&キーの取得
bool ICACHE_RAM_ATTR tTVscreen::isKeyIn() {
#ifdef DEBUG
  return Serial.available();
#endif
#if PS2DEV == 1
 return ps2kbhit();
#endif
}

void pump_events(void);
// 文字入力
uint8_t tTVscreen::get_ch() {
  char c;
  while(1) {
    pump_events();
#ifdef DEBUG
    if (Serial.available()) {
      c = Serial.read();
      dev = 1;
      break;
    }
#endif
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

// カーソルの表示/非表示
// flg: カーソル非表示 0、表示 1、強調表示 2
void tTVscreen::drawCursor(uint8_t flg) {
    flgCur = flg;

    if(!flgCur)
      draw_cls_curs();
    else if (enableCursor)
      tv_drawCurs(pos_x, pos_y);  
}

void ICACHE_RAM_ATTR tTVscreen::updateCursor()
{
  if (!m_cursor_count) {
    m_cursor_state = !m_cursor_state;
    if (enableCursor)
      drawCursor(m_cursor_state);
    m_cursor_count = 30;
  } else
    --m_cursor_count;
}

void tTVscreen::show_curs(uint8_t flg) {
  enableCursor = flg;
  drawCursor(flg);
}

// カーソルの消去
void tTVscreen::draw_cls_curs() {
  uint8_t c = VPEEK(pos_x,pos_y);
  tv_write(pos_x, pos_y, c?c:32);
}
	
/*
// 文字色指定
void tTVscreen::setColor(uint16_t fc, uint16_t bc) {
  static const uint16_t tbl_fcolor[]  =
     { F_BLACK,F_RED,F_GREEN,F_BROWN,F_BLUE,F_MAGENTA,F_CYAN,A_NORMAL,F_YELLOW};
  static const uint16_t tbl_bcolor[]  =
     { B_BLACK,B_RED,B_GREEN,B_BROWN,B_BLUE,B_MAGENTA,B_CYAN,B_WHITE,B_YELLOW};

  if ( (fc >= 0 && fc <= 8) && (bc >= 0 && bc <= 8) )
     attrset(tbl_fcolor[fc]|tbl_bcolor[bc]);

}

// 文字属性
void tTVscreen::setAttr(uint16_t attr) {
  static const uint16_t tbl_attr[]  =
    { A_NORMAL, A_UNDERLINE, A_REVERSE, A_BLINK, A_BOLD };
  
  if (attr >= 0 && attr <= 4)
     attrset(tbl_attr[attr]);
}
*/    

void tv_setcolor(uint16_t fc, uint16_t bc);

void tTVscreen::setColor(uint16_t fc, uint16_t bc)
{
  tv_setcolor(fc, bc);
}

// スクリーン編集
uint8_t tTVscreen::edit() {
  uint8_t ch;  // 入力文字
  keyEvent k;

  do {
    //MOVE(pos_y, pos_x);
    ch = get_ch ();
    k = ps2last();
    if (k.ALT) {
      if (ch >= 'A' && ch <= '_')
        Insert_char(ch - 64);
      else if (ch >= '`' && ch <= '~')
        Insert_char(ch + 32);
    } else switch(ch) {
      case KEY_CR:         // [Enter]キー
        return enter_text();
      case KEY_SHIFT_CR:
        ch = enter_text();
        Insert_newLine(pos_y);
        return ch;

      case SC_KEY_CTRL_L:  // [CTRL+L] 画面クリア
      case KEY_F(1):       // F1
        cls();
        locate(0,0);
        Serial_Ctrl(SC_KEY_CTRL_L);
        break;
 
      case KEY_HOME:      // [HOMEキー] 行先頭移動
        locate(0, pos_y);
        break;
        
      case KEY_NPAGE:     // [PageDown] 表示プログラム最終行に移動
        if (pos_x == 0 && pos_y == height-1) {
          edit_scrollUp();
        } else {
          moveBottom();
        }
        break;
        
      case KEY_PPAGE:     // [PageUP] 画面(0,0)に移動
        if (pos_x == 0 && pos_y == 0) {
          edit_scrollDown();
        } else {
          locate(0, 0);
        }  
        break;

      case SC_KEY_CTRL_R: // [CTRL_R(F5)] 画面更新
      case KEY_F(5):       // F5
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
         Serial_Ctrl(KEY_BACKSPACE);
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
      case KEY_SHIFT_DOWN:
        movePosNextLineChar(true);
        break;
      
      case KEY_UP:         // [↑]キー
        movePosPrevLineChar();
        break;
      case KEY_SHIFT_UP:
        movePosPrevLineChar(true);
        break;

      case SC_KEY_CTRL_N:  // 行挿入 
      case KEY_F(3):       // F3
        Insert_newLine(pos_y);       
        break;

      case SC_KEY_CTRL_D:  // 行削除
      case KEY_F(2):       // F2
        clerLine(pos_y);
        break;

      case SC_KEY_CTRL_C:
        break;
      
      default:             // その他
        Insert_char(ch);
        break;
    }
  } while(1);
}

// シリアルポートスクリーン制御出力
void tTVscreen::Serial_Ctrl(int16_t ch) {
#ifdef DEBUG
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
    // Serial.print(s);     // USBシリアル出力
    while(*s) {
      Serial.write(*s);
      s++;
    }  
  }
#endif
}


// キャラクタ画面スクロール
void tTVscreen::cscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t d) {
  switch(d) {
    case 0: // 上
      for (uint16_t i= 0; i < h-1; i++) {
        memcpy(&VPEEK(x,y+i), &VPEEK(x,y+i+1), w);
      }
      memset(&VPEEK(x, y + h - 1), 0, w);
      break;            

    case 1: // 下
      for (uint16_t i= 0; i < h-1; i++) {
        memcpy(&VPEEK(x,y + h-1-i), &VPEEK(x,y+h-1-i-1), w);
      }
      memset(&VPEEK(x, y), 0, w);
      break;            

    case 2: // 右
      for (uint16_t i=0; i < h; i++) {
        memmove(&VPEEK(x+1, y+i) ,&VPEEK(x,y+i), w-1);
        VPOKE(x,y+i,0);
      }
      break;
      
    case 3: // 左
      for (uint16_t i=0; i < h; i++) {
        memmove(&VPEEK(x,y+i) ,&VPEEK(x+1,y+i), w-1);
        VPOKE(x+w-1,y+i,0);
      }
      break;
  }
  uint8_t c;
  for (uint8_t i = 0; i < h; i++) 
    for (uint8_t j=0; j < w; j++) {
      c = VPEEK(x+j,y+i);
      tv_write(x+j,y+i, c?c:32);
    }
}

void tTVscreen::tone(int16_t freq, int16_t tm) {
  tv_tone(freq, tm);  
}

void tTVscreen::notone() {
  tv_notone();    
}

void tTVscreen::adjustNTSC(int16_t ajst) {
  tv_NTSC_adjust(ajst);
}

void tTVscreen::setFont(const uint8_t *font)
{
  tv_setFont(font);
  width = tv_get_win_cwidth();
  height = tv_get_win_cheight();
  if (pos_x >= width)
    pos_x = width-1;
  if (pos_y >= height)
    pos_y = height-1;
}
