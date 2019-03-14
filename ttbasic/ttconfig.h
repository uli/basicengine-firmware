//
// 豊四季Tiny BASIC for Arduino STM32 構築コンフィグレーション
// 作成日 2017/06/22 たま吉さん
//
// 修正 2017/07/29 , NTSC利用有無指定の追加
// 修正日 2017/08/06 たま吉さん, Wireライブラリ新旧対応
//

#ifndef __ttconfig_h__
#define __ttconfig_h__

// ** NTSCビデオ出力利用有無 *************************************************
#define USE_NTSC  1  // 0:利用しない 1:利用する (デフォルト:1)

#ifdef ESP8266
#define USE_VS23 1
#ifdef ESP8266_NOWIFI
#define USE_BG_ENGINE
#endif
#define PIXEL_TYPE uint8_t
#endif

#ifdef ESP32
#define USE_ESP32GFX 1
#define USE_BG_ENGINE
#define PIXEL_TYPE uint8_t
#endif

#ifdef H3
#define USE_H3GFX 1
#define USE_BG_ENGINE
#define PIXEL_TYPE uint32_t
#endif

// ** ターミナルモード時のデフォルト スクリーンサイズ  ***********************
// ※ 可動中では、WIDTHコマンドで変更可能  (デフォルト:80x25)
#define TERM_W       80
#define TERM_H       25

// ** Serial1のデフォルト通信速度 *********************************************
#define GPIO_S1_BAUD    115200 // // (デフォルト:115200)

// ** デフォルトのターミナル用シリアルポートの指定 0:USBシリアル 1:GPIO UART1
// ※ 可動中では、SMODEコマンドで変更可能
#define DEF_SMODE     0 // (デフォルト:0)

// ** 起動時にBOOT1ピン(PB2)によるシリアルターミナルモード起動選択の有無
#define FLG_CHK_BOOT1  1 // 0:なし  1:あり // (デフォルト:1)

// ** 内蔵RTCの利用指定   0:利用しない 1:利用する *****************************
#define USE_INNERRTC   1 // (デフォルト:1) ※ SDカード利用時は必ず1とする

// ** フォントデータ指定 ******************************************************
#define FONTSELECT  1  // 0 ～ 3 (デフォルト :1)

#if FONTSELECT == 0
  // 6x8 TVoutフォント
  #define TV_DISPLAY_FONT font6x8
  #include <font6x8.h>

#elif FONTSELECT == 1
  // 6x8ドット オリジナルフォント(デフォルト)
  #define TV_DISPLAY_FONT font6x8tt
  #include <font6x8tt.h>

#elif FONTSELECT == 2
  // 8x8 TVoutフォント
  #define TV_DISPLAY_FONT font8x8
  #include <font8x8.h>

#elif FONTSELECT == 3
  // 8x8 IchigoJamフォント(オプション機能 要フォント)
  #define TV_DISPLAY_FONT ichigoFont8x8 
  #include <ichigoFont8x8.h>
#endif

#define MIN_FONT_SIZE_X 6
#define MIN_FONT_SIZE_Y 8

#ifdef HAVE_PROFILE
#define NOINS __attribute__((no_instrument_function))
#endif

//#define ENABLE_GDBSTUB

#include "ati_6x8.h"
#include "amstrad_8x8.h"
#include "cbm_ascii_8x8.h"

#define SMALL __attribute__((optimize("Os")))

#ifdef H3
#define GROUP(g)
#define GROUP_DATA
#define BASIC_FP
#define BASIC_INT
#define BASIC_DAT
#elif defined(ESP32)
#define GROUP(g)
#define GROUP_DATA
#define BASIC_FP	IRAM_ATTR
#define BASIC_INT
#define BASIC_DAT
#else

#ifdef ESP8266_NOWIFI
#define GROUP(g) __attribute__((section(".irom." #g)))
#define GROUP_DATA GROUP
#define BASIC_FP ICACHE_RAM_ATTR
#else
#define GROUP(g) __attribute__((section(".text." #g)))
#define GROUP_DATA(g) __attribute__((section(".irom.text")))
#define BASIC_FP
#endif

#define BASIC_INT GROUP(basic_core)
#define BASIC_DAT GROUP_DATA(basic_data)

#endif

#ifdef ESP8266
#define UNIFILE_USE_OLD_SPIFFS
//#define UNIFILE_USE_FASTROMFS
#define UNIFILE_USE_SDFAT
#define UNIFILE_STDIO
#endif

#ifdef ESP32
#define UNIFILE_USE_NEW_SPIFFS
#define UNIFILE_USE_NEW_SD
#define UNIFILE_STDIO
#endif

#ifdef H3
#define UNIFILE_USE_OLD_SPIFFS
#define UNIFILE_USE_SDFAT
#define __FLASH__
#endif

#if !defined(ESP8266)
#define os_memcpy memcpy
#endif

#if defined(ESP8266) && !defined(ESP8266_NOWIFI)
#define HAVE_NETWORK
#endif

#ifdef ESP8266
#define PSX_DATA_PIN	0
#define PSX_CMD_PIN	1
#define PSX_ATTN_PIN	2
#define PSX_CLK_PIN	3
#define PSX_DELAY	1
#elif defined(ESP32)
#define PSX_DATA_PIN	16
#define PSX_CMD_PIN	17
#define PSX_ATTN_PIN	2
#define PSX_CLK_PIN	4
#define PSX_DELAY	1
#elif defined(H3)
// bogus
#define PSX_DATA_PIN	16
#define PSX_CMD_PIN	17
#define PSX_ATTN_PIN	2
#define PSX_CLK_PIN	4
#define PSX_DELAY	1
#endif

#ifdef UNIFILE_STDIO
#include <unifile_stdio.h>
#endif

#endif
