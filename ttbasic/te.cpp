/*	te.c

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

	Usage:

	te [file_name]

	Compile with MESCC for CP/M & Z80:

	cc te
	zsm te
	hexcom te

	Notes:

	Change TE_VERSION & CRT_FILE in 'te.h' as required, before compilation.

	Changes:

	29 Apr 2015 : Start working.
	02 May 2015 : Added Amstrad PCW version (te_pcw.c).
	03 May 2015 : Added K. Murakami's CP/M emulator for Win32 version (te_em1.c).
	04 May 2015 : Added te.h file. Added forced entry. Nice ruler with tab stops.
	07 May 2015 : While porting it to Pelles C & Windows 32 bit, it was discovered
	              collateral efects in 3 pieces of code. Recoded and marked below.
	              Solved bug in ReadFile() - characters > 0x7F caused 'bad
	              character' errors.
	              Added checking for filenames too long in command line.
	08 May 2015 : v1.00 : Added BackupFile().
	12 May 2015 : v1.01 : Now K_CUT copies the line to the clipboard before deleting it.
	14 May 2015 : v1.02 : Added support for word left & word right cursor movement.
	                      You must define K_LWORD and K_RWORD in your CRT_FILE
	                      to activate them.
	15 May 2015 : v1.03 : Modified getch & putch to getchr & putchr.
	31 Aug 2015 : v1.04 : Minor changes in comments and ReadLine().

	Notes:

	See FIX-ME notes.
*/

/* Libraries
   ---------
*/
//#include <mescc.h>
#include <string.h>
#include <ctype.h>
//#include <fileio.h>
//#include <sprintf.h>

/* CRT library
   -----------
*/
#include "te.h"

#include "te_dev.h"

#include "basic.h"

/* Globals
   -------
*/

/* Array of text lines
   -------------------
*/
char **lp_arr; /* Text lines pointers array */
int   lp_max; /* Max. # of lines */
int   lp_now; /* How man lines are in the array */
int   lp_cur; /* Current line */

/* Current line
   ------------
*/
char *ln_dat; /* Data buffer */
int   ln_max; /* Max. # of characters */
char *ln_clp; /* Clipboard data (just one line) */

/* Filename
   --------
*/
char file_name[FILENAME_MAX];

/* Editor box
   ----------
*/
int box_rows; /* Width */
int box_cols; /* Height */
int box_shr;  /* Position of current line (0..box_rows-1) */
int box_shc;  /* Position of current line (0..box_cols-1) */

/* Data & counters position
   ------------------------
*/
int ps_fname;   /* Filename */

int ps_lin_cur; /* Current line # */
int ps_lin_now; /* How many lines # */
int ps_lin_max; /* Max. # of lines */

int ps_col_cur; /* Current column # */
int ps_col_now; /* Line length # */
int ps_col_max; /* Max. line length */

/* Keyboard forced entry
   ---------------------
*/
char *fe_dat; /* Data buffer */
int fe_now;   /* How many characters are now in the buffer */
int fe_set;   /* Set position */
int fe_get;   /* Get position */

/* System line
   -----------
*/
int sysln;    /* NZ when written - for Loop() */

void ErrLineMem();
void Layout();
void Loop();

/* Read character from keyboard
   ----------------------------
*/
/* **************************** SEE #define
getchr()
{
	return CrtIn();
}
******************************* */

/* Print character on screen
   -------------------------
*/
/* **************************** SEE #define
putchr(ch)
int ch;
{
	CrtOut(ch);
}
******************************* */

/* Print string on screen
   ----------------------
*/
static inline void putstr(const char *s)
{
	while(*s)
		CrtOut(*s++);
}

/* Print string + '\n' on screen
   -----------------------------
*/
static inline void putln(char *s)
{
	putstr(s); CrtOut('\n');
}

/* Print number on screen
   ----------------------
*/
void putint(char *format, int value)
{
	char r[7]; /* -12345 + ZERO */

	sprintf(r, format, value);

	putstr(r);
}


/* Refresh editor box
   ------------------
   Starting from box row 'row', line 'line'.
*/
void Refresh(int row, int line)
{
	int i;

	for(i = row; i < box_rows; ++i)
	{
		CrtClearLine(BOX_ROW + i);

		if(line < lp_now)
			putstr(lp_arr[line++]);
	}
}

/* Refresh editor box
   ------------------
*/
void RefreshAll()
{
	Refresh(0, lp_cur - box_shr);
}

/* Return line # of first line printed on the editor box
   -----------------------------------------------------
*/
int LoopFirst()
{
	return lp_cur - box_shr;
}

