/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * mcurses.c - mcurses lib
 *
 * Copyright (c) 2011-2015 Frank Meyer - frank(at)fli4l.de
 *
 * Revision History:
 * V1.0 2015 xx xx Frank Meyer, original version
 * V1.1 2017 01 13 ChrisMicro, addepted as Arduino library, MCU specific functions removed
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

#include "mcurses-config.h"
#include "basic.h"

#include <string.h>

#ifdef __AVR__
	#include <avr/pgmspace.h>
#elif defined(ESP8266)
#else
	#define PROGMEM
	#define PSTR(x)                                 (x)
	#define pgm_read_byte(s)                        (*s)
#endif 

#include "mcurses.h"

#define SEQ_CSI                                 PSTR("\033[")                   // code introducer
#define SEQ_DELCH                               PSTR("\033[P")                  // delete character
#define SEQ_NEXTLINE                            PSTR("\033E")                   // goto next line (scroll up at end of scrolling region)
#define SEQ_INSERTLINE                          PSTR("\033[L")                  // insert line
#define SEQ_DELETELINE                          PSTR("\033[M")                  // delete line
#define SEQ_INSERT_MODE                         PSTR("\033[4h")                 // set insert mode
#define SEQ_REPLACE_MODE                        PSTR("\033[4l")                 // set replace mode
#define SEQ_RESET_SCRREG                        PSTR("\033[r")                  // reset scrolling region
#define SEQ_CURSOR_VIS                          PSTR("\033[?25")                // set cursor visible/not visible

static uint_fast8_t                             mcurses_scrl_start = 0;         // start of scrolling region, default is 0
static uint_fast8_t                             mcurses_scrl_end = LINES - 1;   // end of scrolling region, default is last line
static uint_fast8_t                             mcurses_nodelay;                // nodelay flag
static uint_fast8_t                             mcurses_halfdelay;              // halfdelay value, in tenths of a second

uint8_t                                         mcurses_is_up = 0;              // flag: mcurses is up
uint8_t                                    	mcurses_cury = 0xff;            // current y position of cursor, public (getyx())
uint8_t                                    	mcurses_curx = 0xff;            // current x position of cursor, public (getyx())

uint8_t start_fg, start_bg;

static uint_fast8_t mcurses_attr = 0;                    // current attributes
static uint8_t *attrs;

