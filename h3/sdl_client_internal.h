// XXX: make this configurable
#include "../../buildroot_jh/output/host/arm-buildroot-linux-gnueabihf/sysroot/usr/include/SDL2/SDL_events.h"

#define SDL_EVENT_BUFFER_SIZE 128

struct sdl_event_buffer {
    int read_pos;
    int write_pos;
    SDL_Event events[SDL_EVENT_BUFFER_SIZE];
};

void sdl_process_key_event(SDL_KeyboardEvent &ev);