/* Return line # of last line printed on the editor box
   ----------------------------------------------------
*/
int LoopLast()
{
	int last;

	last = LoopFirst() + box_rows - 1;

	return last >= lp_now - 1 ? lp_now - 1 : last; /* min(lp_now - 1, last) */
}

/* Go to document top
   ------------------
*/
void LoopTop()
{
	int first;

	first = LoopFirst();

	lp_cur = box_shr = box_shc = 0;

	if(first > 0)
		RefreshAll();
}

/* Go to document bottom
   ---------------------
*/
void LoopBottom()
{
	int first, last;

	first = LoopFirst();
	last = LoopLast();

	lp_cur = lp_now - 1; box_shc = 32000;

	if(last < lp_now - 1)
	{
		box_shr = box_rows - 1;
		RefreshAll();
	}
	else
		box_shr = last - first;
}

/* Page up
   -------
*/
void LoopPgUp()
{
	int first, to;

	first = LoopFirst();

	if(first)
	{
		if((to = first - box_rows) < 0) /* max(to, 0) */
			to = 0;

		lp_cur = to; box_shr = box_shc = 0;

		RefreshAll();
	}
	else
		LoopTop();
}

/* Page down
   ---------
*/
void LoopPgDown()
{
	int first, last, to;

	first = LoopFirst();
	last = LoopLast();

	if(last < lp_now - 1)
	{
		to = first + box_rows;

		if(to >= lp_now)
			to = lp_now - 1;

		lp_cur = to; box_shr = box_shc = 0;

		RefreshAll();
	}
	else
		LoopBottom();
}

/* Print filename
   --------------
*/
void LoopFileName()
{
	int i;

	CrtLocate(PS_ROW, ps_fname);

	for(i = FILENAME_MAX - 1; i; --i)
		putchr(' ');

	CrtLocate(PS_ROW, ps_fname);

	putstr(file_name[0] ? file_name : "-no name-");
}

/* Insert CR (intro)
   -----------------
*/
void LoopInsert()
{
	int left_len, right_len, i;
	char *p1, *p2;

	left_len = box_shc;
	right_len = strlen(ln_dat) - box_shc;

	p1 = (char *)malloc(left_len + 1);
	p2 = (char *)malloc(right_len + 1);

	if(p1 == NULL || p2 == NULL)
	{
		ErrLineMem();

		if(p1 != NULL)
			free(p1);
		if(p2 != NULL)
			free(p2);

		return;
	}

	strcpy(p2, ln_dat + box_shc);

	ln_dat[box_shc] = 0;

	strcpy(p1, ln_dat);

	for(i = lp_now; i > lp_cur; --i)
		lp_arr[i] = lp_arr[i - 1];

	++lp_now;

	free(lp_arr[lp_cur]);

	lp_arr[lp_cur++] = p1;
	lp_arr[lp_cur] = p2;

	CrtClearEol();

	if(box_shr < box_rows - 1)
		Refresh(++box_shr, lp_cur);
	else
		Refresh(0, lp_cur - box_rows + 1);

	box_shc = 0;
}

/* Delete CR on the left
   ---------------------
*/
void LoopLeftDel()
{
	int len_up, len_cur, i;
	char *p;

	len_up = strlen(lp_arr[lp_cur - 1]);
	len_cur = strlen(ln_dat);

	if(len_up + len_cur > ln_max)
	{
		/**ErrLineLong();**/ return;
	}

	if((p = (char *)malloc(len_up + len_cur + 1)) == NULL)
	{
		ErrLineMem(); return;
	}

	strcpy(p, lp_arr[lp_cur - 1]); strcat(p, ln_dat);

	--lp_now;

	free(lp_arr[lp_cur]);
	free(lp_arr[lp_cur - 1]);

	for(i = lp_cur; i < lp_now; ++i)
		lp_arr[i] = lp_arr[i + 1];

	/* *** Following code can cause collateral effects on some compilers ***
	i = lp_cur;

	while(i < lp_now)
		lp_arr[i] = lp_arr[++i];
	************************************************************************/

	lp_arr[lp_now] = NULL;

	lp_arr[--lp_cur] = p;

	box_shc = len_up;

	if(box_shr)
	{
		CrtLocate(BOX_ROW + --box_shr, BOX_COL + box_shc); putstr(lp_arr[lp_cur] + box_shc);

		Refresh(box_shr + 1, lp_cur + 1);
	}
	else
		Refresh(0, lp_cur);
}