static uint_fast8_t mcurses_phyio_init (void)
{
	return false;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * PHYIO: done (AVR)
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
static void mcurses_phyio_done (void)
{
	
}

#define mcurses_phyio_getc c_getch

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * PHYIO: set/reset nodelay (AVR)
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
static void mcurses_phyio_nodelay (uint_fast8_t flag)
{
    mcurses_nodelay = flag;
}
/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * PHYIO: set/reset halfdelay (AVR)
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
static void mcurses_phyio_halfdelay (uint_fast8_t tenths)
{
    mcurses_halfdelay = tenths;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * PHYIO: flush output (AVR)
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
static void mcurses_phyio_flush_output ()
{
	
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * INTERN: put a character (raw)
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#define mcurses_putc screen_putch

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * INTERN: put a string from flash (raw)
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
#define mcurses_puts_P c_puts_P

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * INTERN: put a 3/2/1 digit integer number (raw)
 *
 * Here we don't want to use sprintf (too big on AVR/Z80) or itoa (not available on Z80)
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
mcurses_puti (uint_fast8_t i)
{
    uint_fast8_t ii;

    if (i >= 10)
    {
        if (i >= 100)
        {
            ii = i / 100;
            mcurses_putc (ii + '0');
            i -= 100 * ii;
        }

        ii = i / 10;
        mcurses_putc (ii + '0');
        i -= 10 * ii;
    }

    mcurses_putc (i + '0');
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * INTERN: addch or insch a character
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
mcurses_addch_or_insch (uint_fast8_t ch, uint_fast8_t insert)
{
    static uint_fast8_t  insert_mode = FALSE;

    if (insert)
    {
        if (! insert_mode)
        {
            mcurses_puts_P (SEQ_INSERT_MODE);
            insert_mode = TRUE;
        }
    }
    else
    {
        if (insert_mode)
        {
            mcurses_puts_P (SEQ_REPLACE_MODE);
            insert_mode = FALSE;
        }
    }

    int attr_idx = sc0.c_x() + sc0.c_y() * sc0.getWidth();
    if (attrs[attr_idx] != mcurses_attr) {
        screen_putch(ch);
        if (ch == '\\')
            screen_putch(ch);
        attrs[attr_idx] = mcurses_attr;
    } else
        screen_putch(ch, true);
    mcurses_curx++;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * INTERN: set scrolling region (raw)
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
static void
mysetscrreg (uint_fast8_t top, uint_fast8_t bottom)
{
    if (top == bottom)
    {
        mcurses_puts_P (SEQ_RESET_SCRREG);                                      // reset scrolling region
    }
    else
    {
        mcurses_puts_P (SEQ_CSI);
        mcurses_puti (top + 1);
        mcurses_putc (';');
        mcurses_puti (bottom + 1);
        mcurses_putc ('r');
    }
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * move cursor (raw)
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
static void inline
mymove (uint_fast8_t y, uint_fast8_t x)
{
    sc0.locate(x, y);
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: initialize
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
uint_fast8_t
initscr (void)
{
    uint_fast8_t rtc;

    start_fg = sc0.getFgColor();
    start_bg = sc0.getBgColor();

    attrs = (uint8_t *)calloc(sc0.getWidth() * sc0.getHeight(), 1);
    if (!attrs)
      return ERR;

    if (mcurses_phyio_init ())
    {
        attrset (A_NORMAL);
        clear ();
        move (0, 0);
        mcurses_is_up = 1;
        rtc = OK;
    }
    else
    {
        rtc = ERR;
    }
    return rtc;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: add character
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
addch (uint_fast8_t ch)
{
    mcurses_addch_or_insch (ch, FALSE);
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: add string
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
addstr (const char * str)
{
    while (*str)
    {
        mcurses_addch_or_insch (*str++, FALSE);
    }
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: add string
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
addstr_P (const char * str)
{
    uint_fast8_t ch;

    while ((ch = pgm_read_byte(str)) != '\0')
    {
        mcurses_addch_or_insch (ch, FALSE);
        str++;
    }
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: set attribute(s)
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
attrset (uint_fast16_t attr)
{
    uint_fast8_t        idx;

    if (attr != mcurses_attr)
    {
        sc0.setColor(COL(FG), COL(BG));

        idx = (attr & F_COLOR) >> 8;

        if (idx >= 1 && idx <= 8)
        {
            sc0.setColor(csp.colorFromRgb(CONFIG.color_scheme[idx-1]), sc0.getBgColor());
        }

        idx = (attr & B_COLOR) >> 12;

        if (idx >= 1 && idx <= 8)
        {
            sc0.setColor(sc0.getFgColor(), csp.colorFromRgb(CONFIG.color_scheme[idx-1]));
        }

        if (attr & A_REVERSE)
        {
            sc0.setColor(sc0.getBgColor(), sc0.getFgColor());
        }
        if (attr & A_UNDERLINE)
        {
            // unimplemented
        }
        if (attr & A_BLINK)
        {
            // unimplemented
        }
        if (attr & A_BOLD)
        {
            sc0.setColor(csp.colorFromRgb(255, 255, 255), csp.colorFromRgb(0,0,0));
        }
        if (attr & A_DIM)
        {
            // unimplemented
        }
        mcurses_attr = attr;
    }
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: move cursor
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
move (uint_fast8_t y, uint_fast8_t x)
{
    if (mcurses_cury != y || mcurses_curx != x)
    {
        mcurses_cury = y;
        mcurses_curx = x;
        mymove (y, x);
    }
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: delete line
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
deleteln (void)
{
    mysetscrreg (mcurses_scrl_start, mcurses_scrl_end);                         // set scrolling region
    mymove (mcurses_cury, 0);                                                   // goto to current line
    mcurses_puts_P (SEQ_DELETELINE);                                            // delete line
    mysetscrreg (0, 0);                                                         // reset scrolling region
    mymove (mcurses_cury, mcurses_curx);                                        // restore position
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: insert line
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
insertln (void)
{
    mysetscrreg (mcurses_cury, mcurses_scrl_end);                               // set scrolling region
    mymove (mcurses_cury, 0);                                                   // goto to current line
    mcurses_puts_P (SEQ_INSERTLINE);                                            // insert line
    mysetscrreg (0, 0);                                                         // reset scrolling region
    mymove (mcurses_cury, mcurses_curx);                                        // restore position
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: scroll
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
scroll (void)
{
    mysetscrreg (mcurses_scrl_start, mcurses_scrl_end);                         // set scrolling region
    mymove (mcurses_scrl_end, 0);                                               // goto to last line of scrolling region
    mcurses_puts_P (SEQ_NEXTLINE);                                              // next line
    mysetscrreg (0, 0);                                                         // reset scrolling region
    mymove (mcurses_cury, mcurses_curx);                                        // restore position
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: clear
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
clear (void)
{
    sc0.cls();
    memset(attrs, 0, sc0.getWidth() * sc0.getHeight());
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: clear to bottom of screen
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
clrtobot (void)
{
    sc0.clerLine(sc0.c_y(), sc0.c_x());
    for (int i = sc0.c_y() + 1; i < sc0.getHeight(); ++i)
      sc0.clerLine(i);
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: clear to end of line
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
clrtoeol (void)
{
  sc0.clerLine(sc0.c_y(), sc0.c_x());
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: delete character at cursor position
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
delch (void)
{
    mcurses_puts_P (SEQ_DELCH);
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: insert character
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
insch (uint_fast8_t ch)
{
    mcurses_addch_or_insch (ch, TRUE);
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: set scrolling region
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
setscrreg (uint_fast8_t t, uint_fast8_t b)
{
    mcurses_scrl_start = t;
    mcurses_scrl_end = b;
}

static void redraw_under_cursor()
{
    int x = sc0.c_x();
    int y = sc0.c_y();
    if (attrs[x + y * sc0.getWidth()] != A_NORMAL) {
        uint8_t tmp_attr = mcurses_attr;
        attrset(attrs[x + y * sc0.getWidth()]);
        screen_putch(sc0.vpeek(x, y));
        sc0.locate(x, y);
        attrset(tmp_attr);
    }
}
void
curs_set (uint_fast8_t visibility)
{
    if (visibility > 0)
        sc0.show_curs(1);
    else {
        sc0.show_curs(0);
        redraw_under_cursor();
    }
}


/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: refresh: flush output
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
refresh (void)
{
    mcurses_phyio_flush_output ();
//  sc0.refresh();
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: set/reset nodelay
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
nodelay (uint_fast8_t flag)
{
    if (mcurses_nodelay != flag)
    {
        mcurses_phyio_nodelay (flag);
    }
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: set/reset halfdelay
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
halfdelay (uint_fast8_t tenths)
{
    mcurses_phyio_halfdelay (tenths);
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: read key
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */

uint_fast8_t
getch (void)
{
    uint_fast8_t ch;

    refresh ();
    ch = mcurses_phyio_getc ();

    if (ch == '\r')
      ch = '\n';

    return ch;
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: read string (with mini editor built-in)
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
getnstr (char * str, uint_fast8_t maxlen)
{
    int ch;
    uint_fast8_t curlen = 0;
    uint_fast8_t curpos = 0;
    uint_fast8_t starty;
    uint_fast8_t startx;
    uint_fast8_t i;

    maxlen--;                               // reserve one byte in order to store '\0' in last position
    getyx (starty, startx);                 // get current cursor position

    while ((ch = getch ()) != KEY_CR)
    {
        switch (ch)
        {
            case KEY_LEFT:
                if (curpos > 0)
                {
                    curpos--;
                }
                break;
            case KEY_RIGHT:
                if (curpos < curlen)
                {
                    curpos++;
                }
                break;
            case KEY_HOME:
                curpos = 0;
                break;
            case KEY_END:
                curpos = curlen;
                break;
            case KEY_BACKSPACE:
                if (curpos > 0)
                {
                    curpos--;
                    curlen--;
                    move (starty, startx + curpos);

                    for (i = curpos; i < curlen; i++)
                    {
                        str[i] = str[i + 1];
                    }
                    str[i] = '\0';
                    delch();
                }
                break;

            case KEY_DC:
                if (curlen > 0)
                {
                    curlen--;
                    for (i = curpos; i < curlen; i++)
                    {
                        str[i] = str[i + 1];
                    }
                    str[i] = '\0';
                    delch();
                }
                break;

            default:
                if (curlen < maxlen && (ch & 0x7F) >= 32 && (ch & 0x7F) < 127)      // printable ascii 7bit or printable 8bit ISO8859
                {
                    for (i = curlen; i > curpos; i--)
                    {
                        str[i] = str[i - 1];
                    }
                    insch (ch);
                    str[curpos] = ch;
                    curpos++;
                    curlen++;
                }
        }
        move (starty, startx + curpos);
    }
    str[curlen] = '\0';
}

/*---------------------------------------------------------------------------------------------------------------------------------------------------
 * MCURSES: endwin
 *---------------------------------------------------------------------------------------------------------------------------------------------------
 */
void
endwin (void)
{
    move (LINES - 1, 0);                                                        // move cursor to last line
    clrtoeol ();                                                                // clear this line
    curs_set (TRUE);                                                            // show cursor
    //mcurses_puts_P(SEQ_REPLACE_MODE);                                           // reset insert mode
    refresh ();                                                                 // flush output
    mcurses_phyio_done ();                                                      // end of physical I/O
    mcurses_is_up = 0;
    free (attrs);
    sc0.setColor(start_fg, start_bg);
}

void scrl(int whence)
{
  if (whence >= LINES || whence <= -LINES) {
    sc0.cls();
    memset(attrs, 0, sc0.getWidth() * sc0.getHeight());
    return;
  }
  while (whence < 0) {
    sc0.scroll_down();
    memmove(attrs, attrs + sc0.getWidth(), sc0.getWidth() * (sc0.getHeight() - 1));
    memset(attrs, 0, sc0.getWidth());
    ++whence;
  }
  while (whence > 0) {
    sc0.scroll_up();
    memmove(attrs + sc0.getWidth(), attrs, sc0.getWidth() * (sc0.getHeight() - 1));
    memset(attrs + (sc0.getHeight() - 1) * sc0.getWidth(), 0, sc0.getWidth());
    --whence;
  }
}
