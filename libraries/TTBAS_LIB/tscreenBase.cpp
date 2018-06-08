// 
// スクリーン制御基本クラス ヘッダーファイル
// 作成日 2017/06/27 by たま吉さん
// 修正日 2017/08/05 ファンクションキーをNTSC版同様に利用可能対応
// 修正日 2017/08/12 edit_scrollUp() で最終行が2行以上の場合の処理ミス修正

#include "tscreenBase.h"
#include "../../ttbasic/basic.h"

// スクリーンの初期設定
// 引数
//  w      : スクリーン横文字数
//  h      : スクリーン縦文字数
//  l      : 1行の最大長
//  extmem : 外部獲得メモリアドレス NULL:なし NULL以外 あり
// 戻り値
//  なし
void tscreenBase::init(uint16_t w, uint16_t h, uint16_t l,uint8_t* extmem) {
  if (screen && (w != whole_width || h != whole_height)) {
    free(screen);
    screen = NULL;
  }
  if (!screen)
    screen = (uint8_t*)malloc(w * h);

  whole_width = width   = w;
  whole_height = height  = h;
  maxllen = l;
  win_x = 0; win_y = 0;

  // デバイスの初期化
  INIT_DEV();

  cls();
  show_curs(true);  
  MOVE(pos_y, pos_x);

  // 編集機能の設定
  flgIns = true;
  flgScroll = true;
}

// スクリーン利用終了
void tscreenBase::end() {
  
  // デバイスの終了処理
  END_DEV();

  // 動的確保したメモリーの開放
  if (screen != NULL) {
    free(screen);
    screen = NULL;
  }
}

// 指定行の1行分クリア
void tscreenBase::clerLine(uint16_t l, int from) {
  memset(&VPEEK(from, l), 0, width - from);
  CLEAR_LINE(l, from);
  MOVE(pos_y, pos_x);
}

// スクリーンのクリア
void tscreenBase::cls() {
  CLEAR();
  for (int i = 0; i < height; ++i)
    memset(&VPEEK(0, i), 0, width);
}
void tscreenBase::forget() {
  for (int i = 0; i < height; ++i)
    memset(&VPEEK(0, i), 0, width);
}

// スクリーンリフレッシュ表示
void tscreenBase::refresh() {
  for (uint16_t i = 0; i < height; i++)
    refresh_line(i);
  MOVE(pos_y, pos_x);
}

// 1行分スクリーンのスクロールアップ
void tscreenBase::scroll_up() {
  if (!flgScroll)
    return;
  for (int i = 1; i < height; ++i)
    memmove(&VPEEK(0, i - 1), &VPEEK(0, i), width);
  if (flgCur)
    draw_cls_curs();
  SCROLL_UP();
  clerLine(height-1);
  MOVE(pos_y, pos_x);
}

// 1行分スクリーンのスクロールダウン
void tscreenBase::scroll_down() {
  if (!flgScroll)
    return;
  for (int i = 0; i < height-1; ++i)
    memmove(&VPEEK(0, i + 1), &VPEEK(0, i), width);
  if (flgCur)
    draw_cls_curs();
  SCROLL_DOWN();
  clerLine(0);
  MOVE(pos_y, pos_x);
}

