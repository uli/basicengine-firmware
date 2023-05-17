// SPDX-License-Identifier: MIT
// Copyright (c) 2017-2019 Ulrich Hecht

#include "tscreenBase.h"

void tscreenBase::term_queue_input(const char *s) {
  while (*s) {
    vt_inbuf.push(*s++);
  }
}

extern void screen_putch(utf8_int32_t c, bool lazy = false);

void tscreenBase::term_putch(char c) {
  static char utf8buf[6] = { 0 };
  char buf[2] = { c, 0 };

  strcat(utf8buf, buf);
  if (!utf8nvalid(utf8buf, 5)) {
    // valid UTF-8 string
    utf8_int32_t cp;
    utf8codepoint(utf8buf, &cp);
    screen_putch(cp);
    utf8buf[0] = 0;
  } else if ((buf[0] & 0x80) == 0) {
    screen_putch('?');
    screen_putch(buf[0]);
    utf8buf[0] = 0;
  } else if (strlen(utf8buf) >= 5) {
    screen_putch('?');
    utf8buf[0] = 0;
  }
}

int tscreenBase::term_getch(void) {
  if (vt_inbuf.empty()) {
    int c = get_ch(true);
    if (c == 0)
      return -1;

    switch (c) {
    case SC_KEY_DOWN: term_queue_input("\x1b[B"); break;
    case SC_KEY_UP: term_queue_input("\x1b[A"); break;
    case SC_KEY_LEFT: term_queue_input("\x1b[D"); break;
    case SC_KEY_RIGHT: term_queue_input("\x1b[C"); break;
    case SC_KEY_HOME: term_queue_input("\x1b[H"); break;
    case SC_KEY_END: term_queue_input("\x1b[4~"); break;
    case SC_KEY_NPAGE: term_queue_input("\x1b[G"); break;
    case SC_KEY_PPAGE: term_queue_input("\x1b[I"); break;
    case SC_KEY_F(1): term_queue_input("\x1b[M"); break;
    case SC_KEY_F(2): term_queue_input("\x1b[N"); break;
    case SC_KEY_F(3): term_queue_input("\x1b[O"); break;
    case SC_KEY_F(4): term_queue_input("\x1b[P"); break;
    case SC_KEY_F(5): term_queue_input("\x1b[Q"); break;
    case SC_KEY_F(6): term_queue_input("\x1b[R"); break;
    case SC_KEY_F(7): term_queue_input("\x1b[S"); break;
    case SC_KEY_F(8): term_queue_input("\x1b[T"); break;
    case SC_KEY_F(9): term_queue_input("\x1b[U"); break;
    case SC_KEY_F(10): term_queue_input("\x1b[V"); break;
    case SC_KEY_DC: term_queue_input("\x1b[3~"); break;
    case SC_KEY_BACKSPACE: term_queue_input("\x7f"); break;
    default: {
        char tmp[5];
        char *end = (char *)utf8catcodepoint(tmp, c, 5);
        *end = 0;
        term_queue_input(tmp);
      }
      break;
    }
  }
  int c = (unsigned char)vt_inbuf.front();
  vt_inbuf.pop();
  return c;
}
