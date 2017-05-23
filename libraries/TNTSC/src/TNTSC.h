// FILE: TNTSC.h
// Arduino STM32 用 NTSCビデオ出力ライブラリ by たま吉さん
// 作成日 2017/02/20, Blue Pillボード(STM32F103C8)にて動作確認
// 更新日 2017/02/27, delay_frame()の追加、
// 更新日 2017/02/27, フック登録関数追加
// 更新日 2017/03/03, 解像度モード追加
// 更新日 2017/04/05, クロック48MHz対応
// 更新日 2017/04/27, NTSC走査線数補正関数adjust()追加
// 更新日 2017/04/30, SPI1,SPI2の選択指定を可能に修正
//

#ifndef __TNTSC_H__
#define __TNTSC_H__

#include <Arduino.h>

#if F_CPU == 72000000L
	#define SC_112x108  0 // 112x108
	#define SC_224x108  1 // 224x108
	#define SC_224x216  2 // 224x216
	#define SC_448x108  3 // 448x108
	#define SC_448x216  4 // 448x216
    #define SC_DEFAULT  SC_224x216

#else if  F_CPU == 48000000L
	#define SC_128x96   0 // 128x96
	#define SC_256x96   1 // 256x96
	#define SC_256x192  2 // 256x192
	#define SC_512x96   3 // 512x96
	#define SC_512x192  4 // 512x192
	#define SC_128x108  5 // 128x108
	#define SC_256x108  6 // 256x108
	#define SC_256x216  7 // 256x216
	#define SC_512x108  8 // 512x108
	#define SC_512x216  9 // 512x216
    #define SC_DEFAULT  SC_256x192
#endif

// ntscビデオ表示クラス定義
class TNTSC_class {    
  public:
    void begin(uint8_t mode=SC_DEFAULT,uint8_t spino = 1);  // NTSCビデオ表示開始
    void end();                            // NTSCビデオ表示終了 
    uint8_t*  VRAM();                      // VRAMアドレス取得
    void cls();                            // 画面クリア
    void delay_frame(uint16_t x);          // フレーム換算時間待ち
	void setBktmStartHook(void (*func)()); // ブランキング期間開始フック設定
    void setBktmEndHook(void (*func)());   // ブランキング期間終了フック設定

    uint16_t width() ;
    uint16_t height() ;
    uint16_t vram_size();
    uint16_t screen();
	void adjust(int16_t cnt);
 
  private:
    static void handle_vout();
    static void SPI_dmaSend(uint8_t *transmitBuf, uint16_t length) ;
    static void DMA1_CH3_handle();
};

extern TNTSC_class TNTSC; // グローバルオブジェクト利用宣言

#endif
