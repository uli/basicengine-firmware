// SPDX-License-Identifier: MIT
// Copyright (c) 2018, 2019 Ulrich Hecht

#include "Arduino.h"
#include "FS.h"
#include "SPI.h"
#include <malloc.h>
#include <usb.h>
#include <fs.h>

void digitalWrite(uint8_t pin, uint8_t val) {
  printf("DW %d <- %02X\n", pin, val);
}

int digitalRead(uint8_t pin) {
  return 0;
}

void pinMode(uint8_t pin, uint8_t mode) {
}

uint32_t timer0_read() {
  return 0;
}
void timer0_write(uint32_t count) {
}

void timer0_isr_init() {
}
void timer0_attachInterrupt(timercallback userFunc) {
}
void timer0_detachInterrupt(void) {
}

fs::FS SPIFFS;
SPIClass SPI;

void loop();
void setup();

struct palette {
  uint8_t r, g, b;
};
#include <N-0C-B62-A63-Y33-N10.h>
#include <P-EE-A22-B22-Y44-N10.h>

static void my_exit(void) {
  fprintf(stderr, "my_exit\n");
}

int main(int argc, char **argv) {
  atexit(my_exit);
  setup();
  for (;;)
    loop();
}

#include "border_pal.h"

uint64_t total_frames = 0;
extern uint64_t total_samples;
extern int sound_reinit_rate;

void kbd_task(void);
extern "C" void network_task(void);

void platform_process_events() {
  usb_task();
  sd_detect();
  kbd_task();
  network_task();
}

extern "C" size_t umm_free_heap_size(void) {
  return 42000;
}

#include "Wire.h"
TwoWire Wire;
