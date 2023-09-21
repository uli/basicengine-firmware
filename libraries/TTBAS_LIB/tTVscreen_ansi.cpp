// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Ulrich Hecht

#include "basic.h"
#include "tTVscreen.h"

extern bool screen_putch_enable_reverse;

enum ansi_state {
  S_NUL = 0,
  S_ESC,
  S_ARG,
  S_QUEST,
  S_EQUAL,
};

#define PAR_MAX 4
struct {
  enum ansi_state state;
  int npar;
  int pars[PAR_MAX];
  int arg;
  bool ignored;
} vt;

static void consumearg() {
  if (vt.npar < PAR_MAX)
      vt.pars[vt.npar++] = vt.arg;
  vt.arg = 0;
}

static void resetparser() {
  memset(vt.pars, 0, sizeof(vt.pars));
  vt.state = S_NUL; vt.npar = vt.arg = 0; vt.ignored = false;
}

#define P0(x) (vt.pars[x])
#define P1(x) (vt.pars[x]? vt.pars[x] : 1)

void tTVscreen::sgr() {
  #define FGBG(c) \
    if (P0(i) < 40) \
      setColor(COL(c), getBgColor()); \
    else \
      setColor(getFgColor(), COL(c));
  #define SGR_UNIMP printf("ANSI: unimplemented sgr %d\n", P0(i))

  for (size_t i = 0; i < vt.npar; i++) {
    switch (P0(i)){
    case  0:	// reset
      if (screen_putch_enable_reverse) {
        screen_putch_enable_reverse = false;
        flipColors();
      }
      setColor(COL(FG), COL(BG));
      break;
    case  1: case 22: /*SGR_UNIMP;*/ break; // XXX: bold
    case  2: case 23: /*SGR_UNIMP;*/ break; // XXX: dim
    case  4: case 24: /*SGR_UNIMP;*/ break; // XXX: underline
    case  5: case 25: /*SGR_UNIMP;*/ break; //vt->attrs.blink     = P0(0) < 20; break;
    case  7:
      if (!screen_putch_enable_reverse) {
        screen_putch_enable_reverse = true;
        flipColors();
      }
      break;
    case 27:
      if (screen_putch_enable_reverse) {
        screen_putch_enable_reverse = false;
        flipColors();
      }
      break;
    case  8: case 28: SGR_UNIMP; break;//vt->attrs.invisible = P0(0) < 20; break;
    case 10: break;	// no need to warn about that if 11/12 is not implemented either
    case 11: SGR_UNIMP; break;//vt->acs             = P0(0) > 10; break;
    case 30: case 40: FGBG(BG);            break;
    case 31: case 41: FGBG(PROC);              break;
    case 32: case 42: FGBG(COMMENT);            break;
    case 33: case 43: FGBG(FG); /* XXX: yellow */           break;
    case 34: case 44: FGBG(STR);             break;
    case 35: case 45: FGBG(OP);          break;
    case 36: case 46: FGBG(NUM);             break;
    case 37: case 47: FGBG(FG);            break;
    case 39:	// default FG
      setColor(COL(FG), getBgColor());
      break;
    case 49:	// default BG
      setColor(getFgColor(), COL(BG));
      break;
    default:
      printf("ANSI: unknown sgr %d\n", P0(i));
      break;
    }
  }
}

void tTVscreen::ed() {
    #define ED_UNIMP printf("ANSI: unimplemented ed %d\n", P0(0))

    switch (P0(0)){
        case 0:		// clear from cursor
          clerLine(c_y(), c_x());
          for (int i = c_y() + 1; i < getHeight(); ++i)
            clerLine(i);
          break;
        case 1: ED_UNIMP; break;	// XXX: clear to cursor
        case 2:		// clear screen
          cls();
          break;
        default:
          printf("ANSI: unknown ed %d\n", P0(0));
          return;
    }
}

void tTVscreen::el() {
  #define EL_UNIMP printf("ANSI: unimplemented el %d\n", P0(0))

  switch (P0(0)){
      case 0: clerLine(c_y(), c_x()); break;
      case 1: {
        int x = c_x();
        locate(0, c_y());
        for (int i = 0; i < x; i++)
          putch(' ');
        locate(x, c_y());
      }
      break; // clear line to cursor
      case 2: clerLine(c_y()); break;
  }
}