/* Delete CR on the right
   ----------------------
*/
void LoopRightDel()
{
	int len_dn, len_cur, i;
	char *p;

	len_dn = strlen(lp_arr[lp_cur + 1]);
	len_cur = strlen(ln_dat);

	if(len_dn + len_cur > ln_max)
	{
		/**ErrLineLong();**/ return;
	}

	if((p = (char *)malloc(len_dn + len_cur + 1)) == NULL)
	{
		ErrLineMem(); return;
	}

	strcpy(p, ln_dat); strcat(p, lp_arr[lp_cur + 1]);

	--lp_now;

	free(lp_arr[lp_cur]);
	free(lp_arr[lp_cur + 1]);

	for(i = lp_cur + 1; i < lp_now; ++i)
		lp_arr[i] = lp_arr[i + 1];

	/* *** Following code can cause collateral effects on some compilers ***
	i = lp_cur + 1;

	while(i < lp_now)
		lp_arr[i] = lp_arr[++i];
	************************************************************************/

	lp_arr[lp_now] = NULL;

	lp_arr[lp_cur] = p;

	putstr(p + box_shc);

	if(box_shr < box_rows - 1)
		Refresh(box_shr + 1, lp_cur + 1);
}

/* Print program layout
   --------------------
*/
void Layout()
{
	int i, k;

	/* Clear screen */

	CrtClear();

	/* Header */

	putln("te:");

	/* Information layout */

	CrtLocate(PS_ROW, ps_lin_cur - 4); putstr(PS_TXT);

	/* Max. # of lines */

	CrtLocate(PS_ROW, ps_lin_max); putint("%04d", lp_max);

	/* # of columns */

	CrtLocate(PS_ROW, ps_col_max); putint("%02d", CRT_COLS);

	/* Ruler */

	CrtLocate(BOX_ROW - 1, 0);

	for(i = k = 0; i < CRT_COLS; ++i)
	{
		if(k++)
		{
			putchr(RULER_CHR);

			if(k == TAB_COLS)
				k = 0;
		}
		else
			putchr(RULER_TAB);
	}

	/* System line separator */

	CrtLocate(CRT_ROWS - 2, 0);

	for(i = CRT_COLS; i; --i)
		putchr(SYS_LINE_SEP);
}

/* Read simple line
   ----------------
   Returns last character entered: INTRO or ESC.
*/
int ReadLine(char *buf, int width)
{
	int len;
	int ch;

	putstr(buf); len=strlen(buf);

	while(1)
	{
		switch((ch = toupper(getchr())))
		{
			case K_LDEL :
				if(len)
				{
					putchr('\b'); putchr(' '); putchr('\b');

					--len;
				}
				break;
			case K_INTRO :
			case K_ESC :
				buf[len] = 0;
				return ch;
			default :
				if(len < width && ch >= ' ')
					putchr(buf[len++] = ch);
				break;
		}
	}
}

/* Print message on system line
   ----------------------------
   Message can be NULL == blank line / clear system line.
*/
void SysLine(char *s)
{
	CrtClearLine(CRT_ROWS - 1);

	if(s != NULL)
		putstr(s);

	/* Set flag for Loop() */

	sysln = 1;
}

/* Print message on system line an wait
   for a key press
   ------------------------------------
   Message can be NULL.
*/
void SysLineKey(char *s)
{
	SysLine(s);

	if(s != NULL)
		putchr(' ');

	putstr("Press ANY key, please: "); getchr();

	SysLine(NULL);
}

/* Print message on system line an wait
   for confirmation
   ------------------------------------
   Message can be NULL. Returns NZ if YES, else Z.
*/
int SysLineConf(char *s)
{
	int ch;

	SysLine(s);

	if(s != NULL)
		putchr(' ');

	putstr("Please, confirm Y/N: ");

	ch = toupper(getchr());

	SysLine(NULL);

	return ch == 'Y' ? 1 : 0;
}

/* Ask for a filename
   ------------------
   Return NZ if entered, else Z.
*/
int SysLineFile(char *fn)
{
	int ch;

	SysLine("Filename (or [");
	putstr(CRT_ESC_KEY);
	putstr("]): ");

	ch = ReadLine(fn, FILENAME_MAX - 1);

	SysLine(NULL);

	if(ch == K_INTRO && strlen(fn))
			return 1;

	return 0;
}

/* Print error message and wait for a key press
   --------------------------------------------
*/
void ErrLine(char *s)
{
	SysLineKey(s);
}

/* No memory error
   ---------------
*/
void ErrLineMem()
{
	ErrLine("Not enough memory.");
}

/* Line too long error
   -------------------
*/
void ErrLineLong()
{
	ErrLine("Line too long.");
}

/* Can't open file error
   ---------------------
*/
void ErrLineOpen()
{
	ErrLine("Can't open.");
}

