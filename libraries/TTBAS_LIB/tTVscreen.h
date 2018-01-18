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
#if USE_VS23 == 1
#include "../../ttbasic/vs23s010.h"
#endif
#include "tscreenBase.h"
#include "tGraphicDev.h"

// PS/2キーボードの利用 0:利用しない 1:利用する
#define PS2DEV     1  

//uint8_t* tv_getFontAdr() ;

// スクリーン定義
#define SC_FIRST_LINE  0  // スクロール先頭行
//#define SC_LAST_LINE  24  // スクロール最終行

#define SC_TEXTNUM   256  // 1行確定文字列長さ

void tv_init(int16_t ajst, uint8_t* extmem=NULL, uint8_t vmode=SC_DEFAULT);
void tv_end();
void    tv_write(uint8_t x, uint8_t y, uint8_t c);
void    tv_drawCurs(uint8_t x, uint8_t y);
void    tv_clerLine(uint16_t l) ;
void    tv_insLine(uint16_t l);
void    tv_cls();
void    tv_scroll_up();
void    tv_scroll_down();
uint8_t tv_get_cwidth();
uint8_t tv_get_cheight();
uint8_t tv_get_win_cwidth();
uint8_t tv_get_win_cheight();
void    tv_write(uint8_t c);
void    tv_tone(int16_t freq, int16_t tm);
void    tv_notone();
void    tv_NTSC_adjust(int16_t ajst);
void	tv_window_set(uint8_t x, uint8_t y, uint8_t w, uint8_t h);
void	tv_setFont(const uint8_t *font);

//class tTVscreen : public tscreenBase, public tSerialDev, public tGraphicDev {
class tTVscreen : public tscreenBase, public tGraphicDev {
  private:
    uint8_t enableCursor;

  protected:
    void INIT_DEV(){};                           // デバイスの初期化
    void MOVE(uint8_t y, uint8_t x);             // キャラクタカーソル移動 **
    void WRITE(uint8_t x, uint8_t y, uint8_t c); // 文字の表示
    void CLEAR();                                // 画面全消去
    void CLEAR_LINE(uint8_t l);                  // 行の消去
    void SCROLL_UP();                            // スクロールアップ
    void SCROLL_DOWN();                          // スクロールダウン
    void INSLINE(uint8_t l);                     // 指定行に1行挿入(下スクロール)

  public:
    uint16_t prev_pos_x;        // カーソル横位置
    uint16_t prev_pos_y;        // カーソル縦位置
 
    void init( uint16_t ln=256, uint8_t kbd_type=false,
    	       int16_t NTSCajst=0, uint8_t* extmem=NULL, 
               uint8_t vmode=SC_DEFAULT);                // スクリーンの初期設定
	void end();                                          // スクリーンの利用の終了
    void Serial_Ctrl(int16_t ch);
    void reset_kbd(uint8_t kbd_type=false);
    void putch(uint8_t c);                            // 文字の出力
    uint8_t get_ch();                                 // 文字の取得
    inline uint8_t getDevice() {return dev;};         // 文字入力元デバイス種別の取得
    uint8_t isKeyIn();                                // キー入力チェック 
    uint8_t edit();                                   // スクリーン編集
    void newLine();                                   // 改行出力
    void refresh_line(uint16_t l);                    // 行の再表示
	
    void drawCursor(uint8_t flg);
    void show_curs(uint8_t flg);                      // カーソルの表示/非表示制御
    void draw_cls_curs();                             // カーソルの消去

    void setColor(uint16_t fc, uint16_t bc);          // 文字色指定
    //void setAttr(uint16_t attr);                      // 文字属性
    void beep() {/*addch(0x07);*/};

    inline uint8_t IS_PRINT(uint8_t ch) {
      //return (((ch) >= 32 && (ch) < 0x7F) || ((ch) >= 0xA0)); 
      return (ch >= 32); 
    };

    void tone(int16_t freq, int16_t tm);
    void notone();
    void cscroll(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t d);

    // システム設定
    void  adjustNTSC(int16_t ajst);

    inline void setWindow(uint8_t x, uint8_t y, uint8_t w, uint8_t h) {
      tv_window_set(x, y, w, h);
    }    
    inline uint8_t getScreenWidth() {
      return tv_get_cwidth();
    }
    inline uint8_t getScreenHeight() {
      return tv_get_cheight();
    }
    void setFont(const uint8_t *font);
};

#endif
