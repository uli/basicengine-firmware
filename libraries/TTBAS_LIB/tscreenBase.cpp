// 
// スクリーン制御基本クラス ヘッダーファイル
// 作成日 2017/06/27 by たま吉さん
// 修正日 2017/08/05 ファンクションキーをNTSC版同様に利用可能対応
// 修正日 2017/08/12 edit_scrollUp() で最終行が2行以上の場合の処理ミス修正

#include "tscreenBase.h"

#if DEPEND_TTBASIC == 1
  int32_t getPrevLineNo(int32_t lineno);
  int32_t getNextLineNo(int32_t lineno);
  char* getLineStr(int32_t lineno);
#endif

// スクリーンの初期設定
// 引数
//  w      : スクリーン横文字数
//  h      : スクリーン縦文字数
//  l      : 1行の最大長
//  extmem : 外部獲得メモリアドレス NULL:なし NULL以外 あり
// 戻り値
//  なし
void tscreenBase::init(uint16_t w, uint16_t h, uint16_t l,uint8_t* extmem) {
  width   = w;
  height  = h;
  maxllen = l;

  // デバイスの初期化
  INIT_DEV();

  // 直前の獲得メモリの開放
  if (!flgExtMem) {
  	if (screen != NULL) {
      free(screen);
    }
  }

  // スクリーン用バッファ領域の設定
  if (extmem == NULL) {
    flgExtMem = 0;
    screen = (uint8_t*)malloc( width * height );
  } else {
     flgExtMem = 1;
  	 screen = extmem;
  }
  
  cls();
  show_curs(true);  
  MOVE(pos_y, pos_x);

  // 編集機能の設定
  flgIns = true;
}

// スクリーン利用終了
void tscreenBase::end() {
  
  // デバイスの終了処理
  END_DEV();

  // 動的確保したメモリーの開放
  if (!flgExtMem) {
    if (screen != NULL) {
      free(screen);
      screen = NULL;
    }
  }
}

// 指定行の1行分クリア
void tscreenBase::clerLine(uint16_t l) {
  memset(screen+width*l, 0, width);
  CLEAR_LINE(l);
  MOVE(pos_y, pos_x);
}

// スクリーンのクリア
void tscreenBase::cls() {
  CLEAR();
  memset(screen, 0, width*height);
}

// スクリーンリフレッシュ表示
void tscreenBase::refresh() {
  for (uint16_t i = 0; i < height; i++)
    refresh_line(i);
  MOVE(pos_y, pos_x);
}

// 行のリフレッシュ表示
void tscreenBase::refresh_line(uint16_t l) {
  CLEAR_LINE(l);
  for (uint16_t j = 0; j < width; j++) {
    if( IS_PRINT( VPEEK(j,l) )) { 
      WRITE(j,l,VPEEK(j,l));
    }
  }
}

// 1行分スクリーンのスクロールアップ
void tscreenBase::scroll_up() {
  memmove(screen, screen + width, (height-1)*width);
  draw_cls_curs();
  SCROLL_UP();
  clerLine(height-1);
  MOVE(pos_y, pos_x);
}

// 1行分スクリーンのスクロールダウン
void tscreenBase::scroll_down() {
  memmove(screen + width, screen, (height-1)*width);
  draw_cls_curs();
  SCROLL_DOWN();
  clerLine(0);
  MOVE(pos_y, pos_x);
}

// 指定行に空白行挿入
void tscreenBase::Insert_newLine(uint16_t l) {
  if (l < height-1) {
    memmove(screen+(l+2)*width, screen+(l+1)*width, width*(height-1-l-1));
  }
  memset(screen+(l+1)*width, 0, width);
  INSLINE(l+1);
}

