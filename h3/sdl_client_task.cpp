#ifdef JAILHOUSE

// WARNING!!
// This file is compiled with -fno-short-enums and is not ABI-compatible
// with the rest of the system. Do not use data structures containing enums
// here!

#include "sdl_client.h"
#include "sdl_client_internal.h"

struct sdl_event_buffer *evbuf = (struct sdl_event_buffer *)0x488f1000;//0x4f6f1000;

void sdl_task(void) {
  while (evbuf->read_pos != evbuf->write_pos) {
    SDL_Event &ev = evbuf->events[evbuf->read_pos];
    if (ev.type == SDL_KEYDOWN || ev.type == SDL_KEYUP) {
      sdl_process_key_event(ev.key);
    }
    evbuf->read_pos = (evbuf->read_pos + 1) % SDL_EVENT_BUFFER_SIZE;
  }
}

void sdl_client_init(void) {
    memset(evbuf, 0, sizeof(*evbuf));
}

#endif	// JAILHOUSE
