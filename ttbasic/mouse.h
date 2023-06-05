// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#ifndef _MOUSE_H
#define _MOUSE_H

#include <ttconfig.h>
#include <math.h>

#include "eb_input.h"
#include "eb_video.h"

class Mouse {
public:
  int absX() { return m_x; }
  int absY() { return m_y; }
  double relX() { double tmp = m_dx; m_dx = 0; return tmp; }
  double relY() { double tmp = m_dy; m_dy = 0; return tmp; }
  int wheel() { int tmp = m_dwheel; m_dwheel = 0; return tmp; }
  int buttons() { return m_buttons; }

  void move(double dx, double dy) {
    m_dx += dx;
    m_dy += dy;
    m_x = fmax(0, fmin(eb_psize_width()  - 1, (m_x + dx)));
    m_y = fmax(0, fmin(eb_psize_height() - 1, (m_y + dy)));

  }

  void warp(int x, int y) {
    m_x = x;
    m_y = y;
  }

  void setButtons(int buttons) {
    m_buttons = buttons;
  }

  void updateWheel(int dwheel) {
    m_dwheel += dwheel;
  }

private:
  int m_buttons;
  double m_x, m_y;
  double m_dx, m_dy;
  int m_dwheel;
};

extern Mouse mouse;

#endif	// _MOUSE_H
