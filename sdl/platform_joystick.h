// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#include <SDL/SDL.h>

class Joystick {
public:
	void begin();
	int read();

private:
	SDL_Joystick *m_joy;
};
