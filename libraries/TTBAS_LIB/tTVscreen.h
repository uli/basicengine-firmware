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
//  修正日 2017/05/19, getfontadr()の追加
//  修正日 2017/05/28, 上下スクロール編集対応
//  修正日 2017/06/09, シリアルからは全てもコードを通すように修正
//  修正日 2017/06/22, シリアルからは全てもコードを通す切り替え機能の追加
//

#ifndef __tTVscreen_h__
#define __tTVscreen_h__

#include "../../ttbasic/ttconfig.h"

#include <Arduino.h>
#include <utf8.h>

#if USE_VS23 == 1
#include "../../ttbasic/video.h"
#endif
#include "tscreenBase.h"
#include "tGraphicDev.h"

// PS/2キーボードの利用 0:利用しない 1:利用する
#define PS2DEV 1
#include "ps22tty.h"

//uint8_t* tv_getFontAdr() ;

// スクリーン定義
#define SC_FIRST_LINE 0  // スクロール先頭行
//#define SC_LAST_LINE  24  // スクロール最終行

#define SC_TEXTNUM 256  // 1行確定文字列長さ

#include "tvutil.h"

typedef int (*output_filter_ptr)(int c, void *userdata);
struct output_filter {
  output_filter_ptr filter;
  void *userdata;
};

typedef int (*input_filter_ptr)(int c, void *userdata);
struct input_filter {
  input_filter_ptr filter;
  void *userdata;
};

//class tTVscreen : public tscreenBase, public tSerialDev, public tGraphicDev {
class tTVscreen : public tscreenBase, public tGraphicDev {
private:
  uint8_t enableCursor;
  uint8_t m_cursor_count;
  bool m_cursor_state;
  std::vector<output_filter> output_filters;
  std::vector<input_filter> input_filters;

protected:
  void INIT_DEV(){};                  // デバイスの初期化
  void MOVE(uint16_t y, uint16_t x);  // キャラクタカーソル移動 **
  void WRITE(uint16_t x, uint16_t y, utf8_int32_t c);  // 文字の表示
  void WRITE_COLOR(uint16_t x, uint16_t y, utf8_int32_t c, pixel_t fg, pixel_t bg);
  void CLEAR();                               // 画面全消去
  void CLEAR_LINE(uint16_t l, int from = 0);  // 行の消去
  void SCROLL_UP();                           // スクロールアップ
  void SCROLL_DOWN();                         // スクロールダウン
  void INSLINE(uint16_t l);  // 指定行に1行挿入(下スクロール)

  void updateWindow();

public:
  uint16_t prev_pos_x;  // カーソル横位置
  uint16_t prev_pos_y;  // カーソル縦位置

  inline void write(uint16_t x, uint16_t y, utf8_int32_t c) {
    tv_write(x, y, c);
    VPOKE(x, y, c);
    VPOKE_CCOL(x, y);
  }
  void init(uint16_t ln = 256,
            int16_t NTSCajst = 0,
            uint8_t vmode = SC_DEFAULT);  // スクリーンの初期設定
  void end();                             // スクリーンの利用の終了
  void Serial_Ctrl(int16_t ch);
  void reset_kbd(uint8_t kbd_type = false);

  inline void putch(utf8_int32_t c, bool lazy = false) {
    for (auto h : output_filters) {
      c = h.filter(c, h.userdata);
      if (!c)
        return;
    }
    tscreenBase::putch(c, lazy);
#ifdef DEBUG
    Serial.write(c);  // シリアル出力
#endif
  }

  void add_output_filter(output_filter_ptr filter, void *userdata) {
    struct output_filter h = { filter, userdata };
    output_filters.push_back(h);
  }

  void add_input_filter(input_filter_ptr filter, void *userdata) {
    struct input_filter h = { filter, userdata };
    input_filters.push_back(h);
  }

  virtual utf8_int32_t get_ch();  // 文字の取得
  inline utf8_int32_t tryGetChar() {
    return ps2read();
  }
  inline uint8_t getDevice() {  // 文字入力元デバイス種別の取得
    return dev;
  }
  bool isKeyIn();  // キー入力チェック
  inline utf8_int32_t peekKey() {
    return ps2peek();
  }
  uint8_t edit();                 // スクリーン編集
  void newLine();                 // 改行出力
  void refresh_line(uint16_t l);  // 行の再表示

  void drawCursor(uint8_t flg);
  void updateCursor();
  void show_curs(uint8_t flg);  // カーソルの表示/非表示制御
  bool cursor_enabled() {
    return enableCursor;
  }
  void draw_cls_curs();         // カーソルの消去

  void setColor(pixel_t fc, pixel_t bc);  // 文字色指定
  void setColorIndexed(ipixel_t fc, ipixel_t bc);
  inline void flipColors() {
    tv_flipcolors();
  }
  inline pixel_t getFgColor() {
    return fg_color;
  }
  inline pixel_t getBgColor() {
    return bg_color;
  }
  inline void setCursorColor(pixel_t cc) {
    tv_setcursorcolor(cc);
  }
  void beep(){
    /*addch(0x07);*/
  };

  inline uint8_t IS_PRINT(utf8_int32_t ch) {
    //return (((ch) >= 32 && (ch) < 0x7F) || ((ch) >= 0xA0));
    // XXX: w0t?
    return (ch > 0);
  };

  void cscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t d);

  inline void setWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    tv_window_set(x, y, w, h);
    win_x = x;
    win_y = y;
    width = w;
    height = h;
  }
  inline void getWindow(int &x, int &y, int &w, int &h) {
    tv_window_get(x, y, w, h);
  }
  inline void reset() {
    tv_reinit();
    tscreenBase::init(whole_width, whole_height, maxllen, screen);
    setWindow(0, 0, whole_width, whole_height);
  }

  inline uint16_t getScreenWidth() {
    return tv_get_cwidth();
  }
  inline uint16_t getScreenHeight() {
    return tv_get_cheight();
  }

  int addFont(const uint8_t *font, const char *name);
  bool haveFont(const char *name) {
    return tv_have_font(name);
  }
  void setFontByIndex(int idx);
  bool setFontByName(const char *name, int w, int h);

  inline int getFontHeight() {
    return tv_font_height();
  }
  inline int getFontWidth() {
    return tv_font_width();
  }
  inline int fontCount() {
    return tv_font_count();
  }
  inline const char *fontInfo(int idx, int *w, int *h) {
    return tv_font_info(idx, w, h);
  }
  inline int currentFontIndex() {
    return tv_current_font_index();
  }

  void saveScreenshot();
};

#endif
