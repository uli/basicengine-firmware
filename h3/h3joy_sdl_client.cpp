// SPDX-License-Identifier: MIT
// Copyright (c) 2023 Ulrich Hecht

#ifdef JAILHOUSE

#include <joystick.h>

#include "sdl_client.h"
#include "sdl_client_internal.h"

void Joystick::begin() {
    for (int i = 0; i < PLATFORM_MAX_PADS; ++i)
        m_state[i] = 0;
    m_index.resize(0);
    m_num_pads = 0;
}

int Joystick::read(int num) {
    if (num < m_num_pads)
        return m_state[num];

    return 0;
}

int Joystick::count() {
    return m_num_pads;
}

void Joystick::addInstance(uint32_t id) {
    if (m_num_pads >= PLATFORM_MAX_PADS)
        return;

    // NB: SDL keeps counting up the ID even as devices disconnect. That
    // means that, theoretically, this vector could grow indefinitely,
    // causing us to run out of memory.
    // This could happen either through millions of disconnects and
    // reconnects, in which case the USB socket is likely to melt first, or
    // through malicious activity by the superuser in the Linux root cell.
    // The likelihood that anybody would mount such an attack is negligible,
    // though, given that by design they could arbitrarily manipulate the EB
    // cell using standard mechanisms anyway.
    // So we don't care.

    m_index.resize(std::max(m_index.size(), id + 1), -1);
    m_index[id] = m_num_pads;
    m_num_pads++;
}

void Joystick::delInstance(uint32_t id) {
    if (id < m_index.size() && m_index[id] != -1) {
        m_num_pads--;
        for (int i = m_index[id]; i < m_num_pads; ++i) {
            m_state[i] = m_state[i + 1];
        }
        m_index[id] = -1;
    }
}

static int sdl_to_ebjoy(uint8_t sdl_button) {
    switch (sdl_button) {
    case SDL_CONTROLLER_BUTTON_A:	return EB_JOY_X;
    case SDL_CONTROLLER_BUTTON_B:	return EB_JOY_O;
    case SDL_CONTROLLER_BUTTON_X:	return EB_JOY_SQUARE;
    case SDL_CONTROLLER_BUTTON_Y:	return EB_JOY_TRIANGLE;

    case SDL_CONTROLLER_BUTTON_BACK:	return EB_JOY_SELECT;
    case SDL_CONTROLLER_BUTTON_GUIDE:	return 0;
    case SDL_CONTROLLER_BUTTON_START:	return EB_JOY_START;

    // XXX: These should really be L/R3, and the triggers should be L/R2
    case SDL_CONTROLLER_BUTTON_LEFTSTICK:	return EB_JOY_L2;
    case SDL_CONTROLLER_BUTTON_RIGHTSTICK:	return EB_JOY_R2;
    case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:	return EB_JOY_L1;
    case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:	return EB_JOY_R1;

    case SDL_CONTROLLER_BUTTON_DPAD_UP:		return EB_JOY_UP;
    case SDL_CONTROLLER_BUTTON_DPAD_DOWN:	return EB_JOY_DOWN;
    case SDL_CONTROLLER_BUTTON_DPAD_LEFT:	return EB_JOY_LEFT;
    case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:	return EB_JOY_RIGHT;

    default:	return 0;
    };
}

void Joystick::buttonDown(uint32_t id, uint8_t button) {
    m_state[m_index[id]] |= sdl_to_ebjoy(button);
}

void Joystick::buttonUp(uint32_t id, uint8_t button) {
    m_state[m_index[id]] &= ~sdl_to_ebjoy(button);
}

#define AXIS_THRESHOLD 20000

void Joystick::axis(uint32_t id, uint8_t axis, int16_t value) {
    int *state = &m_state[m_index[id]];

    switch (axis) {
    case 0:
        if (value < -AXIS_THRESHOLD)
            *state |= EB_JOY_LEFT;
        else
            *state &= ~EB_JOY_LEFT;

        if (value > AXIS_THRESHOLD)
            *state |= EB_JOY_RIGHT;
        else
            *state &= ~EB_JOY_RIGHT;

        break;

    case 1:
        if (value < -AXIS_THRESHOLD)
            *state |= EB_JOY_UP;
        else
            *state &= ~EB_JOY_UP;

        if (value > AXIS_THRESHOLD)
            *state |= EB_JOY_DOWN;
        else
            *state &= ~EB_JOY_DOWN;

        break;

    default:
        break;
    }
}

Joystick joy;

void sdl_process_controller_event(SDL_Event &ev) {
    switch (ev.type) {
    case SDL_CONTROLLERDEVICEADDED:
        // NB: on plain SDL, this event gives the joystick index, a number
        // that is utterly useless. The SDL server replaces it with the
        // instance ID.
        joy.addInstance(ev.cdevice.which);
        break;

    case SDL_CONTROLLERDEVICEREMOVED:
        joy.delInstance(ev.cdevice.which);
        break;

    case SDL_CONTROLLERBUTTONDOWN:
        joy.buttonDown(ev.cbutton.which, ev.cbutton.button);
        break;

    case SDL_CONTROLLERBUTTONUP:
        joy.buttonUp(ev.cbutton.which, ev.cbutton.button);
        break;

    case SDL_CONTROLLERAXISMOTION:
        joy.axis(ev.caxis.which, ev.caxis.axis, ev.caxis.value);
        break;

    default:
        break;
    }
}

#endif	// JAILHOUSE
