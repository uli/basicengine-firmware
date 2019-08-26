#include <TKeyboard.h>
#include "TPS2.h"

TPS2 pb;

#define KEYBUF_SIZE 8
keyEvent keybuf[KEYBUF_SIZE];
int keybuf_r = 0;
int keybuf_w = 0;

static uint8_t key_state[256];

bool TKeyboard::state(uint8_t keycode) {
  return false;
}

uint8_t TPS2::available() {
  if (keybuf_w != keybuf_r)
    return 1;
  else
    return 0;
}

keyEvent TKeyboard::read() {
  if (keybuf_w != keybuf_r) {
    keyEvent *k = &keybuf[keybuf_r];
    keybuf_r = (keybuf_r + 1) % KEYBUF_SIZE;
    return *k;
  } else {
    keyEvent k;
    memset(&k, 0, sizeof(k));
    return k;
  }
}

uint8_t TKeyboard::begin(uint8_t clk, uint8_t dat, uint8_t flgLED, uint8_t layout) {
  return 0;
}

void TKeyboard::end() {
}
