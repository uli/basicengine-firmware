#ifndef __BASIC_H
#define __BASIC_H

#include <stdint.h>
#include <string.h>

#include <tTVscreen.h>

#include "variable.h"

#define SIZE_LINE 128    // コマンドライン入力バッファサイズ + NULL
#define SIZE_IBUF 128    // 中間コード変換バッファサイズ

extern char lbuf[SIZE_LINE];
extern char tbuf[SIZE_LINE];
extern int32_t tbuf_pos;
extern unsigned char ibuf[SIZE_IBUF];

extern unsigned char *listbuf;

extern uint8_t err; // Error message index

uint8_t toktoi(bool find_prg_text = true);
void putlist(unsigned char* ip, uint8_t devno=0);

// メモリ書き込みポインタのクリア
static inline void cleartbuf() {
  tbuf_pos=0;
  memset(tbuf,0,SIZE_LINE);
}

void c_putch(uint8_t c, uint8_t devno = 0);
void c_puts(const char *s, uint8_t devno=0);
void c_puts_P(const char *s, uint8_t devno=0);

void putnum(num_t value, int8_t d, uint8_t devno=0);

void newline(uint8_t devno=0);

BString getParamFname();

void get_input(bool numeric = false);

#ifdef ESP8266
extern "C" size_t umm_free_heap_size( void );
#endif

#include <tscreenBase.h>

extern tTVscreen sc0;

#endif
