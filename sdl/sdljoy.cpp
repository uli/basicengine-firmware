// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#include <joystick.h>
#include <SDL/SDL.h>

int Joystick::read()
{
  if (!m_joy)
    return 0;

  int ret = 0;

  // Joystick API supports max. 10 buttons.
  int buttons = SDL_JoystickNumButtons(m_joy);
  if (buttons > 10)
    buttons = 10;

  for (int i = 0; i < SDL_JoystickNumButtons(m_joy); ++i) {
    // Map SDL button to PSX button.
    int internal_button;
    if (i < 8)
      internal_button = i + joySquShift;
    else
      internal_button = i - 8 + joyStrtShift;

    if (SDL_JoystickGetButton(m_joy, i))
      ret |= 1 << internal_button;
  }

  // Only two digital axes supported ATM.
  int axes = SDL_JoystickNumAxes(m_joy);

  if (axes >= 2) {
    int x = SDL_JoystickGetAxis(m_joy, 0);
    int y = SDL_JoystickGetAxis(m_joy, 1);

    if (x < -16384)
      ret |= joyLeft;
    else if (x > 16384)
      ret |= joyRight;

    if (y < -16384)
      ret |= joyUp;
    else if (y > 16384)
      ret |= joyDown;
  }

  return ret;
}

void Joystick::begin()
{
  m_joy = NULL;

  if (SDL_NumJoysticks() == 0)
    return;

  m_joy = SDL_JoystickOpen(0);
}

Joystick joy;
