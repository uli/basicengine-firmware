//
// 豊四季Tiny BASIC for Arduino STM32 構築コンフィグレーション
// 作成日 2017/06/22 たま吉さん
//
// 修正 2017/07/29 , NTSC利用有無指定の追加
// 修正日 2017/08/06 たま吉さん, Wireライブラリ新旧対応
//

#ifndef __ttconfig_h__
#define __ttconfig_h__

// ** Use of NTSC video output *************************************************
// XXX: This is non-optional by now and should thus be removed.
#define USE_NTSC  1  // 0: Not used, 1: Used. (Default: 1)

#ifdef H3
#define USE_H3GFX 1
#define USE_BG_ENGINE
#define PIXEL_TYPE uint32_t
#define IPIXEL_TYPE uint32_t
#define TRUE_COLOR
#define BUFFERED_SCREEN
#define SINGLE_BLIT
#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_16BIT
#define USE_ROTOZOOM
#define HAVE_NETWORK
#define HAVE_TFTP
#ifdef JAILHOUSE
#define HAVE_MULTI_PAD
#define HAVE_SCREEN_RECORDING
#endif
#endif

#ifdef __DJGPP__
#define USE_DOSGFX 1
#define USE_BG_ENGINE
#define PIXEL_TYPE uint8_t
#define BUFFERED_SCREEN
#endif

#ifdef SDL

#define SDL_BPP 32

#define USE_SDLGFX 1
#define USE_BG_ENGINE
#if SDL_BPP == 8
#define PIXEL_TYPE uint8_t
#elif SDL_BPP == 16
#define PIXEL_TYPE uint16_t
#else
#define PIXEL_TYPE uint32_t
#define TRUE_COLOR
#define IPIXEL_TYPE uint32_t
#endif
#define USE_ROTOZOOM
#define BUFFERED_SCREEN
#define SINGLE_BLIT
#define AUDIO_SAMPLE_RATE 48000
#define AUDIO_16BIT
#endif

#ifndef IPIXEL_TYPE
#define IPIXEL_TYPE uint8_t
#endif

#ifdef LOWMEM
#define TOKEN_TYPE uint8_t
#else
#define TOKEN_TYPE uint32_t
#endif

#ifdef TRUE_COLOR
#define DEFAULT_COLORSPACE 2
#else
#define DEFAULT_COLORSPACE 0
#endif

// ** Default screen size in terminal mode ************************
// ※ While moving, can be changed by WIDTH command (default: 80x25)
// XXX: I don't think this works anymore.
#define TERM_W       80
#define TERM_H       25

// ** Default speed of Serial port 1 *********************************************
#define GPIO_S1_BAUD    115200

#ifdef HAVE_PROFILE
#define NOINS __attribute__((no_instrument_function))
#endif

//#define ENABLE_GDBSTUB

#ifdef USE_SMALL
#define SMALL __attribute__((optimize("Os")))
#else
#define SMALL
#endif

#ifdef NO_JUMP_TABLES
#define NOJUMP __attribute__((optimize("no-jump-tables")))
#else
#define NOJUMP
#endif

#if defined(H3) || defined(__DJGPP__)
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
#define GROUP(g)   __attribute__((section(".irom." #g)))
#define GROUP_DATA GROUP
#define BASIC_FP   ICACHE_RAM_ATTR
#else
#define GROUP(g)      __attribute__((section(".text." #g)))
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
#ifndef HOSTED
#define UNIFILE_STDIO
#endif
#endif

#ifdef ESP32
#define UNIFILE_USE_NEW_SPIFFS
#define UNIFILE_USE_NEW_SD
#define UNIFILE_STDIO
#endif

#ifdef H3
#define SD_PREFIX "/sd"
#define __FLASH__
#endif

#ifdef __DJGPP__
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
#elif defined(H3) || defined(__DJGPP__) || defined(SDL)
// bogus
#define PSX_DATA_PIN	16
#define PSX_CMD_PIN	17
#define PSX_ATTN_PIN	2
#define PSX_CLK_PIN	4
#define PSX_DELAY	1
#endif

#ifdef UNIFILE_STDIO
#include <unifile_stdio.h>
#else
#define _opendir  opendir
#define _closedir closedir
#define _readdir  readdir
#define _remove   remove
#define _rename   rename
#define _chdir    chdir
#define _getcwd   getcwd
#define _stat     stat
#define _mkdir    mkdir
#define _rmdir    rmdir
#endif

const char *gettext(const char *msg);
#define _(s) gettext(s)

#define NUM_LANGS 5

#endif
