// FILE: TNTSC.h
// Arduino STM32 用 NTSCビデオ出力ライブラリ by たま吉さん
// 作成日 2017/02/20, Blue Pillボード(STM32F103C8)にて動作確認
// 更新日 2017/02/27, delay_frame()の追加、
// 更新日 2017/02/27, フック登録関数追加
// 更新日 2017/03/03, 解像度モード追加
// 更新日 2017/04/05, クロック48MHz対応
// 更新日 2017/04/27, NTSC走査線数補正関数adjust()追加
// 更新日 2017/04/30, SPI1,SPI2の選択指定を可能に修正
// 更新日 2017/06/25, 外部確保VRAMの指定を可能に修正
//

#ifndef __VS23S010_H
#define __VS23S010_H

#include "ntsc.h"
#include <Arduino.h>

#define SC_DEFAULT 0

struct vs23_mode_t {
  uint16_t x;
  uint16_t y;
  uint8_t top;
  uint8_t left;
  uint8_t vclkpp;
  uint8_t bextra;
};


// ntscビデオ表示クラス定義
class VS23S010 {    
  private:
	uint8_t flgExtVram; // 外部確保メモリ利用(0:利用なり 1:利用あり)
  public:
    void begin(uint8_t mode=SC_DEFAULT,uint8_t spino = 1, uint8_t* extram=NULL);  // NTSCビデオ表示開始
    void end();                            // NTSCビデオ表示終了 
    uint8_t*  VRAM();                      // VRAMアドレス取得
    void cls();                            // 画面クリア
    void delay_frame(uint16_t x);          // フレーム換算時間待ち
	void setBktmStartHook(void (*func)()); // ブランキング期間開始フック設定
    void setBktmEndHook(void (*func)());   // ブランキング期間終了フック設定

    inline uint16_t width() {
      return XPIXELS;
    }
    inline uint16_t height() {
      return YPIXELS;
    }
    uint16_t vram_size();
    uint16_t screen();
	void adjust(int16_t cnt);

    void setPixel(uint16_t x, uint16_t y, uint8_t c);
 
    void SpiRamVideoInit();
    static const uint8_t numModes;
    static const struct vs23_mode_t modes[];
};

extern VS23S010 vs23; // グローバルオブジェクト利用宣言

#endif
