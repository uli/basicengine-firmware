// SPDX-License-Identifier: MIT
// Copyright (c) 2017-2019 Ulrich Hecht

#include "tscreenBase.h"
#include "colorspace.h"

// XXX: should this go to tTVscreen?
static pixel_t colorFromTMT(TMTATTRS *a, bool fg) {
  double scale = a->bold ? 1.33 : 1.0;

#define TCOL(r, g, b) csp.colorFromRgb((r)*scale, (g)*scale, (b)*scale)

  switch (fg ? a->fg : a->bg) {
  case TMT_COLOR_RED: return TCOL(192 * scale, 0, 0);
  case TMT_COLOR_GREEN: return TCOL(0, 192 * scale, 0);
  case TMT_COLOR_YELLOW: return TCOL(192 * scale, 192 * scale, 0);
  case TMT_COLOR_BLUE: return TCOL(0, 0, 192 * scale);
  case TMT_COLOR_MAGENTA: return TCOL(192 * scale, 0, 192 * scale);
  case TMT_COLOR_CYAN: return TCOL(0, 192 * scale, 192 * scale);
  case TMT_COLOR_WHITE: return TCOL(192, 192, 192);
  case TMT_COLOR_BLACK: return TCOL(0, 0, 0);
  case TMT_COLOR_DEFAULT:
    if (fg)
      return TCOL(192, 192, 192);
    else
      return TCOL(0, 0, 0);
  default: return 0;
  }
}

#undef TCOL

void tscreenBase::term_queue_input(const char *s) {
  while (*s) {
    vt_inbuf.push(*s++);
  }
}

void tscreenBase::term_callback(tmt_msg_t m, TMT *vt, const void *a, void *p) {
  tscreenBase *sc = (tscreenBase *)p;
  sc->term_handler(m, vt, a);
}

void tscreenBase::term_handler(tmt_msg_t m, TMT *vt, const void *a) {
  /* grab a pointer to the virtual screen */
  const TMTSCREEN *s = tmt_screen(vt);
  const TMTPOINT *c = tmt_cursor(vt);
  const char *str = (const char *)a;

  switch (m) {
  case TMT_MSG_BELL:
    /* the terminal is requesting that we ring the bell/flash the
     * screen/do whatever ^G is supposed to do; a is NULL
     */
    printf("bing!\n");
    break;

  case TMT_MSG_UPDATE:
    /* the screen image changed; a is a pointer to the TMTSCREEN */
    for (size_t r = 0; r < s->nline; r++) {
      if (s->lines[r]->dirty) {
        for (size_t c = 0; c < s->ncol; c++) {
          TMTCHAR *chr = &s->lines[r]->chars[c];

          pixel_t fg = chr->a.reverse ? colorFromTMT(&chr->a, false) :
                                        colorFromTMT(&chr->a, true);
          pixel_t bg = chr->a.reverse ? colorFromTMT(&chr->a, true) :
                                        colorFromTMT(&chr->a, false);

          VPOKE(c, r, chr->c);
          VPOKE_FGBG(c, r, fg, bg);
          WRITE_COLOR(c, r, chr->c, fg, bg);
        }
      }
    }

    /* let tmt know we've redrawn the screen */
    tmt_clean(vt);
    break;

  case TMT_MSG_ANSWER:
    /* the terminal has a response to give to the program; a is a
     * pointer to a string */
    printf("terminal answered %s\n", str);
    term_queue_input(str);
    break;

  case TMT_MSG_MOVED:
    /* the cursor moved; a is a pointer to the cursor's TMTPOINT */
    MOVE(c->r, c->c);
    break;
  case TMT_MSG_CURSOR:
    if (str[0] == 't')
      vt_cursor_on = true;
    else
      vt_cursor_on = false;

    show_curs(vt_cursor_on);
    break;
  }
}

void tscreenBase::term_putch(char c) {
  tmt_write(vt, &c, 1);
}

int tscreenBase::term_getch(void) {
  if (cursor_enabled() != vt_cursor_on) {
    show_curs(vt_cursor_on);
  }

  if (vt_inbuf.empty()) {
    int c = get_ch();
    switch (c) {
    case SC_KEY_DOWN: term_queue_input(TMT_KEY_DOWN); break;
    case SC_KEY_UP: term_queue_input(TMT_KEY_UP); break;
    case SC_KEY_LEFT: term_queue_input(TMT_KEY_LEFT); break;
    case SC_KEY_RIGHT: term_queue_input(TMT_KEY_RIGHT); break;
    case SC_KEY_HOME: term_queue_input(TMT_KEY_HOME); break;
    case SC_KEY_END: term_queue_input(TMT_KEY_END); break;
    case SC_KEY_NPAGE: term_queue_input(TMT_KEY_PAGE_DOWN); break;
    case SC_KEY_PPAGE: term_queue_input(TMT_KEY_PAGE_UP); break;
    case SC_KEY_F(1): term_queue_input(TMT_KEY_F1); break;
    case SC_KEY_F(2): term_queue_input(TMT_KEY_F2); break;
    case SC_KEY_F(3): term_queue_input(TMT_KEY_F3); break;
    case SC_KEY_F(4): term_queue_input(TMT_KEY_F4); break;
    case SC_KEY_F(5): term_queue_input(TMT_KEY_F5); break;
    case SC_KEY_F(6): term_queue_input(TMT_KEY_F6); break;
    case SC_KEY_F(7): term_queue_input(TMT_KEY_F7); break;
    case SC_KEY_F(8): term_queue_input(TMT_KEY_F8); break;
    case SC_KEY_F(9): term_queue_input(TMT_KEY_F9); break;
    case SC_KEY_F(10): term_queue_input(TMT_KEY_F10); break;
    default: vt_inbuf.push(c); break;
    }
  }
  int c = vt_inbuf.front();
  vt_inbuf.pop();
  return c;
}
