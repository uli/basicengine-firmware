#ifndef _PS22TTY_H
#define _PS22TTY_H

#include <TKeyboard.h>
#include <TPS2.h>
#include "ring_buffer.h"

extern TPS2 pb;
extern struct ring_buffer kbuf;

uint8_t ICACHE_RAM_ATTR ps2read();
uint8_t ICACHE_RAM_ATTR ps2peek();

static inline bool ICACHE_RAM_ATTR ps2kbhit() {
  if (!pb.available() && rb_is_empty(&kbuf))
    return false;
  return true;
}

extern keyEvent ps22tty_last_key;

static inline keyEvent ICACHE_RAM_ATTR ps2last() {
  return ps22tty_last_key;
}
#endif
