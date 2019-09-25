// SPDX-License-Identifier: MIT
// Copyright (c) 2018, 2019 Ulrich Hecht

#ifndef _ARDUINO_H
#define _ARDUINO_H

#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>

#include "pgmspace.h"
#define ICACHE_RAM_ATTR
#define ICACHE_RODATA_ATTR

#define HIGH 0x1
#define LOW  0x0

#define INPUT             0x00
#define INPUT_PULLUP      0x02
#define OUTPUT            0x01
#define OUTPUT_OPEN_DRAIN 0x03

#define os_memcpy memcpy

#define noInterrupts()
#define interrupts()

#define isDigit isdigit
#define isAlpha isalpha
#define isAlphaNumeric isalnum
#define isHexadecimalDigit isxdigit

typedef unsigned int word;

#define bit(b) (1UL << (b))
#define _BV(b) (1UL << (b))

typedef uint8_t boolean;
typedef uint8_t byte;

#ifdef __cplusplus
extern "C" {
#endif
uclock_t my_uclock(void);
#ifdef __cplusplus
}
#endif

static uint32_t micros() {
	return my_uclock() * 1000000 / UCLOCKS_PER_SEC;
}
static uint32_t millis() {
	return micros() / 1000;
}

#define delay(ms) delayMicroseconds((ms)*1000)
static void delayMicroseconds(unsigned int us) {
	uint32_t until = micros() + us;
	while (micros() < until) {}
}

#ifdef __cplusplus 

enum SerialConfig {
  SERIAL_8N1,
};

class HardwareSerial {
public:
  static void begin(unsigned long baud, SerialConfig c=SERIAL_8N1) {
  }
  static void println(const char *p) {
    puts(p); putchar('\n');
  }
  static void write(char c) {
    putchar(c);
  }
  static int read() {
    return -1;
  }
  static int available() {
    return 0;
  }
  static void flush() {
  }
};

static HardwareSerial Serial;

#include "Esp.h"

#endif // __cplusplus

void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
void pinMode(uint8_t pin, uint8_t mode);

#include <stdlib.h>
static inline long _random(long v) {
  return rand() % v;
}
#define random _random
#define randomSeed srand

#ifdef __cplusplus
// workaround to avoid mix-ups with our min()/max() macros
#include <memory>
#endif

#define _min(a,b) ((a)<(b)?(a):(b))
#define _max(a,b) ((a)>(b)?(a):(b))
#define min _min
#define max _max

#include "binary.h"

#include "WString.h"

void platform_process_events();

#ifdef __cplusplus
extern "C" {
#endif
void yield();

void *
memmem (const void *haystack_start,
	size_t haystack_len,
	const void *needle_start,
	size_t needle_len);
void *
memrchr (const void *src_void,
	int c,
	size_t length);
#ifdef __cplusplus
};
#endif

#define TRAC printf("%s:%d\n", __FUNCTION__, __LINE__);
#endif
