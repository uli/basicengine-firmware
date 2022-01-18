// SPDX-License-Identifier: MIT
// Copyright (c) 2022 Ulrich Hecht

#include "basic.h"

extern bool screen_putch_enable_reverse;

enum ansi_state {
  S_NUL = 0,
  S_ESC,
  S_ARG,
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

static void sgr() {
  #define FGBG(c) \
    if (P0(i) < 40) \
      sc0.setColor(COL(c), sc0.getBgColor()); \
    else \
      sc0.setColor(sc0.getFgColor(), COL(c));
  #define SGR_UNIMP printf("ANSI: unimplemented sgr %d\n", P0(i))

  for (size_t i = 0; i < vt.npar; i++) {
    switch (P0(i)){
    case  0:	// reset
      if (screen_putch_enable_reverse) {
        screen_putch_enable_reverse = false;
        sc0.flipColors();
      }
      sc0.setColor(COL(FG), COL(BG));
      break;
    case  1: case 22: SGR_UNIMP; break; // XXX: bold
    case  2: case 23: SGR_UNIMP; break; // XXX: dim
    case  4: case 24: SGR_UNIMP; break; // XXX: underline
    case  5: case 25: SGR_UNIMP; break; //vt->attrs.blink     = P0(0) < 20; break;
    case  7:
      if (!screen_putch_enable_reverse) {
        screen_putch_enable_reverse = true;
        sc0.flipColors();
      }
      break;
    case 27:
      if (screen_putch_enable_reverse) {
        screen_putch_enable_reverse = false;
        sc0.flipColors();
      }
      break;
    case  8: case 28: SGR_UNIMP; break;//vt->attrs.invisible = P0(0) < 20; break;
    case 10: case 11: SGR_UNIMP; break;//vt->acs             = P0(0) > 10; break;
    case 30: case 40: FGBG(BG);            break;
    case 31: case 41: FGBG(PROC);              break;
    case 32: case 42: FGBG(COMMENT);            break;
    case 33: case 43: FGBG(FG); /* XXX: yellow */           break;
    case 34: case 44: FGBG(STR);             break;
    case 35: case 45: FGBG(OP);          break;
    case 36: case 46: FGBG(NUM);             break;
    case 37: case 47: FGBG(FG);            break;
    case 39:	// default FG
      sc0.setColor(COL(FG), sc0.getBgColor());
      break;
    case 49:	// default BG
      sc0.setColor(sc0.getFgColor(), COL(BG));
      break;
    default:
      printf("ANSI: unknown sgr %d\n", P0(i));
      break;
    }
  }
}

static void ed() {
    #define ED_UNIMP printf("ANSI: unimplemented ed %d\n", P0(0))

    switch (P0(0)){
        case 0:		// clear from cursor
          sc0.clerLine(sc0.c_y(), sc0.c_x());
          for (int i = sc0.c_y() + 1; i < sc0.getHeight(); ++i)
            sc0.clerLine(i);
          break;
        case 1: ED_UNIMP; break;	// XXX: clear to cursor
        case 2:		// clear screen
          sc0.cls();
          break;
        default:
          printf("ANSI: unknown ed %d\n", P0(0));
          return;
    }
}

static void el() {
  #define EL_UNIMP printf("ANSI: unimplemented el %d\n", P0(0))

  switch (P0(0)){
      case 0: sc0.clerLine(sc0.c_y(), sc0.c_x()); break;
      case 1: {
        int x = sc0.c_x();
        sc0.locate(0, sc0.c_y());
        for (int i = 0; i < x; i++)
          sc0.putch(' ');
        sc0.locate(x, sc0.c_y());
      }
      break; // clear line to cursor
      case 2: sc0.clerLine(sc0.c_y()); break;
  }
}

bool ansi_machine(utf8_int32_t i)
{
    if (i >= 256)	// or even 128?
      return false;

    char cs[] = {(char)i, 0};

    #define ON(S, C, A) if (vt.state == (S) && strchr(C, i)){ A; return true;}
    #define DO(S, C, A) ON(S, C, consumearg(); if (!vt.ignored) {A;} \
                                 /*fixcursor(vt);*/ resetparser(););

//    printf("stat %d i %c\n", vt.state, i);
    DO(S_NUL, "\x07",       true)//CB(vt, TMT_MSG_BELL, NULL))
    DO(S_NUL, "\x08",       sc0.locate(sc0.c_x() - 1, sc0.c_y()))//if (c->c) c->c--)
    ON(S_NUL, "\x1b",       vt.state = S_ESC)
    ON(S_ESC, "\x1b",       vt.state = S_ESC)
    DO(S_ESC, "H",          sc0.locate(0, 0))
    ON(S_ESC, "[",          vt.state = S_ARG)
    ON(S_ARG, "\x1b",       vt.state = S_ESC)
    ON(S_ARG, ";",          consumearg())
    ON(S_ARG, "?",          (void)0)
    ON(S_ARG, "0123456789", vt.arg = vt.arg * 10 + atoi(cs))
    DO(S_ARG, "A",          sc0.locate(sc0.c_x(), sc0.c_y() - P1(0)))//c->r = MAX(c->r - P1(0), 0))
    DO(S_ARG, "B",          sc0.locate(sc0.c_x(), sc0.c_y() + P1(0)))//c->r = MIN(c->r + P1(0), s->nline - 1))
    DO(S_ARG, "C",          sc0.locate(sc0.c_x() + P1(0), sc0.c_y()))//c->c = MIN(c->c + P1(0), s->ncol - 1))
    DO(S_ARG, "D",          sc0.locate(sc0.c_x() - P1(0), sc0.c_y()))//c->c = MIN(c->c - P1(0), c->c))
    DO(S_ARG, "G",          sc0.locate(P1(0) - 1, sc0.c_y()))//c->c = MIN(P1(0) - 1, s->ncol - 1))
    DO(S_ARG, "d",          sc0.locate(sc0.c_x(), P1(0) - 1))//c->r = MIN(P1(0) - 1, s->nline - 1))
    DO(S_ARG, "Hf",         sc0.locate(P1(1) - 1, P1(0) - 1))
    DO(S_ARG, "I",          sc0.putch('\t'))//while (++c->c < s->ncol - 1 && t[c->c].c != L'*'))
    DO(S_ARG, "J",          ed())
    DO(S_ARG, "K",          el())
    DO(S_ARG, "L",          for (int i = 0; i < P1(0); ++i) sc0.cscroll(0, sc0.c_y(), sc0.getScreenWidth(), sc0.getScreenHeight() - sc0.c_y(), 1))//scrdn(vt, c->r, P1(0)))
    DO(S_ARG, "M",          for (int i = 0; i < P1(0); ++i) sc0.cscroll(0, sc0.c_y(), sc0.getScreenWidth(), sc0.getScreenHeight() - sc0.c_y(), 0))//scrup(vt, c->r, P1(0)))
    DO(S_ARG, "P",          for (int i = 0; i < P1(0); ++i) sc0.delete_char())//dch(vt))
    DO(S_ARG, "S",          for (int i = 0; i < P1(0); ++i) sc0.scroll_up())//scrup(vt, 0, P1(0)))
    DO(S_ARG, "T",          for (int i = 0; i < P1(0); ++i) sc0.scroll_down())//scrdn(vt, 0, P1(0)))
    DO(S_ARG, "m",          sgr())
    DO(S_ARG, "h",          printf("curon\n");sc0.show_curs(true))//if (P0(0) == 25) CB(vt, TMT_MSG_CURSOR, "t"))
    DO(S_ARG, "@",          sc0.cscroll(sc0.c_x(), sc0.c_y(), sc0.getScreenWidth() - sc0.c_x(), 1, 2))//sc0.Insert_char(' '))//ich(vt))

  if (vt.state != S_NUL)
    printf("ANSI: unknown %s %c (p0 %d p1 %d)\n", vt.state == S_ESC ? "esc" : "arg", i, P0(0), P0(1));

  resetparser();
  return false;
}
