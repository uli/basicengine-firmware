//
// スクリーン制御基本クラス ヘッダーファイル
// 作成日 2017/06/27 by たま吉さん
//

#ifndef __tscreenBase_h__
#define __tscreenBase_h__

#define DEPEND_TTBASIC           1     // 豊四季TinyBASIC依存部利用の有無 0:利用しない 1:利用する

#include "ttconfig.h"
#include <Arduino.h>

// Definition of edit key
#define SC_KEY_TAB       '\t'   // [TAB] key
#define SC_KEY_CR        '\r'   // [Enter] key
#define SC_KEY_BACKSPACE '\b'   // [Backspace] key
#define SC_KEY_ESCAPE    0x1B   // [ESC] key
#define SC_KEY_DOWN      0x180   // [↓] key
#define SC_KEY_UP        0x181   // [↑] key
#define SC_KEY_LEFT      0x182   // [←] key
#define SC_KEY_RIGHT     0x183   // [→] key
#define SC_KEY_HOME      0x184   // [Home] key
#define SC_KEY_DC        0x185   // [Delete] key
#define SC_KEY_IC        0x186   // [Insert] key
#define SC_KEY_NPAGE     0x187   // [PageDown] key
#define SC_KEY_PPAGE     0x188   // [PageUp] key
#define SC_KEY_END       0x189   // [End] key
#define SC_KEY_BTAB      0x18A   // [Back tab] key
#define SC_KEY_F1                  0x18B            // Function key F1
#define SC_KEY_F(n)                (SC_KEY_F1+(n)-1)  // Space for additional 12 function keys

#define SC_KEY_PRINT     0x1A0

#define SC_KEY_SHIFT_DOWN	SC_KEY_DOWN | 0x40
#define SC_KEY_SHIFT_UP	SC_KEY_UP | 0x40

// Definition of control key code
#define SC_KEY_CTRL_L   12  // 画面を消去
#define SC_KEY_CTRL_R   18  // 画面を再表示
#define SC_KEY_CTRL_X   24  // 1文字削除(DEL)
#define SC_KEY_CTRL_C    3  // break
#define SC_KEY_CTRL_D    4  // 行削除
#define SC_KEY_CTRL_N   14  // 行挿入

// text memory access macros
#define VPEEK(X,Y)      (screen[whole_width*((Y)+win_y) + (X)+win_x])
#define VPOKE(X,Y,C)    (screen[whole_width*((Y)+win_y) + (X)+win_x]=(C))

// color memory access macros
#define COLMEM(X, Y) (&colmem[(whole_width * (Y) + (X)) * 2])

#define VPEEK_FG(X, Y)	(COLMEM(X, Y)[0])
#define VPEEK_BG(X, Y)	(COLMEM(X, Y)[1])
#define VPOKE_FGBG(X,Y,F,B) do { \
  COLMEM(X, Y)[0] = (F); \
  COLMEM(X, Y)[1] = (B); \
} while (0)

#define VMOVE_C(X1, Y1, X2, Y2, W) do { \
  memmove(COLMEM(X2, Y2), COLMEM(X1, Y1), (W) * sizeof(IPIXEL_TYPE) * 2); \
} while (0)

#define VSET_C(X,Y,F,B,W) do { \
  for (int _x = (X); _x < (X)+(W); ++_x) { \
    COLMEM(_x, (Y))[0] = (F); \
    COLMEM(_x, (Y))[1] = (B); \
  } \
} while (0)

// poke current FG/BG colors to color memory
#define VPOKE_CCOL(X, Y)	VPOKE_FGBG(X, Y, fg_color, bg_color)

class tscreenBase {
  protected:
    uint8_t* screen = NULL;     // スクリーン用バッファ
    IPIXEL_TYPE *colmem = NULL;
    uint16_t width;             // text window width
    uint16_t height;            // text window height
    uint16_t whole_width, whole_height;	// full screen width/height (chars)
    uint16_t win_x, win_y;	// text window position
    uint16_t maxllen;           // 1行最大長さ
    uint16_t pos_x;             // カーソル横位置
    uint16_t pos_y;             // カーソル縦位置
    uint8_t*  text;             // 行確定文字列
    uint8_t dev;                // 文字入力デバイス
    uint8_t flgCur:1;             // カーソル表示設定
    bool flgIns:1;             // 編集モード
    bool flgScroll:1;

protected:
    virtual void INIT_DEV() = 0;                              // デバイスの初期化
	virtual void END_DEV() {};                                // デバイスの終了
    virtual void MOVE(uint16_t y, uint16_t x) = 0;              // キャラクタカーソル移動
    virtual void WRITE(uint16_t x, uint16_t y, uint8_t c) = 0;  // 文字の表示
    virtual void CLEAR() = 0;                                 // 画面全消去
    virtual void CLEAR_LINE(uint16_t l, int from = 0)  = 0;                  // 行の消去
    virtual void SCROLL_UP()  = 0;                            // スクロールアップ
    virtual void SCROLL_DOWN() = 0;                           // スクロールダウン
    virtual void INSLINE(uint16_t l) = 0;                      // 指定行に1行挿入(下スクロール)

