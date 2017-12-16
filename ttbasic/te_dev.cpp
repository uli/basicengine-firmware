/*	te_dev.cpp

	Text editor.

	CRT module for DEVBASIC.

	Copyright (c) 2015 Ulrich Hecht

	This program is free software; you can redistribute it and/or modify it
	under the terms of the GNU General Public License as published by the
	Free Software Foundation; either version 2, or (at your option) any
	later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "ttconfig.h"
#include "basic.h"
#include <tscreenBase.h>
extern tscreenBase *sc;

#define SIZEOF_PTR (sizeof(void *))

/* Definitions
   -----------
*/
#define CRT_NAME "DEVBASIC"

#define CRT_ROWS (sc->getHeight()-1)        /* CRT rows: 24 - 1 (system line) */
#define CRT_COLS (sc->getWidth())        /* CRT columns */

#define RULER_TAB     '!' /* Ruler: Tab stop character - ie: ! */
#define RULER_CHR     '.' /* Ruler: Character - ie: . */
#define SYS_LINE_SEP  '-' /* System line separator character - ie: - */

/* Keys
   ----
*/

#define CRT_ESC_KEY "ESC" /* ESCAPE key name */

#define K_UP	 0x81
#define K_DOWN	 0x80
#define K_LEFT	 0x82
#define K_RIGHT	 0x83

#define K_PGUP	 0x88
#define K_PGDOWN 0x87

#define K_BEGIN	 0x84 /* Ctl V */
#define K_END	 0x89 /* Ctl \ */

#define K_TOP    16 /* Ctl P */
#define K_BOTTOM 19 /* Ctl S */

#define K_TAB    9  /* Ctl I */

#define K_INTRO	 13 /* Ctl M */
#define K_ESC	 27 /* Ctl [ */

#define K_RDEL	 0x85
#define K_LDEL   8

#define K_CUT    21 /* Ctl U */
#define K_COPY   18 /* Crl R */
#define K_PASTE  23 /* Ctl W */

/* Help
   ----
*/

#define H_0 "Up     ^_ [UP]     Left   ^A [LEFT]"
#define H_1 "Down   ^^ [DOWN]   Right  ^F [RIGHT]"
#define H_2 "Begin  ^V          LtDel  7F [DEL]"
#define H_3 "End    ^\\          RtDel  ^G [GRAPH]"
#define H_4 "Top    ^P          PgUp   ^Q"
#define H_5 "Bottom ^S          PgDown ^Z"
#define H_6 ""
#define H_7 "Cut    ^U"           
#define H_8 "Copy   ^R          Intro  ^M [ENTER]"
#define H_9 "Paste  ^W          Esc    ^[ [BREAK]"

/* Setup CRT: Used when the editor starts
   --------------------------------------
   void CrtSetup(void)
*/
static void CrtSetup()
{
}

/* Reset CRT: Used when the editor exits
   -------------------------------------
   void CrtReset(void)
*/
static void CrtReset()
{
	err = 0;
}

static inline void CrtOut(int ch)
{
	sc->putch(ch);
}

static inline int CrtIn(void)
{
  sc->show_curs(true);
  int c = sc->get_ch();
  if (c)
    Serial.printf("crtin %02X/%d/%c\n", c, c, c);
//  sc->show_curs(false);
  return c;
}

/* Clear screen and send cursor to 0,0
   -----------------------------------
   void CrtClear(void)
*/
static inline void CrtClear()
{
	sc->cls();
}

/* Locate the cursor (HOME is 0,0)
   -------------------------------
   void CrtLocate(int row, int col)
*/
static inline void CrtLocate(int row, int col)
{
	sc->locate(col, row);
}

/* Erase line and cursor to row,0
   ------------------------------
   void CrtClearLine(int row)
*/
static inline void CrtClearLine(int row)
{
	sc->clerLine(row);
	sc->locate(0, row);
}

/* Erase from the cursor to the end of the line
   --------------------------------------------
*/
static void CrtClearEol()
{
	int cnt = sc->getWidth() - sc->c_x();
	for (int i = 0; i < cnt; ++i)
		sc->putch(' ');
}
