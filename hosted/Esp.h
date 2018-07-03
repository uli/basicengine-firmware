#include <Arduino.h>

class EspClass {
public:
  static uint32_t getCycleCount() {
    return micros() * 160UL;
  }
};

static EspClass ESP;
