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
