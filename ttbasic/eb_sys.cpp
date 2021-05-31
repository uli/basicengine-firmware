// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "eb_sys.h"
#include "basic.h"

void eb_wait(unsigned int ms) {
  unsigned end = ms + millis();
  while (millis() < end) {
    process_events();
    uint16_t c = sc0.peekKey();
    if (process_hotkeys(c)) {
      break;
    }
    yield();
  }
}

unsigned int eb_tick(void) {
  return millis();
}

void eb_udelay(unsigned int us) {
  return delayMicroseconds(us);
}

// NB: process_events() is directly exported to native modules as
// eb_process_events(), so changing this will not necessarily have the
// desired effect.
void eb_process_events(void) {
  process_events();
}

void eb_set_cpu_speed(int percent) {
#ifdef H3
  int factor;

  if (percent < 0)
    factor = SYS_CPU_MULTIPLIER_DEFAULT;
  else
    factor = SYS_CPU_MULTIPLIER_MAX * percent / 100;

  sys_set_cpu_multiplier(factor);
#else
  // unsupported, ignore silently
#endif
}
