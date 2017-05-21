//
// file: tscreen.h
// ターミナルスクリーン制御ライブラリ ヘッダファイル for Arduino STM32
// V1.0 作成日 2017/03/22 by たま吉さん
//  修正日 2017/03/26, 色制御関連関数の追加
//  修正日 2017/03/30, moveLineEnd()の追加,[HOME],[END]の編集キーの仕様変更
//  修正日 2017/04/02, getScreenByteSize()の追加
//  修正日 2017/04/03, getScreenByteSize()の不具合対応
//  修正日 2017/04/06, beep()の追加
//  修正日 2017/04/11, TVout版に移行
//  修正日 2017/04/15, 行挿入の追加
//  修正日 2017/04/17, bitmap表示処理の追加
//  修正日 2017/04/18, シリアルポート設定機能の追加,cscroll,gscroll表示処理の追加
//  修正日 2017/04/25, gscrollの機能修正, gpeek,ginpの追加
//  修正日 2017/04/29, キーボード、NTSCの補正対応
//  修正日 2017/06/19, getfontadr()の追加
//

#ifndef __tscreen_h__
#define __tscreen_h__

#include <Arduino.h>
//#include <mcurses.h>
#define KEY_TAB                 '\t'   // TAB key
#define KEY_CR                  '\r'   // RETURN key
#define KEY_BACKSPACE           '\b'   // Backspace key
#define KEY_ESCAPE              0x1B   // ESCAPE (pressed twice)

#define KEY_DOWN                0x80   // Down arrow key
#define KEY_UP                  0x81   // Up arrow key
#define KEY_LEFT                0x82   // Left arrow key
#define KEY_RIGHT               0x83   // Right arrow key
#define KEY_HOME                0x84   // Home key
#define KEY_DC                  0x85   // Delete character key
#define KEY_IC                  0x86   // Ins char/toggle ins mode key
#define KEY_NPAGE               0x87   // Next-page key
#define KEY_PPAGE               0x88   // Previous-page key
#define KEY_END                 0x89   // End key
#define KEY_BTAB                0x8A   // Back tab key

#define SC_KEY_CTRL_L   12  // 画面を消去
#define SC_KEY_CTRL_R   18  // 画面を再表示
#define SC_KEY_CTRL_X   24  // 1文字削除(DEL)
#define SC_KEY_CTRL_C    3  // break
#define SC_KEY_CTRL_D    4  // 行削除
#define SC_KEY_CTRL_N   14  // 行挿入

uint8_t* tv_getFontAdr() ;

// スクリーン定義
#define SC_FIRST_LINE  0  // スクロール先頭行
//#define SC_LAST_LINE  24  // スクロール最終行

#define SC_TEXTNUM   256  // 1行確定文字列長さ

class tscreen {
  private:
    uint8_t* screen = NULL;     // スクリーン用バッファ
    uint16_t width;             // スクリーン横サイズ
    uint16_t height;            // スクリーン縦サイズ
    uint16_t gwidth;             // スクリーングラフィック横サイズ
    uint16_t gheight;            // スクリーングラフィック縦サイズ
    uint16_t maxllen;           // 1行最大長さ
    uint8_t*  text;             // 行確定文字列
    uint8_t flgIns;             // 編集モード

    uint8_t flgCur;             // カーソル表示設定
    //uint8_t flgSirialout;       // SUBシリアル入出力指定フラグ
    uint8_t serialMode;         // シリアルポートモード

  public:
    uint16_t pos_x;             // カーソル横位置
    uint16_t pos_y;             // カーソル縦位置

    uint16_t prev_pos_x;        // カーソル横位置
    uint16_t prev_pos_y;        // カーソル縦位置
 
    //void init(uint16_t w,uint16_t h,uint16_t ln=256); // スクリーンの初期設定
    void init(uint16_t ln=256, uint8_t kbd_type=false, int16_t NTSCajst=0); // スクリーンの初期設定