// 現在のカーソル位置の文字削除
void tscreenBase::delete_char() {
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
void tscreenBase::putch(uint8_t c) {
 VPOKE(pos_x, pos_y, c);
 if (!flgCur)
   WRITE(pos_x, pos_y, c);
 movePosNextNewChar();
}


// 現在のカーソル位置に文字を挿入
void tscreenBase::Insert_char(uint8_t c) {  
/*
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
    movePosNextNewChar();
    // 挿入した行の再表示
    for (uint8_t i=0; i < (pos_x+ln)/width+1; i++)
       refresh_line(pos_y+i);   
    MOVE(pos_y,pos_x);    
  }
*/
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
    if (pos_y + (pos_x+ln+1)/width >= height) {
      // 最終行を超える場合は、挿入前に1行上にスクロールして表示行を確保
      scroll_up();
      start_adr-=width;
      MOVE(pos_y-1, pos_x);
    } else  if ( (pos_x + ln >= width-1) && !VPEEK(width-1,pos_y) ) {
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
void tscreenBase::newLine() {
  int16_t x = 0;
  int16_t y = pos_y+1;
  if (y >= height) {
     scroll_up();
     y--;
   }    
   MOVE(y, x);
}

// カーソルを１文字分次に移動
void tscreenBase::movePosNextNewChar() {
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
void tscreenBase::movePosPrevChar() {
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
void tscreenBase::movePosNextChar() {
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
void tscreenBase::movePosNextLineChar() {
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
  } else if (pos_y+1 == height) {
    edit_scrollUp();    
  }
}

// カーソルを前行に移動
void tscreenBase::movePosPrevLineChar() {
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
  } else if (pos_y == 0){
    edit_scrollDown();
  }
}

// カーソルを行末に移動
void tscreenBase::moveLineEnd() {
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
void tscreenBase::moveBottom() {
  int16_t y = height-1;
  while(y) {
    if (IS_PRINT(VPEEK(0, y)) ) 
       break;
    y--;
  }
  MOVE(y,0);
}

// カーソルを指定位置に移動
void tscreenBase::locate(uint16_t x, uint16_t y) {
  if ( x >= width )  x = width-1;
  if ( y >= height)  y = height;
  MOVE(y, x);
}

// カーソル位置の文字コード取得
uint16_t tscreenBase::vpeek(uint16_t x, uint16_t y) {
  if (x >= width || y >= height) 
     return 0;
  return VPEEK(x,y);
}

// 行データの入力確定
uint8_t tscreenBase::enter_text() {

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

// 指定行の行番号の取得
int16_t tscreenBase::getLineNum(int16_t l) {
  uint8_t* ptr = screen+width*l;
  uint32_t n = 0;
  int rc = -1;  
  while (isDigit(*ptr)) {
    n *= 10;
    n+= *ptr-'0';
    if (n>32767) {
      n = 0;
      break;
    }
    ptr++;
  }
  if (!n)
    rc = -1;
  else {
    if (*ptr==32 && *(ptr+1) > 0)
      rc = n;
    else 
      rc = -1;
  }
  return rc;
}

// 編集中画面をスクロールアップする
uint8_t tscreenBase::edit_scrollUp() {
  static uint32_t prvlineNum = 0; // 直前の処理行の行数
#if DEPEND_TTBASIC == 0
   scroll_up();
#else
  // 1行分スクロールアップを試みる
  int32_t lineno,nm,len;
  char* text;
  lineno = getLineNum(height-1); // 最終行の表示行番号の取得
  if (lineno <= 0) {
    lineno = prvlineNum;
   }
  if (lineno > 0) {
    // 取得出来た場合、次の行番号を取得する
    nm = getNextLineNo(lineno); 
    if (nm > 0) {
      // 次の行が存在する
      text = getLineStr(nm);
      len = strlen(text);
      for (uint8_t i=0; i < len/width+1; i++) {
        scroll_up();
      }
      strcpy((char*)&VPEEK(0,height-1-(len/width)),text);
      for (uint8_t i=0; i < len/width+1; i++)
         refresh_line(height-1-i);
      prvlineNum = nm; // 今回の処理した行を保持
    } else {
      prvlineNum = 0; // 保持していた処理行をクリア
      scroll_up();      
    }
  } else {
    scroll_up();    
  }
  MOVE(pos_y, pos_x);
#endif
}

// 編集中画面をスクロールダウンする
uint8_t tscreenBase::edit_scrollDown() {
#if DEPEND_TTBASIC == 0
  scroll_down();
#else
  // 1行分スクロールダウンを試みる
  int32_t lineno,prv_nm,len;
  char* text;
  lineno = getLineNum(0); // 最終行の表示行番号の取得
  if (lineno > 0) {
    prv_nm = getPrevLineNo(lineno);
    if (prv_nm > 0) {
      text = getLineStr(prv_nm);
      len = strlen(text);
      for (uint8_t i=0; i < len/width+1; i++) {
        scroll_down();
      }
      strcpy((char*)&VPEEK(0,0),text);
      //refresh();
      for (uint8_t i=0; i < len/width+1; i++)
         refresh_line(0+i);
    } else {
      scroll_down();      
    }
  } else {
    scroll_down();
  }
 MOVE(pos_y, pos_x);
#endif
}

// スクリーン編集
uint8_t tscreenBase::edit() {
  uint8_t ch;  // 入力文字
  do {
    //MOVE(pos_y, pos_x);
    ch = get_ch ();
    switch(ch) {
      case KEY_CR:         // [Enter]キー
        return enter_text();
        break;

      case SC_KEY_CTRL_L:  // [CTRL+L] 画面クリア
      case KEY_F(1):       // F1
        cls();
        locate(0,0);
        break;
 
      case KEY_HOME:      // [HOMEキー] 行先頭移動
        locate(0, pos_y);
        break;
        
      case KEY_NPAGE:      // [PageDown] 表示プログラム最終行に移動
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
        
      case SC_KEY_CTRL_R:  // [CTRL_R] 画面更新
      case KEY_F(5):       // F5
        //beep();
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
      case KEY_F(3):       // F3
        Insert_newLine(pos_y);       
        break;

      case SC_KEY_CTRL_D:  // 行削除
      case KEY_F(2):       // F2
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
