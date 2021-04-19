// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "basic.h"
#include "eb_input.h"
#include "eb_api.h"

int eb_inkey(void) {
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
  return kb.state(PS2KEY_L_Arrow) << joyLeftShift |
         kb.state(PS2KEY_R_Arrow) << joyRightShift |
         kb.state(PS2KEY_Down_Arrow) << joyDownShift |
         kb.state(PS2KEY_Up_Arrow) << joyUpShift |
         kb.state(PS2KEY_X) << joyXShift | kb.state(PS2KEY_A) << joyTriShift |
         kb.state(PS2KEY_S) << joyOShift | kb.state(PS2KEY_Z) << joySquShift;
}

int eb_pad_state(int num) {
  switch (num) {
  case 0: return (joy.read() & JOY_MASK) | cursor_pad_state();
  case 1: return cursor_pad_state();
  case 2: return joy.read() & JOY_MASK;
  }
  return 0;
}

int eb_key_state(int scancode) {
  if (check_param(scancode, 0, 255))
    return -1;
  return kb.state(scancode);
}
