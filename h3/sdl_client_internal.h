#include JAILHOUSE_SDL_HEADER

#define USE_CUSTOM_SDL_HEADERS
#include <sdl_server.h>
#undef USE_CUSTOM_SDL_HEADERS

void sdl_process_key_event(SDL_KeyboardEvent &ev);
void sdl_process_controller_event(SDL_Event &ev);
