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
