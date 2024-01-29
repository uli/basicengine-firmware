// SPDX-License-Identifier: MIT
// Copyright (c) 2018, 2019 Ulrich Hecht

#ifndef _ARDUINO_H
#define _ARDUINO_H

#include <SDL.h>

#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stdint.h>
#include <limits.h>

#include "pgmspace.h"
#define ICACHE_RAM_ATTR
#define ICACHE_RODATA_ATTR

#define HIGH 0x1
#define LOW  0x0

#define INPUT             0x00
#define INPUT_PULLUP      0x02
#define OUTPUT            0x01
#define OUTPUT_OPEN_DRAIN 0x03

//#define PS2CLK	4
//#define PS2DAT	5

#define os_memcpy memcpy

#define noInterrupts()
#define interrupts()

#define isDigit            isdigit
#define isAlpha            isalpha
#define isAlphaNumeric     isalnum
#define isHexadecimalDigit isxdigit

typedef unsigned int word;

#define bit(b) (1UL << (b))
#define _BV(b) (1UL << (b))

typedef uint8_t boolean;
typedef uint8_t byte;

#include <sys/time.h>
static uint32_t micros() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000000UL + tv.tv_usec;
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
  static void begin(unsigned long baud, SerialConfig c = SERIAL_8N1) {
  }
  static void println(const char *p) {
    puts(p);
    putchar('\n');
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

struct vs23_int {
  uint16_t vdctrl1, vdctrl2;
  uint16_t write_status;
  uint16_t picstart;
  uint16_t picend;
  uint16_t linelen;
  uint16_t indexstart;

  int line_count;
  bool enabled;
  bool pal;
  int plen;
  double line_us;
};
extern struct vs23_int vs23_int;

#endif  // __cplusplus

void digitalWrite(uint8_t pin, uint8_t val);
int digitalRead(uint8_t pin);
void pinMode(uint8_t pin, uint8_t mode);

#include <stdlib.h>
static inline long _random(long v) {
  return random() % v;
}
#define random     _random
#define randomSeed srand

#ifdef __cplusplus
// workaround to avoid mix-ups with our min()/max() macros
#include <memory>
#include <queue>
#endif

#define _min(a, b) ((a) < (b) ? (a) : (b))
#define _max(a, b) ((a) > (b) ? (a) : (b))

#include "binary.h"

uint32_t timer0_read();
void timer0_write(uint32_t count);
void timer0_isr_init();
typedef void (*timercallback)(void);
void timer0_attachInterrupt(timercallback userFunc);
void timer0_detachInterrupt(void);

#include "WString.h"
#include <unistd.h>
static inline void yield() {
  usleep(1000);
}

void platform_process_events();

extern uint8_t vs23_mem[];

#endif
