//
// file:tscreen.h
// ターミナルスクリーン制御ライブラリ for Arduino STM32
//  V1.0 作成日 2017/03/22 by たま吉さん
//  修正日 2017/03/26, 色制御関連関数の追加
//

#include <string.h>
#include "tscreen.h"

#define VPEEK(X,Y)      (screen[width*(Y)+(X)])
#define VPOKE(X,Y,C)    (screen[width*(Y)+(X)]=C)

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

// スクリーンの初期設定
// 引数
//  w  : スクリーン横文字数
//  h  : スクリーン縦文字数
//  l  : 1行の最大長
// 戻り値
//  なし
void tscreen::init(uint16_t w, uint16_t h, uint16_t l) {
    
  width   = w;
  height  = h;
  maxllen = l;
  pos_x = 0;
  pos_y = 0;

  if (screen != NULL) 
    free(screen);
  screen = (uint8_t*)malloc( width * height );

  cls();
  
  // mcursesの設定
  setFunction_putchar(Arduino_putchar); 
  setFunction_getchar(Arduino_getchar); 
  initscr();
  clear();
  setscrreg (SC_FIRST_LINE, SC_LAST_LINE);
  move(pos_y, pos_x);

  // 編集機能の設定
  flgIns = true;
}

// キー入力チェック
uint8_t tscreen::isKeyIn() {
  return Serial.available();
}

// 文字入力
uint8_t tscreen::get_ch() {
  //move(pos_y, pos_x);
  return getch ();
}

// キー入力チェック(文字参照)
int16_t tscreen::peek_ch() {
  return Serial.peek();
}

// カーソルの表示/非表示
// flg: カーソル非表示 0、表示 1、強調表示 2
void tscreen::show_curs(uint8_t flg) {
    curs_set(flg);  
}

// 文字色指定
void tscreen::setColor(uint16_t fc, uint16_t bc) {
  static const uint16_t tbl_fcolor[]  =
     { F_BLACK,F_RED,F_GREEN,F_BROWN,F_BLUE,F_MAGENTA,F_CYAN,A_NORMAL,F_YELLOW};
  static const uint16_t tbl_bcolor[]  =
     { B_BLACK,B_RED,B_GREEN,B_BROWN,B_BLUE,B_MAGENTA,B_CYAN,B_WHITE,B_YELLOW};

  if ( (fc >= 0 && fc <= 8) && (bc >= 0 && bc <= 8) )
     attrset(tbl_fcolor[fc]|tbl_bcolor[bc]);
}

// 文字属性
void tscreen::setAttr(uint16_t attr) {
  static const uint16_t tbl_attr[]  =
    { A_NORMAL, A_UNDERLINE, A_REVERSE, A_BLINK, A_BOLD };
  
  if (attr >= 0 && attr <= 4)
     attrset(tbl_attr[attr]);
}

// 指定行の1行分クリア
void tscreen::clerLine(uint16_t l) {
  memset(screen+width*l, 0, width);
  move(l,0);
  clrtoeol();
  move(pos_y, pos_x);
}

// スクリーンのクリア
void tscreen::cls() {
  clear(); 
  memset(screen, 0, width*height);
}

// スクリーンリフレッシュ表示
void tscreen::refresh() {
  clear(); 
  for (uint16_t i = 0; i < height; i++) {
    for (uint16_t j = 0; j < width; j++) {
        if( IS_PRINT( VPEEK(j,i) )) { 
        move(i,j);
        addch( VPEEK(j,i) ); 
      }
    }
  }
  move(pos_y, pos_x);
}

// 1行分スクリーンのスクロールアップ
void tscreen::scroll_up() {
  memmove(screen, screen + width, (height-1)*width);
  scroll();
  clerLine(height-1);
}

// 現在のカーソル位置の文字削除
void tscreen::delete_char() {
  uint8_t* start_adr = &VPEEK(pos_x,pos_y);
  uint8_t* top = start_adr;
  uint16_t ln = 0;

  if (!*top) 
    return;
  
  while( *top ) {
    ln++;
    top++;
  }
  if ( ln > 1 )
    memmove(start_adr, start_adr + 1, ln-1);
  *(top-1) = 0;
  refresh();
  //delch();
  return;
}

// 文字の出力
void tscreen::putch(uint8_t c) {
 VPOKE(pos_x, pos_y, c);
 move(pos_y, pos_x);
 addch(c);
 movePosNextNewChar();
}

// 現在のカーソル位置に文字を挿入
void tscreen::Insert_char(uint8_t c) {  
  uint8_t* start_adr = &VPEEK(pos_x,pos_y);
  uint8_t* last = start_adr;
  uint16_t ln = 0;  

  // 入力位置の既存文字列長の参照
  while( *last ) {
    ln++;
    last++;
  }

  if (ln == 0 || flgIns == false) {
     // 文字列長さが0または上書きモードの場合
     putch(c);
  } else {
     // 挿入処理が必要の場合  
    memmove(start_adr+1, start_adr, ln);
    *start_adr=c;
    insch(c);
    movePosNextNewChar();
    refresh();
  }
}