/* Too many lines error
   --------------------
*/
void ErrLineTooMany()
{
	ErrLine("Too many lines.");
}

/* Reset lines array
   -----------------
*/
void ResetLines()
{
	int i;

	for(i = 0; i < lp_max; ++i)
	{
		if(lp_arr[i] != NULL)
		{
			free(lp_arr[i]); lp_arr[i] = NULL;
		}
	}

	lp_cur = lp_now = box_shr = box_shc = 0;
}

#include <sdfiles.h>

/* New file
   --------
*/
void NewFile()
{
	char *p;

	/* Free current contents */

	ResetLines();

	file_name[0] = 0;

	/* Build first line: Just one byte, please! */

	p = (char *)malloc(1); *p = 0; lp_arr[lp_now++] = p;
}

/* Read text file
   --------------
   Returns NZ on error.
*/
int ReadFile(const char *fn)
{
	Unifile fp;
	int ch, code, len, i, tabs;
	char *p;

	/* Free current contents */

	ResetLines();

	/* Setup some things */

	code = 0;

	/* Open the file */

	SysLine("Reading file.");

	if(!(fp = Unifile::open(fn, FILE_READ)))
	{
		ErrLineOpen(); return -1;
	}

	/* Read the file */

	for(i = 0; i < 32000; ++i)
	{
		if(fp.fgets(ln_dat, ln_max + 2) <= 0) /* ln_max + CR + ZERO */
			break;

		if(i == lp_max)
		{
			ErrLineTooMany(); ++code; break;
		}

		len = strlen(ln_dat);

		if(ln_dat[len - 1] == '\n')
			ln_dat[--len] = 0;
		else if(len > ln_max)
		{
			ErrLineLong(); ++code; break;
		}

		if((lp_arr[i] = (char *)malloc(len + 1)) == NULL)
		{
			ErrLineMem(); ++code; break;
		}

		strcpy(lp_arr[i], ln_dat);
	}

	/* Close the file */

	fp.close();

	/* Check errors */

	if(code)
		return -1;

	/* Set readed lines */

	lp_now = i;

	/* Check if empty file */

	if(!lp_now)
	{
		/* Build first line: Just one byte, please! */

		p = (char *)malloc(1); *p = 0; lp_arr[lp_now++] = p;
	}

	/* Change TABs to SPACEs, and check characters */

	for(i = tabs = 0; i < lp_now; ++i)
	{
		p = lp_arr[i];

		while((ch = (*p & 0xFF)))
		{
			if(ch < ' ')
			{
				if(ch == '\t')
				{
					*p = ' '; ++tabs;
				}
				else
				{
					ErrLine("Bad character found.");

					return -1;
				}
			}

			++p;
		}
	}

	/* Check TABs */

	if(tabs)
		ErrLine("Tabs changed to spaces.");

	/* Success */

	return 0;
}

/* Backup the previous file with the same name
   -------------------------------------------
   Return NZ on error.
*/

void BackupFile(char *fn)
{
	Unifile fp;
	char *bkp;

	/* Check if file exists */

	if((fp = Unifile::open(fn, FILE_READ)))
	{
	        fp.close();

		bkp = "te.bkp";

		/* Remove the previous backup file */

		// XXX: unimp
		//remove(bkp);

		/* Rename the old file as backup */

		// XXX: unimp
		//rename(fn, bkp);
	}
}

/* Write text file
   ---------------
   Returns NZ on error.
*/
int WriteFile(char *fn)
{
	Unifile fp;
	int i, err;
	char *p;

	/* Do backup of old file */

	SysLine("Writing file.");

	BackupFile(fn);

	/* Open the file */

	if((fp = Unifile::open(fn, FILE_OVERWRITE)) == NULL)
	{
		ErrLineOpen(); return -1;
	}

	/* Write the file */

	for(i = err = 0; i < lp_now; ++i)
	{
		p = lp_arr[i];

		/* FIX-ME: We don't have fputs() yet! */

		if(fp.write(p) < 0)
		{
			if (fp.write('\n') < 0)
				++err;
		}

		if(err)
		{
			fp.close(); // XXX remove(fn);

			ErrLine("Can't write.");

			return -1;
		}
	}

	fp.close();

	/* Success */

	return 0;
}

/* Clear the editor box
   --------------------
*/
void ClearBox()
{
	int i;

	for(i = 0; i < box_rows; ++i)
		CrtClearLine(BOX_ROW + i);
}

/* Print centered text on the screen
   ---------------------------------
*/
void CenterText(int row, const char *txt)
{
	CrtLocate(row, (CRT_COLS - strlen(txt)) / 2);

	putstr(txt);
}

