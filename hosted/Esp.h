#include <Arduino.h>

extern "C" size_t umm_free_heap_size( void );

class EspClass {
public:
  static uint32_t getCycleCount() {
    return micros() * 160UL;
  }
};

static EspClass ESP;
