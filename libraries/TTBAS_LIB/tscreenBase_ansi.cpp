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

void tscreenBase::term_callback(tmt_msg_t m, TMT *vt, const void *a, void *p) {
  tscreenBase *sc = (tscreenBase *)p;
  /* grab a pointer to the virtual screen */
  const TMTSCREEN *s = tmt_screen(vt);
  const TMTPOINT *c = tmt_cursor(vt);

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

          sc->WRITE_COLOR(c, r, chr->c, fg, bg);
        }
      }
    }

    /* let tmt know we've redrawn the screen */
    tmt_clean(vt);
    break;

  case TMT_MSG_ANSWER:
    /* the terminal has a response to give to the program; a is a
     * pointer to a string */
    printf("terminal answered %s\n", (const char *)a);
    break;

  case TMT_MSG_MOVED:
    /* the cursor moved; a is a pointer to the cursor's TMTPOINT */
    sc->MOVE(c->r, c->c);
    break;
  case TMT_MSG_CURSOR:
    if (((const char *)a)[0] == 't')
      sc->show_curs(1);
    else
      sc->show_curs(0);
    break;
  }
}

void tscreenBase::term_putch(char c) {
  tmt_write(vt, &c, 1);
}
