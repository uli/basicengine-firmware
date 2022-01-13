// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#include <joystick.h>
#include <SDL2/SDL.h>
#include <list>
#include <queue>

std::queue<SDL_Event> controller_events;
std::list<int> controllers;

static uint32_t button_bit[SDL_CONTROLLER_BUTTON_MAX + 1] = {
  EB_JOY_X,
  EB_JOY_O,
  EB_JOY_SQUARE,
  EB_JOY_TRIANGLE,
  EB_JOY_SELECT,
  0,
  EB_JOY_START,
  EB_JOY_L3,
  EB_JOY_R3,
  EB_JOY_L1,
  EB_JOY_R1,
  EB_JOY_UP,
  EB_JOY_DOWN,
  EB_JOY_LEFT,
  EB_JOY_RIGHT,
};

int Joystick::read() {
  static int ret = 0;

  if (controller_events.empty())
    return ret;

  while (!controller_events.empty()) {
    SDL_Event event = controller_events.front();
    controller_events.pop();
    switch (event.type) {
    case SDL_CONTROLLERBUTTONDOWN:
      break;
    case SDL_CONTROLLERDEVICEADDED:
      SDL_GameControllerOpen(event.cdevice.which);
      controllers.push_back(event.cdevice.which);
      break;
    case SDL_CONTROLLERDEVICEREMOVED:
      SDL_GameControllerClose(SDL_GameControllerFromInstanceID(event.cdevice.which));
      controllers.remove(event.cdevice.which);
      break;
    default:
      break;
    }
  }

  ret = 0;

  for (auto joy_idx : controllers) {
    SDL_GameController *controller = SDL_GameControllerFromInstanceID(joy_idx);
    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
      if (SDL_GameControllerGetButton(controller, (SDL_GameControllerButton)i)) {
        ret |= button_bit[i];
      }
    }

    int x = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
    if (x < -16384)
      ret |= EB_JOY_LEFT;
    else if (x > 16384)
      ret |= EB_JOY_RIGHT;

    int y = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
    if (y < -16384)
      ret |= EB_JOY_UP;
    else if (y > 16384)
      ret |= EB_JOY_DOWN;
  }

  return ret;
}

void Joystick::begin() {
}

Joystick joy;