    void reset_kbd(uint8_t kbd_type=false);
    void clerLine(uint16_t l);                        // 1行分クリア
    void cls();                                       // スクリーンのクリア
    void refresh();                                   // スクリーンリフレッシュ表示
    void refresh_line(uint16_t l);                    // 行の再表示
    void scroll_up();                                 // 1行分スクリーンのスクロールアップ
    void delete_char() ;                              // 現在のカーソル位置の文字削除
    void putch(uint8_t c);                            // 文字の出力
    uint8_t get_ch();                                 // 文字の取得
    uint8_t isKeyIn();                                // キー入力チェック
    //int16_t peek_ch();                              // キー入力チェック(文字参照)
    void Insert_char(uint8_t c);                      // 現在のカーソル位置に文字を挿入
    void movePosNextNewChar();                        // カーソルを１文字分次に移動
    void movePosPrevChar();                           // カーソルを1文字分前に移動
    void movePosNextChar();                           // カーソルを1文字分次に移動
    void movePosNextLineChar();                       // カーソルを次行に移動
    void movePosPrevLineChar();                       // カーソルを前行に移動
    void moveLineEnd();                               // カーソルを行末に移動
    void moveBottom();                                // スクリーン表示の最終表示の行先頭に移動
    void locate(uint16_t x, uint16_t y);              // カーソルを指定位置に移動
    uint8_t edit();                                   // スクリーン編集
    uint8_t enter_text();                             // 行入力確定ハンドラ
    void newLine();                                   // 改行出力
    void Insert_newLine(uint16_t l);                  // 指定行に空白挿入

    void MOVE(uint8_t y, uint8_t x);                  // キャラクタカーソル移動
    void show_curs(uint8_t flg);                      // カーソルの表示/非表示制御
    void dwaw_cls_curs();                             // カーソルの消去
  
    uint16_t vpeek(uint16_t x, uint16_t y);           // カーソル位置の文字コード取得
  
    inline uint8_t *getText() { return text; };       // 確定入力の行データアドレス参照
    inline uint8_t *getfontadr() { return tv_getFontAdr()+3; };   // フォントアドレスの参照

    inline uint8_t *getScreen() { return screen; };   // スクリーン用バッファアドレス参照
    inline uint16_t c_x() { return pos_x;};           // 現在のカーソル横位置参照
    inline uint16_t c_y() { return pos_y;};           // 現在のカーソル縦位置参照
    inline uint16_t getWidth() { return width;};      // スクリーン横幅取得
    inline uint16_t getHeight() { return height;};    // スクリーン縦幅取得
    inline uint16_t getGWidth() { return gwidth;};      // グラフックスクリーン横幅取得
    inline uint16_t getGHeight() { return gheight;};    // グラフックスクリーン縦幅取得
    inline uint16_t getScreenByteSize() {return width * height ;} // スクリーン領域バイトサイズ

    void setColor(uint16_t fc, uint16_t bc);          // 文字色指定
    void setAttr(uint16_t attr);                      // 文字属性

    void beep() {/*addch(0x07);*/};
    void tone(int16_t freq, int16_t tm);
    void notone();
    
    inline uint8_t IS_PRINT(uint8_t ch) {
      //return (((ch) >= 32 && (ch) < 0x7F) || ((ch) >= 0xA0)); 
      return (ch >= 32); 
    };

    // グラフィック描画
    void pset(int16_t x, int16_t y, uint8_t c);
    void line(int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint8_t c);
    void circle(int16_t x, int16_t y, int16_t r, uint8_t c, int8_t f);
    void rect(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c, int8_t f);
    void bitmap(int16_t x, int16_t y, uint8_t* adr, uint16_t index, uint16_t w, uint16_t h, uint16_t d);
    void cscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t d);
    void gscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t mode);
    int16_t gpeek(int16_t x, int16_t y);
    int16_t ginp(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t c);
    void set_gcursor(uint16_t, uint16_t);
    void gputch(uint8_t c);
    
    // シリアルポート制御
    void    Serial_open(uint32_t baud) ;  // シリアルポートオープン
    void    Serial_close();               // シリアルポートクローズ
    void    Serial_write(uint8_t c);      // シリアル1バイト出力
    int16_t Serial_read();                // シリアル1バイト入力
    uint8_t Serial_available();           // シリアルデータ着信チェック
    void    Serial_newLine();             // シリアル改行出力
    void    Serial_mode(uint8_t c, uint32_t b);  // シリアルポートモード設定 

    // システム設定
    void  adjustNTSC(int16_t ajst);
    
};

#endif