/* Menu option: New
   ----------------
   Return Z to quit the menu.
*/
int MenuNew()
{
	if(lp_now > 1 || strlen(lp_arr[0]))
	{
		if(!SysLineConf(NULL))
			return 1;
	}

	NewFile();

	return 0;
}

/* Menu option: Open
   -----------------
   Return Z to quit the menu.
*/
int MenuOpen()
{
	char fn[FILENAME_MAX];

	if(lp_now > 1 || strlen(lp_arr[0]))
	{
		if(!SysLineConf(NULL))
			return 1;
	}

	fn[0] = 0;

	if(SysLineFile(fn))
	{
		if(ReadFile(fn))
			NewFile();
		else
			strcpy(file_name, fn);

		return 0;
	}

	return 1;
}

/* Menu option: Save as
   --------------------
   Return Z to quit the menu.
*/
int MenuSaveAs()
{
	char fn[FILENAME_MAX];

	strcpy(fn, file_name);

	if(SysLineFile(fn))
	{
		if(!WriteFile(fn))
			strcpy(file_name, fn);

		return 0;
	}

	return 1;
}

/* Menu option: Save
   -----------------
   Return Z to quit the menu.
*/
int MenuSave()
{
	if(!file_name[0])
		return MenuSaveAs();

	WriteFile(file_name);

	return 1;
}

/* Menu option: Help
   -----------------
*/
void MenuHelp()
{
	ClearBox();

	CrtLocate(BOX_ROW + 1, 0);

	putstr("HELP for te & "); putstr(CRT_NAME); putln(":\n");

#ifdef H_0
	putln(H_0);
#endif
#ifdef H_1
	putln(H_1);
#endif
#ifdef H_2
	putln(H_2);
#endif
#ifdef H_3
	putln(H_3);
#endif
#ifdef H_4
	putln(H_4);
#endif
#ifdef H_5
	putln(H_5);
#endif
#ifdef H_6
	putln(H_6);
#endif
#ifdef H_7
	putln(H_7);
#endif
#ifdef H_8
	putln(H_8);
#endif
#ifdef H_9
	putln(H_9);
#endif

	SysLineKey(NULL);
}

/* Menu option: About
   ------------------
*/
void MenuAbout()
{
	int row;

	row = BOX_ROW + 1;

	ClearBox();

	CenterText(row++, "te - Text Editor");
	row++;
	CenterText(row++, TE_VERSION);
	row++;
	CenterText(row++, "Configured for");
	CenterText(row++, CRT_NAME);
	row++;
	CenterText(row++, "(c) 2015 Miguel Garcia / FloppySoftware");
	row++;
	CenterText(row++, "www.floppysoftware.vacau.com");
	CenterText(row++, "cpm-connections.blogspot.com");
	CenterText(row++, "floppysoftware@gmail.com");

	SysLineKey(NULL);
}

/* Menu option: Quit program
   -------------------------
*/
int MenuExit()
{
	return !SysLineConf(NULL);
}

/* Show the menu
   -------------
   Return NZ to quit program.
*/
int Menu()
{
	int run, row, stay;

	/* Setup some things */

	run = stay = 1;

	/* Loop */

	while(run)
	{
		/* Show the menu */

		row = BOX_ROW + 1;

		ClearBox();

		CenterText(row++, "OPTIONS");
		row++;
		CenterText(row++, "New");
		CenterText(row++, "Open");
		CenterText(row++, "Save");
		CenterText(row++, "save As");
		CenterText(row++, "Help");
		CenterText(row++, "aBout te");
		CenterText(row++, "eXit te");

		/* Ask for option */

		SysLine("Enter option, please (or [");
		putstr(CRT_ESC_KEY);
		putstr("] to return): ");

		/* Do it */

		switch(toupper(getchr()))
		{
			case 'N'   : run = MenuNew(); break;
			case 'O'   : run = MenuOpen(); break;
			case 'S'   : run = MenuSave(); break;
			case 'A'   : run = MenuSaveAs(); break;
			case 'B'   : MenuAbout(); break;
			case 'H'   : MenuHelp(); break;
			case 'X'   : run = stay = MenuExit(); break;
			case K_ESC : run = 0; break;
		}
	}

	/* Clear editor box */

	ClearBox();

	SysLine(NULL);

	/* Return NZ to quit the program */

	return !stay;
}

/* Add a character to forced entry buffer
   --------------------------------------
   Return Z on success, NZ on failure.
*/
int ForceCh(int ch)
{
	if(fe_now < FORCED_MAX)
	{
		++fe_now;

		if(fe_set == FORCED_MAX)
			fe_set = 0;

		fe_dat[fe_set++] = ch;

		return 0;
	}

	return -1;
}

