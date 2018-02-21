// 
// スクリーン制御基本クラス ヘッダーファイル
// 作成日 2017/06/27 by たま吉さん
//

#ifndef __tscreenBase_h__
#define __tscreenBase_h__

#define DEPEND_TTBASIC           1     // 豊四季TinyBASIC依存部利用の有無 0:利用しない 1:利用する

#include <Arduino.h>

// 編集キーの定義
#define KEY_TAB       '\t'   // [TAB] key
#define KEY_CR        '\r'   // [Enter] key
#define KEY_BACKSPACE '\b'   // [Backspace] key
#define KEY_ESCAPE    0x1B   // [ESC] key
#define KEY_DOWN      0x80   // [↓] key
#define KEY_UP        0x81   // [↑] key
#define KEY_LEFT      0x82   // [←] key
#define KEY_RIGHT     0x83   // [→] key
#define KEY_HOME      0x84   // [Home] key
#define KEY_DC        0x85   // [Delete] key
#define KEY_IC        0x86   // [Insert] key
#define KEY_NPAGE     0x87   // [PageDown] key
#define KEY_PPAGE     0x88   // [PageUp] key
#define KEY_END       0x89   // [End] key
#define KEY_BTAB      0x8A   // [Back tab] key
#define KEY_F1                  0x8B            // Function key F1
#define KEY_F(n)                (KEY_F1+(n)-1)  // Space for additional 12 function keys

#define KEY_SHIFT_DOWN	KEY_DOWN | 0x40
#define KEY_SHIFT_UP	KEY_UP | 0x40
#define KEY_SHIFT_CR	0xfe

// コントロールキーコードの定義
#define SC_KEY_CTRL_L   12  // 画面を消去
#define SC_KEY_CTRL_R   18  // 画面を再表示
#define SC_KEY_CTRL_X   24  // 1文字削除(DEL)
#define SC_KEY_CTRL_C    3  // break
#define SC_KEY_CTRL_D    4  // 行削除
#define SC_KEY_CTRL_N   14  // 行挿入

#define VPEEK(X,Y)      (screen[width*(Y)+(X)])
#define VPOKE(X,Y,C)    (screen[width*(Y)+(X)]=C)

class tscreenBase {
  protected:
    uint8_t* screen = NULL;     // スクリーン用バッファ
    uint16_t width;             // スクリーン横サイズ
    uint16_t height;            // スクリーン縦サイズ
    uint16_t maxllen;           // 1行最大長さ
    uint16_t pos_x;             // カーソル横位置
    uint16_t pos_y;             // カーソル縦位置
    uint8_t*  text;             // 行確定文字列
    uint8_t flgIns;             // 編集モード
    uint8_t dev;                // 文字入力デバイス
    uint8_t flgCur;             // カーソル表示設定
    uint8_t flgExtMem = 0;      // 外部確保メモリ利用フラグ
	
protected:
    virtual void INIT_DEV() = 0;                              // デバイスの初期化
	virtual void END_DEV() {};                                // デバイスの終了
    virtual void MOVE(uint8_t y, uint8_t x) = 0;              // キャラクタカーソル移動
    virtual void WRITE(uint8_t x, uint8_t y, uint8_t c) = 0;  // 文字の表示
    virtual void CLEAR() = 0;                                 // 画面全消去
    virtual void CLEAR_LINE(uint8_t l)  = 0;                  // 行の消去
    virtual void SCROLL_UP()  = 0;                            // スクロールアップ
    virtual void SCROLL_DOWN() = 0;                           // スクロールダウン
    virtual void INSLINE(uint8_t l) = 0;                      // 指定行に1行挿入(下スクロール)
    
  public:
	virtual void beep() {};                              // BEEP音の発生
    virtual void show_curs(uint8_t flg) = 0;                 // カーソルの表示/非表示
    virtual void draw_cls_curs() = 0;                        // カーソルの消去
    virtual void putch(uint8_t c);                       // 文字の出力
    virtual uint8_t get_ch() = 0;                            // 文字の取得
    virtual bool isKeyIn() = 0;                           // キー入力チェック
	virtual void setColor(uint16_t fc, uint16_t bc) {};  // 文字色指定
	virtual void setAttr(uint16_t attr) {};              // 文字属性

	//virtual int16_t peek_ch();                           // キー入力チェック(文字参照)
    virtual inline uint8_t IS_PRINT(uint8_t ch) {
      return (((ch) >= 32 && (ch) < 0x7F) || ((ch) >= 0xA0)); 
    };
    void init(uint16_t w=0,uint16_t h=0,uint16_t ln=128, uint8_t* extmem=NULL); // スクリーンの初期設定
	virtual void end();                               // スクリーン利用終了
    void clerLine(uint16_t l);                        // 1行分クリア
    void cls();                                       // スクリーンのクリア
    void refresh();                                   // スクリーンリフレッシュ表示
    virtual void refresh_line(uint16_t l);            // 行の再表示
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
    void locate(uint16_t x, uint16_t y);              // カーソルを指定位置に移動
    uint8_t enter_text();                             // 行入力確定ハンドラ
    virtual void newLine();                           // 改行出力
    void Insert_newLine(uint16_t l);                  // 指定行に空白挿入 
    void edit_scrollUp();                             // スクロールして前行の表示
    void edit_scrollDown();                           // スクロールして次行の表示
    uint16_t vpeek(uint16_t x, uint16_t y);           // カーソル位置の文字コード取得
    
    inline uint8_t *getText() { return &text[0]; };   // 確定入力の行データアドレス参照
    inline uint8_t *getScreen() { return screen; };   // スクリーン用バッファアドレス参照
    inline uint16_t c_x() { return pos_x;};           // 現在のカーソル横位置参照
    inline uint16_t c_y() { return pos_y;};           // 現在のカーソル縦位置参照
    inline uint16_t getWidth() { return width;};      // スクリーン横幅取得
    inline uint16_t getHeight() { return height;};    // スクリーン縦幅取得
    inline uint16_t getScreenByteSize() {return width*height;}; // スクリーン領域バイトサイズ
    int16_t getLineNum(int16_t l);                    // 指定行の行番号の取得
};

#endif

