// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "basic.h"
#include "mouse.h"
#include "eb_input.h"
#include "eb_api.h"

EBAPI int eb_inkey(void) {
  int32_t rc = 0;

  if (c_kbhit()) {
    rc = sc0.tryGetChar();
    process_hotkeys(rc);
  }
  return rc;
}

#ifdef USE_PSX_GPIO
#define JOY_MASK 0xffff
#else
#define JOY_MASK 0x3ffff
#endif

static int cursor_pad_state() {
  // The state is kept up-to-date by the interpreter polling for Ctrl-C.
  return kb.state(PS2KEY_L_Arrow) << EB_JOY_LEFT_SHIFT |
         kb.state(PS2KEY_R_Arrow) << EB_JOY_RIGHT_SHIFT |
         kb.state(PS2KEY_Down_Arrow) << EB_JOY_DOWN_SHIFT |
         kb.state(PS2KEY_Up_Arrow) << EB_JOY_UP_SHIFT |
         kb.state(PS2KEY_X) << EB_JOY_X_SHIFT | kb.state(PS2KEY_A) << EB_JOY_TRIANGLE_SHIFT |
         kb.state(PS2KEY_S) << EB_JOY_O_SHIFT | kb.state(PS2KEY_Z) << EB_JOY_SQUARE_SHIFT |
         kb.state(PS2KEY_Tab) << EB_JOY_SELECT_SHIFT | kb.state(PS2KEY_Enter) << EB_JOY_START_SHIFT;
}

EBAPI int eb_pad_state(int num) {
  switch (num) {
  case 0: return (joy.read() & JOY_MASK) | cursor_pad_state();
  case 1: return cursor_pad_state();
  case 2: return joy.read() & JOY_MASK;
  }
  return 0;
}

EBAPI int eb_key_state(int scancode) {
  if (check_param(scancode, 0, 255))
    return -1;
  return kb.state(scancode);
}

EBAPI int eb_mouse_abs_x(void) {
  return mouse.absX();
}
EBAPI int eb_mouse_abs_y(void) {
  return mouse.absY();
}

EBAPI double eb_mouse_rel_x(void) {
  return mouse.relX();
}
EBAPI double eb_mouse_rel_y(void) {
  return mouse.relY();
}

EBAPI int eb_mouse_button(int n) {
  return !!(mouse.buttons() & (1 << n));
}

EBAPI int eb_mouse_buttons(void) {
  return mouse.buttons();
}

EBAPI int eb_mouse_wheel(void) {
  return mouse.wheel();
}

EBAPI void eb_mouse_warp(int x, int y) {
  mouse.warp(x, y);
}
