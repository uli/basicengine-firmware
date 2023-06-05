// SPDX-License-Identifier: MIT
// Copyright (c) 2023 Ulrich Hecht

#ifdef JAILHOUSE

#include <mouse.h>

#include "sdl_client.h"
#include "sdl_client_internal.h"

Mouse mouse;

void sdl_process_mouse_event(SDL_Event &ev)
{
    switch (ev.type) {
    case SDL_MOUSEMOTION:
        mouse.move(ev.motion.xrel * eb_psize_width() / 600.0,
                   ev.motion.yrel * eb_psize_width() / 600.0);
        break;
    case SDL_MOUSEBUTTONDOWN:
        mouse.setButtons(mouse.buttons() | (1 << (ev.button.button - SDL_BUTTON_LEFT)));
        break;
    case SDL_MOUSEBUTTONUP:
        mouse.setButtons(mouse.buttons() & ~(1 << (ev.button.button - SDL_BUTTON_LEFT)));
        break;
    case SDL_MOUSEWHEEL:
        mouse.updateWheel(ev.wheel.y);
        break;
    default:
        break;
    }
}

#endif	// JAILHOUSE
