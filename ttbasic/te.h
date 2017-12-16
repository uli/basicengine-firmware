/*	te.h

	Text editor.

	Main module.

	Copyright (c) 2015 Miguel Garcia / FloppySoftware

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

	Changes:

	04 May 2015 : 1st version.
	12 May 2015 : v1.01
	14 May 2015 : v1.02
	15 May 2015 : v1.03
	31 Aug 2015 : v1.04

	Notes:

	Change TE_VERSION & CRT_FILE as required, before compilation.
*/

/* Version
   -------
*/

#define TE_VERSION "v1.04 / 31 Aug 2015 for CP/M"  /* Program version and date */

/* CRT file
   --------
*/

#define CRT_FILE "te_pcw.c" /* CRT file */

/* More defs.
   ----------
*/

#define MAX_LINES 512    /* Max. # of text lines: each empty line uses 2 bytes with the Z80 */

#define TAB_COLS 8       /* How many columns has a tab. (usually 8) */

#define FORCED_MAX 128   /* Keyboard forced entry buffer size (for paste, tabs, etc.) */

#define PS_ROW 0         /* Information position */
#define PS_TXT "Lin:0000/0000/0000 Col:00/00 Len:00"  /* Information layout */

#define BOX_ROW 2        /* Editor box position */
#define BOX_COL 0        /* Editor box position */

#define getchr CrtIn      /* Get a character from the keyboard */
#define putchr CrtOut     /* Print a character on screen */

