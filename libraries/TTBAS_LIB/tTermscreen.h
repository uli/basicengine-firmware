//
// file: tTermscreen.h
// ターミナルスクリーン制御ライブラリ ヘッダファイル for Arduino STM32
// V1.0 作成日 2017/03/22 by たま吉さん
//  修正日 2017/03/26, 色制御関連関数の追加
//  修正日 2017/03/30, moveLineEnd()の追加,[HOME],[END]の編集キーの仕様変更
//  修正日 2017/04/02, getScreenByteSize()の追加
//  修正日 2017/04/03, getScreenByteSize()の不具合対応
//  修正日 2017/04/06, beep()の追加
//  修正日 2017/06/27, 汎用化のための修正
//

#ifndef __tTermscreen_h__
#define __tTermscreen_h__

#include <Arduino.h>
#include "tscreenBase.h"
//#include "tSerialDev.h"
#include <mcurses.h>
//#undef erase()

//class tTermscreen : public tscreenBase, public tSerialDev {
class tTermscreen : public tscreenBase  {
	protected:
    void INIT_DEV();                             // デバイスの初期化
    void MOVE(uint8_t y, uint8_t x);             // キャラクタカーソル移動
    void WRITE(uint8_t x, uint8_t y, uint8_t c); // 文字の表示
    void CLEAR();                                // 画面全消去
    void CLEAR_LINE(uint8_t l);                  // 行の消去
    void SCROLL_UP();                            // スクロールアップ
    void SCROLL_DOWN();                          // スクロールダウン
    void INSLINE(uint8_t l);                     // 指定行に1行挿入(下スクロール)
    
  public:
    void beep() {addch(0x07);};                  // BEEP音の発生
    void show_curs(uint8_t flg);                 // カーソルの表示/非表示
    void draw_cls_curs();                        // カーソルの消去
    void setColor(uint16_t fc, uint16_t bc);     // 文字色指定
    void setAttr(uint16_t attr);                 // 文字属性
    uint8_t get_ch();                            // 文字の取得
    uint8_t isKeyIn();                           // キー入力チェック
    int16_t peek_ch();                           // キー入力チェック(文字参照)
    inline uint8_t getSerialMode()               // シリアルモードの取得
      { return serialMode; };
};

#endif