// 改行
void tscreen::newLine() {
  pos_x = 0;
  pos_y++;
 if (pos_y >= height) {
    scroll_up();
    pos_y--;
  }    
  move(pos_y, pos_x);  
}

// カーソルを１文字分次に移動
void tscreen::movePosNextNewChar() {
 pos_x++;
 if (pos_x >= width) {
    pos_x = 0;
    pos_y++;
   if (pos_y >= height) {
      scroll_up();
      pos_y--;
    }    
    move(pos_y, pos_x);
 }
}

// カーソルを1文字分前に移動
void tscreen::movePosPrevChar() {
  if (pos_x > 0) {
    if ( IS_PRINT(VPEEK(pos_x-1 , pos_y))) {
       pos_x--;
       move(pos_y, pos_x);
    }
  } else {
   if(pos_y > 0) {
      if (IS_PRINT(VPEEK(width-1, pos_y-1))) {
         pos_x = width - 1;
         pos_y--;
         move(pos_y, pos_x);
      } 
   }    
  }
}

// カーソルを1文字分次に移動
void tscreen::movePosNextChar() {
  if (pos_x+1 < width) {
    if ( IS_PRINT( VPEEK(pos_x ,pos_y)) ) {
      pos_x++;
      move(pos_y, pos_x);
    }
  } else {
    if (pos_y+1 < height) {
        if ( IS_PRINT( VPEEK(0, pos_y + 1)) ) {
          pos_x = 0;
          pos_y++;
        }
        move(pos_y, pos_x);
    }
  }
}

// カーソルを次行に移動
void tscreen::movePosNextLineChar() {
  if (pos_y+1 < height) {
    if ( IS_PRINT(VPEEK(pos_x, pos_y + 1)) ) {
      // カーソルを真下に移動
      pos_y++;
      move(pos_y, pos_x);
    } else {
      // カーソルを次行の行末文字に移動
      while(1) {
        if (IS_PRINT(VPEEK(pos_x, pos_y + 1)) ) 
           break;  
        if (pos_x > 0)
          pos_x--;
        else
          break;
      }      
      pos_y++;
      move(pos_y, pos_x);      
    }
  }
}

// カーソルを前行に移動
void tscreen::movePosPrevLineChar() {
  if (pos_y > 0) {
    if ( IS_PRINT(VPEEK(pos_x, pos_y-1)) ) {
      // カーソルを真上に移動
      pos_y--;
      move(pos_y, pos_x);
    } else {
      // カーソルを前行の行末文字に移動
      while(1) {
        if (IS_PRINT(VPEEK(pos_x, pos_y - 1)) ) 
           break;  
        if (pos_x > 0)
          pos_x--;
        else
          break;
      }      
      pos_y--;
      move(pos_y, pos_x);      
    }
  }  
}

// カーソルを指定位置に移動
void tscreen::locate(uint16_t x, uint16_t y) {
  if (x >= width)
    x = width-1;
  if (y >= height)
    y = height;

  pos_x = x;
  pos_y = y;
  move(y, x);
}

// カーソル位置の文字コード取得
uint16_t tscreen::vpeek(uint16_t x, uint16_t y) {
  if (x >= width || y >= height) 
     return 0;
  return VPEEK(x,y);
}

// 行データの入力確定
uint8_t tscreen::enter_text() {
  memset(text, 0, SC_TEXTNUM);

  // 現在のカーソル位置の行先頭アドレス取得
  uint8_t *ptr = &VPEEK(0, pos_y); 

  // ポインタをさかのぼって、前行からの文字列の連続性を調べる
  // その文字列先頭アドレスをtopにセットする
  uint8_t *top = ptr;
  while (top > screen && *top != 0 )
    top--;
  if ( top != screen ) top++;
    
  // 文字列長の有効性のチェック  
  if (strlen((char*)top) >= maxllen) {
     return false;  // 文字列オーバー
  }

  // 確定した文字列を一時バッファに保持する
  strcpy((char*)text, (char*)top);
  return true;
}

// スクリーン編集
uint8_t tscreen::edit() {
  uint8_t ch;  // 入力文字

  do {
    move(pos_y, pos_x);
    ch = getch ();
    switch(ch) {
      case KEY_CR:         // [Enter]キー
      case KEY_ESCAPE:
        return enter_text();
        break;

      case SC_KEY_CTRL_L:  // [CTRL+L] 画面クリア
        cls();
        locate(0,0);
        break;
 
      case KEY_HOME:      // [HOMEキー] 画面再表示
      case SC_KEY_CTRL_R:
        refresh();  break;

      case KEY_IC:         // [Insert]キー
        flgIns = !flgIns;
        if (flgIns) curs_set(2);
        else  curs_set(1);
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

      default:             // その他
      
      if (IS_PRINT(ch)) {
        Insert_char(ch);
      }
/*
      else {
          move(23,0);
          clrtoeol();
          addstr("[KEY]");
          Serial.print(ch, DEC); 
          move(c_y(), c_x());
      }
*/    
      break;
    }
  } while(1);
}