bool tTVscreen::ansi_machine(utf8_int32_t i)
{
#ifdef DEBUG_ANSI
    if (i < 32)
      printf("^%c", i + 64);
    else if (i >= 128)
      printf("[u%04X]", i);
    else
      printf("%c", i);
#endif
    if (i >= 256)	// or even 128?
      return false;

    char cs[] = {(char)i, 0};

    #define ON(S, C, A) if (vt.state == (S) && strchr(C, i)){ A; return true;}
    #define DO(S, C, A) ON(S, C, consumearg(); if (!vt.ignored) {A;} \
                                 /*fixcursor(vt);*/ resetparser(););

    DO(S_NUL, "\x07",       true)//CB(vt, TMT_MSG_BELL, NULL))
    DO(S_NUL, "\x08",       locate(c_x() - 1, c_y()))
    ON(S_NUL, "\x1b",       vt.state = S_ESC)
    ON(S_ESC, "\x1b",       vt.state = S_ESC)
    DO(S_ESC, "H",          locate(0, 0))
    ON(S_ESC, "[",          vt.state = S_ARG)
    ON(S_ARG, "\x1b",       vt.state = S_ESC)
    ON(S_QUEST, "\x1b",       vt.state = S_ESC)
    ON(S_EQUAL, "\x1b",       vt.state = S_ESC)
    ON(S_ARG, ";",          consumearg())
    ON(S_QUEST, ";",          consumearg())
    ON(S_EQUAL, ";",          consumearg())
    ON(S_ARG, "?",          vt.state = S_QUEST)
    ON(S_ARG, "=",	    vt.state = S_EQUAL)
    ON(S_ARG, "0123456789", vt.arg = vt.arg * 10 + atoi(cs))
    ON(S_QUEST, "0123456789", vt.arg = vt.arg * 10 + atoi(cs))
    ON(S_EQUAL, "0123456789", vt.arg = vt.arg * 10 + atoi(cs))
    DO(S_ARG, "A",          locate(c_x(), c_y() - P1(0)))
    DO(S_ARG, "B",          locate(c_x(), c_y() + P1(0)))
    DO(S_ARG, "C",          locate(c_x() + P1(0), c_y()))
    DO(S_ARG, "D",          locate(c_x() - P1(0), c_y()))
    DO(S_ARG, "G",          locate(P1(0) - 1, c_y()))
    DO(S_ARG, "d",          locate(c_x(), P1(0) - 1))
    DO(S_ARG, "Hf",         locate(P1(1) - 1, P1(0) - 1))
    DO(S_ARG, "I",          putch('\t'))//while (++c->c < s->ncol - 1 && t[c->c].c != L'*'))
    DO(S_ARG, "J",          ed())
    DO(S_ARG, "K",          el())
    DO(S_ARG, "L",          for (int i = 0; i < P1(0); ++i) cscroll(0, c_y(), getScreenWidth(), getScreenHeight() - c_y(), 1))
    DO(S_ARG, "M",          for (int i = 0; i < P1(0); ++i) cscroll(0, c_y(), getScreenWidth(), getScreenHeight() - c_y(), 0))
    DO(S_ARG, "P",          for (int i = 0; i < P1(0); ++i) delete_char())
    DO(S_ARG, "S",          for (int i = 0; i < P1(0); ++i) scroll_up())
    DO(S_ARG, "T",          for (int i = 0; i < P1(0); ++i) scroll_down())
    DO(S_ARG, "b",          if (c_x() > 0) { utf8_int32_t c = vpeek(c_x() - 1, c_y()); for (int i = 0; i < P1(0); ++i) putch(c); })//rep(vt));
    DO(S_ARG, "m",          sgr())
//    DO(S_ARG, "n",          if (P0(0) == 6) dsr(vt))
//    DO(S_ARG, "i",          (void)0)
//    DO(S_ARG, "s",          vt.oldcurs = vt.curs; vt.oldattrs = vt.attrs)
//    DO(S_ARG, "u",          vt.curs = vt.oldcurs; vt.attrs = vt.oldattrs)
    DO(S_ARG, "@",          cscroll(c_x(), c_y(), getScreenWidth() - c_x(), 1, 2))
    DO(S_ARG, "X",	    int x=c_x(); int y=c_y(); for (int i = 0; i < P0(0); ++i) putch(' '); locate(x, y);)
    DO(S_ARG, "`",	    locate(P1(0) - 1, c_y()))
    DO(S_ARG, "x",	    setColor(COL(FG), COL(BG));)
    DO(S_EQUAL, "C",	    (void)0)	// "Set cursor parameters"(?)
    DO(S_QUEST, "h",        (void)0)	// enable mouse?
    DO(S_QUEST, "l",        (void)0)	// disable mouse?

  if (vt.state != S_NUL) {
    consumearg();
    printf("ANSI: unknown %d %c (p0 %d p1 %d)\n", vt.state, i, P0(0), P0(1));
  }

  resetparser();
  return false;
}
