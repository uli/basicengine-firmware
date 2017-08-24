//
// 豊四季Tiny BASIC コンパイル設定
// 作成日 2017/06/22 たま吉さん
// 修正日 2017/08/06 たま吉さん, Wireライブラリ新旧対応
//

#ifndef __ttconfig_h__
#define __ttconfig_h__

// Wireライブラリ新旧指定対応 0:新 1:旧(R20170323相当)
//#define OLD_WIRE_LIB   1

// I2Cライブラリの選択 0:Wire(ソフトエミュレーション) 1:HWire 
#define I2C_USE_HWIRE  1

// 内蔵RTCの利用指定   0:利用しない 1:利用する
//#define USE_INNERRTC   1 

// SDカードの利用      0:利用しない 1:利用する
//#define USE_SD_CARD    1 

// PS/2キーボードの利用 0:利用しない 1:利用する
#define PS2DEV         1     

#endif

