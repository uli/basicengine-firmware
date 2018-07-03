#include <TKeyboard.h>
#include "TPS2.h"
#include <SDL/SDL.h>
#include <ring_buffer.h>

TPS2 pb;

bool TKeyboard::state(uint8_t keycode) {
  const Uint8 *state = SDL_GetKeyState(NULL);
  return state[keycode];	// XXX: translate!
}

uint8_t TPS2::available() {
  SDL_Event events[8];
  SDL_PumpEvents();
  return SDL_PeepEvents(events, 8, SDL_PEEKEVENT, SDL_KEYUPMASK);
}

keyEvent TKeyboard::read() {
  keyinfo ki;
  ki.value = 0;
  SDL_Event event;
  SDL_PumpEvents();
  while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_KEYUPMASK|SDL_KEYDOWNMASK) == 1) {
    switch (event.type) {
    case SDL_KEYUP:
      ki.kevt.code = event.key.keysym.unicode;
      ki.kevt.BREAK = 1;
      return ki.kevt;
    case SDL_KEYDOWN:
      ki.kevt.code = event.key.keysym.unicode;
      return ki.kevt;
    default:
      abort();
    }
    SDL_PumpEvents();
  }
  return ki.kevt;
}

uint8_t TKeyboard::begin(uint8_t clk, uint8_t dat, uint8_t flgLED, uint8_t layout) {
}

void TKeyboard::end() {
}
