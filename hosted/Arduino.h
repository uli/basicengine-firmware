#ifndef _ARDUINO_H
#define _ARDUINO_H

#include <SDL/SDL.h>

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

#define PS2CLK	4
#define PS2DAT	5

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

#include <sys/time.h>
static uint32_t micros() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_usec;
}
static uint32_t millis() {
  return micros() / 1000;
}

#define delay SDL_Delay
static void delayMicroseconds(unsigned int us) {
  SDL_Delay(us / 1000);
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
void pinMode(uint8_t pin, uint8_t mode);

#include <stdlib.h>
static inline long _random(long v) {
  return random() % v;
}
#define random _random
#define randomSeed srand

#define _min(a,b) ((a)<(b)?(a):(b))
#define _max(a,b) ((a)>(b)?(a):(b))
#define min _min
#define max _max

#include "binary.h"

uint32_t timer0_read();
void timer0_write(uint32_t count);
void timer0_isr_init();
typedef void(*timercallback)(void);
void timer0_attachInterrupt(timercallback userFunc);
void timer0_detachInterrupt(void);

#include "WString.h"

static inline void yield() {
}

void hosted_pump_events();

extern uint8_t vs23_mem[];

#endif