  public:
	virtual void beep() {};                              // BEEP音の発生
    virtual void show_curs(uint8_t flg) = 0;                 // カーソルの表示/非表示
    virtual void draw_cls_curs() = 0;                        // カーソルの消去
    void putch(uint8_t c, bool lazy = false);                       // 文字の出力
    virtual bool isKeyIn() = 0;                           // キー入力チェック

	//virtual int16_t peek_ch();                           // キー入力チェック(文字参照)
    virtual inline uint8_t IS_PRINT(uint8_t ch) {
      return (((ch) >= 32 && (ch) < 0x7F) || ((ch) >= 0xA0)); 
    };
    void init(uint16_t w=0,uint16_t h=0,uint16_t ln=128, uint8_t* extmem=NULL); // スクリーンの初期設定
	virtual void end();                               // スクリーン利用終了
    void clerLine(uint16_t l, int from = 0);                        // 1行分クリア
    void cls();                                       // スクリーンのクリア
    void forget();
    void refresh();                                   // スクリーンリフレッシュ表示
    virtual void refresh_line(uint16_t l) { (void)l; }            // 行の再表示
    void scroll_up();                                 // 1行分スクリーンのスクロールアップ
    void scroll_down();                               // 1行分スクリーンのスクロールダウン 
    void delete_char() ;                              // 現在のカーソル位置の文字削除
    inline uint8_t getDevice() {return dev;};         // 文字入力元デバイス種別の取得        ***********
    void Insert_char(uint8_t c);                      // 現在のカーソル位置に文字を挿入
    void movePosNextNewChar();                        // カーソルを１文字分次に移動
    void movePosPrevChar();                           // カーソルを1文字分前に移動
    void movePosNextChar();                           // カーソルを1文字分次に移動
    void movePosNextLineChar(bool force = false);     // カーソルを次行に移動
    void movePosPrevLineChar(bool force = false);     // カーソルを前行に移動
    void moveLineEnd();                               // カーソルを行末に移動
    void moveBottom();                                // スクリーン表示の最終表示の行先頭に移動 
    void locate(uint16_t x, int16_t y = -1);              // カーソルを指定位置に移動
    uint8_t enter_text();                             // 行入力確定ハンドラ
    virtual void newLine();                           // 改行出力
    void Insert_newLine(uint16_t l);                  // 指定行に空白挿入 
    void edit_scrollUp();                             // スクロールして前行の表示
    void edit_scrollDown();                           // スクロールして次行の表示
    // カーソル位置の文字コード取得
    inline uint16_t vpeek(uint16_t x, uint16_t y) {
      if (x >= width || y >= height) 
         return 0;
      return VPEEK(x,y);
    }
    
    inline uint8_t *getText() { return &text[0]; };   // 確定入力の行データアドレス参照
    inline uint8_t *getScreen() { return screen; }   // スクリーン用バッファアドレス参照
    inline uint8_t *getScreenWindow() { return &VPEEK(0, 0); }
    inline uint16_t getStride() { return whole_width; }
    inline uint16_t c_x() { return pos_x;};           // 現在のカーソル横位置参照
    inline uint16_t c_y() { return pos_y;};           // 現在のカーソル縦位置参照
    inline uint16_t getWidth() { return width;};      // スクリーン横幅取得
    inline uint16_t getHeight() { return height;};    // スクリーン縦幅取得
    inline uint16_t getScreenWidth() { return whole_width;};      // スクリーン横幅取得
    inline uint16_t getScreenHeight() { return whole_height;};    // スクリーン縦幅取得
    inline uint16_t getScreenByteSize() {return whole_width*whole_height;}; // スクリーン領域バイトサイズ
    int16_t getLineNum(int16_t l);                    // 指定行の行番号の取得

    inline void setScroll(bool enabled) { flgScroll = enabled; }
};

#endif