// 指定行に空白行挿入
void tscreenBase::Insert_newLine(uint16_t l) {
  if (l < height-1) {
    for (int i = l + 1; i < height - 1; ++i)
      memmove(&VPEEK(0, i+1), &VPEEK(0, i), width);
    //memmove(screen+(l+2)*width, screen+(l+1)*width, width*(height-1-l-1));
  }
  memset(&VPEEK(0, l+1), 0, width);
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
void tscreenBase::putch(uint8_t c, bool lazy) {
 if (!lazy || VPEEK(pos_x, pos_y) != c) {
   VPOKE(pos_x, pos_y, c);
   if (!flgCur)
     WRITE(pos_x, pos_y, c);
 }
 movePosNextNewChar();
}

// 現在のカーソル位置に文字を挿入
void tscreenBase::Insert_char(uint8_t c) {  
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
      start_adr-=whole_width;
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
      start_adr-=whole_width;
      MOVE(pos_y-1, pos_x);
    } else  if ( ((pos_x + ln +1)%width == width-1) && !VPEEK(pos_x + ln , pos_y) ) {
       // 画面左端に1文字を書く場合で、次行と連続でない場合は下の行に1行空白を挿入する
          Insert_newLine(pos_y+(pos_x+ln)/width);
    }
    // 1文字挿入のために1文字分のスペースを確保
    memmove(start_adr+1, start_adr, ln);	// XXX: off-by-one?
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
void tscreenBase::movePosNextLineChar(bool force) {
  if (pos_y+1 < height) {
    if (force) {
      char* text;
      uint8_t y = pos_y;
      while (y && VPEEK(width-1, y-1))
        y--;
      int lineno = getLineNum(y);
      if (lineno > 0) {
        int curlen = strlen(getLineStr(lineno));
        int nm = getNextLineNo(lineno); 
        if (nm > 0) {
          text = getLineStr(nm);
          int len = strlen(text);

          // scroll up if the line doesn't fit
          int remaining_lines = height - y - curlen/width;
          for (int i=0; i < len/width+1 - remaining_lines; i++) {
            scroll_up();
            y--;
          }

          MOVE(y + curlen/width + 1, 0);
          getLineStr(nm, 0);
          MOVE(pos_y - 1, pos_x);
        }
      }
    }
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
void tscreenBase::movePosPrevLineChar(bool force) {
  if (pos_y > 0) {
    if (force) {
      char* text;
      uint8_t y = pos_y;
      while (y && VPEEK(width-1, y-1))
        y--;
      int lineno = getLineNum(y);
      if (lineno > 0) {
        int nm = getPrevLineNo(lineno); 
        if (nm > 0) {
          text = getLineStr(nm);
          int len = strlen(text);

          // scroll down if the line doesn't fit
          int remaining_lines = y;
          for (int i=0; i < len/width+1 - remaining_lines; i++) {
            scroll_down();
            pos_y++;
            y++;
          }

          MOVE(y - len/width - 1, 0);
          getLineStr(nm, 0);
          MOVE(pos_y + 1, pos_x);
        }
      }
    }
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
  int16_t x = width-2;
  while(1) {
    if (IS_PRINT(VPEEK(x, pos_y)) ) 
       break;  
    if (x > 0)
      x--;
    else
      break;
  }        
  MOVE(pos_y, x+1);
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
void tscreenBase::locate(uint16_t x, int16_t y) {
  if ( x >= width )  x = width-1;
  if (y < 0)
    y = pos_y;
  else if ( y >= height) 
    y = height;
  MOVE(y, x);
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
  while (top > &VPEEK(0, 0) && *top != 0 )
    top--;
  if ( top != &VPEEK(0, 0) ) top++;
  text = top;
  return ((top - &VPEEK(0, 0)) + strlen((char *)top)) / width + 1 - pos_y;
}

// 指定行の行番号の取得
int16_t tscreenBase::getLineNum(int16_t l) {
  uint8_t* ptr = &VPEEK(0, l);
  uint32_t n = 0;
  int rc = -1;  
  while (isDigit(*ptr)) {
    n *= 10;
    n+= *ptr-'0';
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
void tscreenBase::edit_scrollUp() {
#if DEPEND_TTBASIC == 0
   scroll_up();
#else
  // 1行分スクロールアップを試みる
  int32_t lineno,nm,len;
  char* text;
  uint8_t y = height - 1;
  while (y && VPEEK(width-1, y-1))
    y--;
  lineno = getLineNum(y);
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
      // print to screen
      MOVE(height-1 - len/width, 0);
      getLineStr(nm, 0);
    } else {
      scroll_up();      
    }
  } else {
    scroll_up();    
  }
  MOVE(pos_y, pos_x);
#endif
}

// Scroll down the editing screen
void tscreenBase::edit_scrollDown() {
#if DEPEND_TTBASIC == 0
  scroll_down();
#else
  // Try scrolling down by one line
  int32_t lineno,prv_nm,len;
  char* text;
  lineno = getLineNum(0); // Obtain display line number of the last line
  if (lineno > 0) {
    prv_nm = getPrevLineNo(lineno);
    if (prv_nm > 0) {
      text = getLineStr(prv_nm);
      len = strlen(text);
      for (uint8_t i=0; i < len/width+1; i++) {
        scroll_down();
      }
      // print to screen
      MOVE(0, 0);
      getLineStr(prv_nm, 0);
    } else {
      scroll_down();      
    }
  } else {
    scroll_down();
  }
 MOVE(pos_y, pos_x);
#endif
}