/* Add a string to forced entry buffer
   -----------------------------------
   Return Z on success, NZ on failure.
*/
int ForceStr(char *s)
{
	while(*s)
	{
		if(ForceCh(*s++))
			return -1;
	}

	return 0;
}

/* Return character from forced entry buffer
   -----------------------------------------
   Return Z if no characters left.
*/
int ForceGetCh()
{
	if(fe_now)
	{
		--fe_now;

		if(fe_get == FORCED_MAX)
			fe_get = 0;

		return fe_dat[fe_get++];
	}

	return 0;
}

int ParseBasic(const char *code)
{
	strcpy(lbuf, code);
	toktoi(false);
	cleartbuf();
	putlist(ibuf, 3);
	printf("-%s-\n", tbuf);
	return err;
}

/* Edit current line
   -----------------
   Returns last character entered.
*/
int BfEdit()
{
	int i, ch, len, run, upd_lin, upd_col, upd_now, upd_cur, spc, old_len;
	char *buf;

	/* Get current line contents */

	strcpy(ln_dat, lp_arr[lp_cur]);

	/* Setup some things */

	len = old_len = strlen(ln_dat);

	run = upd_col = upd_now = upd_cur = 1; upd_lin = spc = 0;

	/* Adjust column position */

	if(box_shc > len)
		box_shc = len;

	/* Loop */

	while(run)
	{
		/* Print line? */

		if(upd_lin)
		{
			upd_lin = 0;

		/* ************************************
			for(i = box_shc; i < len; ++i)
				putchr(ln_dat[i]);
		   ************************************ */

			putstr(ln_dat + box_shc);

			/* Print a space? */

			if(spc)
			{
				putchr(' '); spc = 0;
			}
		}

		/* Print length? */

		if(upd_now)
		{
			upd_now = 0;

			CrtLocate(PS_ROW, ps_col_now); putint("%02d", len);
		}

		/* Print column #? */

		if(upd_col)
		{
			upd_col = 0;

			CrtLocate(PS_ROW, ps_col_cur); putint("%02d", box_shc + 1);
		}

		/* Locate cursor? */

		if(upd_cur)
		{
			upd_cur = 0;

			CrtLocate(BOX_ROW + box_shr, BOX_COL + box_shc);
		}

		/* Get character: forced entry or keyboard */

		if(!(ch = ForceGetCh()))
			ch = getchr();

		/* Check character and do action */

		/* Note: This function does preliminary checks in some
		   keys for Loop(), to avoid wasted time. */

		switch(ch)
		{
			case K_LEFT :    /* Move one character to the left ----------- */
				if(box_shc)
				{
					--box_shc; ++upd_col;
				}
				else if(lp_cur)
				{
					box_shc = 9999 /* strlen(lp_arr[lp_cur - 1]) */ ;

					ch = K_UP;

					run = 0;
				}
				++upd_cur;
				break;
			case K_RIGHT :   /* Move one character to the right ---------- */
				if(box_shc < len)
				{
					++box_shc; ++upd_col;
				}
				else if(lp_cur < lp_now - 1)
				{
					ch = K_DOWN;

					box_shc = run = 0;
				}
				++upd_cur;
				break;
			case K_LDEL :   /* Delete one character to the left --------- */
				if(box_shc)
				{
					strcpy(ln_dat + box_shc - 1, ln_dat + box_shc);

					--box_shc; --len; ++upd_now; ++upd_lin; ++spc; ++upd_col;

					CrtLocate(BOX_ROW + box_shr, BOX_COL + box_shc);
				}
				else if(lp_cur)
					run = 0;
				++upd_cur;
				break;
			case K_RDEL :   /* Delete one character to the right ------- */
				if(box_shc < len)
				{
					strcpy(ln_dat + box_shc, ln_dat + box_shc + 1);

					--len; ++upd_now; ++upd_lin; ++spc;
				}
				else if(lp_cur < lp_now -1)
					run = 0;
				++upd_cur;
				break;
			case K_CUT :    /* Delete the line ------------------------ */
				CrtClearLine(BOX_ROW + box_shr);

				strcpy(ln_clp, ln_dat);

				ln_dat[0] = len = box_shc = 0;

				++upd_now; ++upd_col; ++upd_cur;
				break;
			case K_COPY :   /* Copy the line to the clipboard --------- */
				strcpy(ln_clp, ln_dat);
				++upd_cur;
				break;
			case K_PASTE :  /* Paste the line into current column ----- */
/*******************************************
				if((tmp = strlen(ln_clp)))
				{
					if(len + tmp < ln_max)
					{
						for(i = len; i > box_shc; --i)
							ln_dat[i + tmp - 1] = ln_dat[i - 1];

						for(i = 0; i < tmp; ++i)
							putchr((ln_dat[i + box_shc] = ln_clp[i]));

						len += tmp; box_shc += tmp;

						ln_dat[len] = 0;

						++upd_lin; ++upd_now; ++upd_col;
					}
				}
				++upd_cur;
**********************************************/
				ForceStr(ln_clp);
				++upd_cur;
				break;
			case K_PGUP :   /* Page up ------------------------------ */
			case K_TOP :    /* Document top ------------------------- */
				if(lp_cur || box_shc)
					run = 0;
				++upd_cur;
				break;
			case K_UP :     /* Up one line -------------------------- */
				if(lp_cur)
					run = 0;
				++upd_cur;
				break;
			case K_PGDOWN : /* Page down ---------------------------- */
			case K_BOTTOM : /* Document bottom ---------------------- */
				if(lp_cur < lp_now - 1 || box_shc != len)
					run = 0;
				++upd_cur;
				break;
			case K_DOWN :   /* One line down ------------------------ */
				if(lp_cur < lp_now - 1)
					run = 0;
				++upd_cur;
				break;
			case K_BEGIN :  /* Begin of line ------------------------ */
				if(box_shc)
				{
					box_shc = 0; ++upd_col;
				}
				++upd_cur;
				break;
			case K_END :    /* End of line -------------------------- */
				if(box_shc != len)
				{
					box_shc = len; ++upd_col;
				}
				++upd_cur;
				break;
			case K_ESC :    /* Escape: Show the menu ---------------- */
				run = 0;
				break;
			case K_INTRO :  /* Insert CR (split the line) ----------- */
				if(lp_now < lp_max)
					run = 0;
				break;
			case K_TAB :    /* Insert TAB (spaces) ------------------ */
				i = 8 - box_shc % TAB_COLS;

				while(i--)
				{
					if(ForceCh(' '))
						break;
				}
				break;
#ifdef K_LWORD
			case K_LWORD :  /* Move one word to the left ------------ */

				if(box_shc)
				{
					/* Skip the current word if we are at its begining */

					if(ln_dat[box_shc] != ' ' && ln_dat[box_shc - 1] == ' ')
						--box_shc;

					/* Skip spaces */

					while(box_shc && ln_dat[box_shc] == ' ')
						--box_shc;

					/* Find the beginning of the word */

					while(box_shc && ln_dat[box_shc] != ' ')
					{
						/* Go to the beginning of the word */

						if(ln_dat[--box_shc] == ' ')
						{
							++box_shc; break;
						}
					}

					++upd_col;
				}

				++upd_cur;

				break;
#endif

#ifdef K_RWORD
			case K_RWORD : /* Move one word to the right ------------ */

				/* Skip current word */

				while(ln_dat[box_shc] && ln_dat[box_shc] != ' ')
					++box_shc;

				/* Skip spaces */

				while(ln_dat[box_shc] == ' ')
					++box_shc;

				++upd_col; ++upd_cur;

				break;
#endif
			default :       /* Other: Insert character -------------- */
				if(len < ln_max && ch >= ' ')
				{
					putchr(ch);

					for(i = len; i > box_shc; --i)
					{
						ln_dat[i] = ln_dat[i - 1];
					}

			/* *** Following code can cause collateral effects on some compilers ***
					i = len;

					while(i > box_shc)
						ln_dat[i] = ln_dat[--i];
			************************************************************************/

					ln_dat[box_shc++] = ch; ln_dat[++len] = 0;

					++upd_lin; ++upd_now; ++upd_col;
				}
				++upd_cur;
				break;
		}
	}

	/* Save changes */

	if(len == old_len)
	{
		/* FIX-ME: May be we are just copying the same data if there were no changes */

		if (ParseBasic(ln_dat) == 0) {
			strcpy(lp_arr[lp_cur], tbuf);
			if (strcmp(ln_dat, tbuf)) {
				CrtClearLine(BOX_ROW + box_shr);
				putstr(tbuf);
			}
		} else
			strcpy(lp_arr[lp_cur], ln_dat);
	}
	else if((buf = (char *)malloc(len + 1)) == NULL)
	{
		ErrLineMem(); /* FIX-ME: Re-print the line with old contents? */
	}
	else
	{
		if (ParseBasic(ln_dat) == 0) {
			strcpy(buf, tbuf);
			if (strcmp(ln_dat, tbuf)) {
				CrtClearLine(BOX_ROW + box_shr);
				putstr(tbuf);
			}
		} else
			strcpy(buf, ln_dat);

		free(lp_arr[lp_cur]);

		lp_arr[lp_cur] = buf;
	}

	/* Return last character entered */

	return ch;
}


