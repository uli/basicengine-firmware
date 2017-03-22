//
// file: tscreen.h
// ターミナルスクリーン制御ライブラリ ヘッダファイル for Arduino STM32
// V1.0 作成日 2017/03/22 by たま吉さん
//

#ifndef __tscreen_h__
#define __tscreen_h__

#include <Arduino.h>
#include <mcurses.h>

// スクリーン定義
#define SC_FIRST_LINE  0  // スクロール先頭行
#define SC_LAST_LINE  22  // スクロール最終行

#define SC_TEXTNUM   256  // 1行確定文字列長さ

class tscreen {
  private:
    uint8_t* screen = NULL;     // スクリーン用バッファ
    uint16_t width;             // スクリーン横サイズ
    uint16_t height;            // スクリーン縦サイズ
    uint16_t maxllen;           // 1行最大長さ
    uint16_t pos_x;             // カーソル横位置
    uint16_t pos_y;             // カーソル縦位置
    uint8_t  text[SC_TEXTNUM] ; // 行確定文字列
    uint8_t flgIns;             // 編集モード

  public:

    void init(uint16_t w,uint16_t h,uint16_t ln=256); // スクリーンの初期設定     
    void clerLine(uint16_t l);                        // 1行分クリア
    void cls();                                       // スクリーンのクリア
    void refresh();                                   // スクリーンリフレッシュ表示
    void scroll_up();                                 // 1行分スクリーンのスクロールアップ
    void delete_char() ;                              // 現在のカーソル位置の文字削除
    void putch(uint8_t c);                            // 文字の出力
    uint8_t get_ch();                                 // 文字の取得
    uint8_t isKeyIn();                                // キー入力チェック
    void Insert_char(uint8_t c);                      // 現在のカーソル位置に文字を挿入
    void movePosNextNewChar();                        // カーソルを１文字分次に移動
    void movePosPrevChar();                           // カーソルを1文字分前に移動
    void movePosNextChar();                           // カーソルを1文字分次に移動
    void movePosNextLineChar();                       // カーソルを次行に移動
    void movePosPrevLineChar();                       // カーソルを前行に移動
    void locate(uint16_t x, uint16_t y);              // カーソルを指定位置に移動
    uint8_t edit();                                   // スクリーン編集
    uint8_t enter_text();                             // 行入力確定ハンドラ
    void newLine();                                   // 改行出力
    inline uint8_t *getText() { return &text[0]; };   // 確定入力の行データアドレス参照
    inline uint8_t *getScreen() { return screen; };   // スクリーン用バッファアドレス参照
    inline uint16_t c_x() { return pos_x;};           // 現在のカーソル横位置参照
    inline uint16_t c_y() { return pos_y;};           // 現在のカーソル縦位置参照

   private:
    inline uint8_t IS_PRINT(uint8_t ch) {
      return (((ch) >= 32 && (ch) < 0x7F) || ((ch) >= 0xA0)); 
    };
};

#endif
