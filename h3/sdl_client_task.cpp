#ifdef JAILHOUSE

// WARNING!!
// This file is compiled with -fno-short-enums and is not ABI-compatible
// with the rest of the system. Do not use data structures containing enums
// here!

#include "sdl_client.h"
#include "sdl_client_internal.h"
#include <fixed_addr.h>

struct sdl_event_buffer *evbuf = (struct sdl_event_buffer *)SDL_EVENT_BUFFER_ADDR;

void sdl_task(void) {
  while (evbuf->read_pos != evbuf->write_pos) {
    SDL_Event &ev = evbuf->events[evbuf->read_pos];
    switch (ev.type) {
    case SDL_KEYDOWN:
    case SDL_KEYUP:
      sdl_process_key_event(ev.key);
      break;
    case SDL_CONTROLLERAXISMOTION...SDL_CONTROLLERDEVICEREMAPPED:
      sdl_process_controller_event(ev);
      break;
    case SDL_MOUSEMOTION:
    case SDL_MOUSEBUTTONDOWN:
    case SDL_MOUSEBUTTONUP:
    case SDL_MOUSEWHEEL:
      sdl_process_mouse_event(ev);
    default:
      break;
    }

    evbuf->read_pos = (evbuf->read_pos + 1) % SDL_EVENT_BUFFER_SIZE;
  }
}

void sdl_client_init(void) {
    memset(evbuf, 0, sizeof(*evbuf));
}

#endif	// JAILHOUSE