/* Main loop
   ---------
*/
void Loop()
{
	int run, ch;

	/* Setup forced entry */

	fe_now = fe_get = fe_set = 0;

	/* Setup more things */

	run = sysln = 1;

	/* Print filename */

	LoopFileName();

	/* Refresh editor box */

	RefreshAll();

	/* Loop */

	while(run)
	{
		/* Refresh system line message if changed */

		if(sysln)
		{
			SysLine("Press [");
			putstr(CRT_ESC_KEY);
			putstr("] to show the menu.");

			sysln = 0; /* Reset flag */
		}

		/* Print current column position & line length */

		CrtLocate(PS_ROW, ps_lin_cur); putint("%04d", lp_cur + 1);
		CrtLocate(PS_ROW, ps_lin_now); putint("%04d", lp_now);

		/* Edit the line */

		ch = BfEdit();

		/* Note: BfEdit() does previous checks for following
		   actions, to not to waste time when an action is not
		   possible */

		/* Check returned control character */

		switch(ch)
		{
			case K_UP :    /* Up one line --------------------- */
				--lp_cur;

				if(box_shr)
					--box_shr;
				else
					Refresh(0, lp_cur);
				break;
			case K_DOWN :  /* Down one line ------------------- */
				++lp_cur;

				if(box_shr < box_rows - 1)
					++box_shr;
				else
					Refresh(0, lp_cur - box_rows + 1);
				break;
			case K_INTRO : /* Insert CR ----------------------- */
				LoopInsert();
				break;
			case K_LDEL :  /* Delete CR on the left ----------- */
				LoopLeftDel();
				break;
			case K_RDEL :  /* Delete CR on the right ---------- */
				LoopRightDel();
				break;
			case K_PGUP :  /* Page up ------------------------- */
				LoopPgUp();
				break;
			case K_PGDOWN :/* Page down ----------------------- */
				LoopPgDown();
				break;
			case K_TOP :   /* Top of document ----------------- */
				LoopTop();
				break;
			case K_BOTTOM :/* Bottom of document -------------- */
				LoopBottom();
				break;
			case K_ESC :   /* Escape: show the menu ----------- */
				if(Menu())
					run = 0;
				else
				{
					LoopFileName(); /* Refresh filename */
					RefreshAll();   /* Refresh editor box */
				}
				break;
		}
	}
}

/* Program entry
   -------------
*/
int te_main(char argc, const char **argv)
{
	int i;

	/* Setup some globals */

	box_rows = CRT_ROWS - 4;
	box_cols = CRT_COLS;

	lp_max = MAX_LINES;

	ln_max = box_cols - 1;

	ps_fname = 4;

	ps_lin_cur = CRT_COLS - 31;
	ps_lin_now = ps_lin_cur + 5;
	ps_lin_max = ps_lin_now + 5;

	ps_col_cur = CRT_COLS - 12;
	ps_col_max = ps_col_cur + 3;

	ps_col_now = CRT_COLS - 2;

	/* Setup CRT */

	CrtSetup();

	/* Print layout */

	Layout();

	/* Allocate buffers */

	ln_dat = (char *)malloc(ln_max + 2);

	ln_clp = (char *)malloc(ln_max + 1);

	fe_dat = (char *)malloc(FORCED_MAX);

	lp_arr = (char **)malloc(lp_max * SIZEOF_PTR);

	if(ln_dat == NULL || ln_clp == NULL || fe_dat == NULL || lp_arr == NULL)
	{
		ErrLineMem(); CrtReset(); return 1;
	}

	/* Setup more things */

	*ln_clp = 0;

	/* Setup lines array */

	for(i = 0; i < lp_max; ++i)
		lp_arr[i] = NULL;

	/* Check command line */

	if(argc == 1)
	{
		NewFile();
	}
	else if(argc == 2)
	{
		if(strlen(argv[1]) > FILENAME_MAX - 1)
		{
			ErrLine("Filename too long.");

			NewFile();
		}
		else if(ReadFile(argv[1]))
			NewFile();
		else
			strcpy(file_name, argv[1]);
	}
	else
	{
		ErrLine("Bad command line. Use: te [filename]");

		NewFile();
	}

	/* Main loop */

	Loop();

	/* FIX-ME: We should free all buffers allocated with malloc? Not needed in CP/M. */

	/* Clear CRT */

	CrtClear();

	/* Reset CRT */

	CrtReset();

	/* Exit */

	return 0;
}
