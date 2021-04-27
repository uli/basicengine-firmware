/* main.c, Atto Emacs, Public Domain, Hugh Barney, 2016, Derived from: Anthony's Editor January 93 */

#include "header.h"

int done;
point_t nscrap;
char_t *scrap;

char_t *input;
int msgflag;
char msgline[TEMPBUF];
char temp[TEMPBUF];
char searchtext[STRBUF_M];
char replace[STRBUF_M];

keymap_t *key_return;
keymap_t *key_map;
buffer_t *curbp;			/* current buffer */
buffer_t *bheadp;			/* head of list of buffers */
window_t *curwp;
window_t *wheadp;

#ifdef ENGINEBASIC
const unsigned short color_pairs[] = {
	[ID_DEFAULT]  = F_CYAN | B_BLACK,
	[ID_SYMBOL]   = F_WHITE | B_BLACK,
	[ID_MODELINE] = F_BLACK | B_WHITE,
	[ID_DIGITS]   = F_YELLOW | B_BLACK,
	[ID_BLOCK_COMMENT] = F_GREEN | B_BLACK,
	[ID_LINE_COMMENT] = F_GREEN | B_BLACK,
	[ID_SINGLE_STRING] = F_YELLOW | B_BLACK,
	[ID_DOUBLE_STRING] = F_YELLOW | B_BLACK,
};
#endif

int main(int argc, char **argv)
{
#ifndef ENGINEBASIC
	setlocale(LC_ALL, "") ; /* required for 3,4 byte UTF8 chars */
#endif
	if (initscr() == NULL) fatal("%s: Failed to initialize the screen.\n");
#ifndef ENGINEBASIC
	raw();
	noecho();
	idlok(stdscr, TRUE);

	start_color();
	init_pair(ID_DEFAULT, COLOR_CYAN, COLOR_BLACK);          /* alpha */
	init_pair(ID_SYMBOL, COLOR_WHITE, COLOR_BLACK);          /* non alpha, non digit */
	init_pair(ID_MODELINE, COLOR_BLACK, COLOR_WHITE);        /* modeline */
	init_pair(ID_DIGITS, COLOR_YELLOW, COLOR_BLACK);         /* digits */
	init_pair(ID_BLOCK_COMMENT, COLOR_GREEN, COLOR_BLACK);   /* block comments */
	init_pair(ID_LINE_COMMENT, COLOR_GREEN, COLOR_BLACK);    /* line comments */
	init_pair(ID_SINGLE_STRING, COLOR_YELLOW, COLOR_BLACK);  /* single quoted strings */
	init_pair(ID_DOUBLE_STRING, COLOR_YELLOW, COLOR_BLACK);  /* double quoted strings */
#endif

	if (1 < argc) {
		curbp = find_buffer(argv[1], TRUE);
		(void) insert_file(argv[1], FALSE);
		/* Save filename irregardless of load() success. */
		strncpy(curbp->b_fname, argv[1], NAME_MAX);
		curbp->b_fname[NAME_MAX] = '\0'; /* force truncation */
	} else {
		curbp = find_buffer("*scratch*", TRUE);
		strncpy(curbp->b_bname, "*scratch*", STRBUF_S);
	}

	wheadp = curwp = new_window();
	one_window(curwp);
	associate_b2w(curbp, curwp);

	if (!growgap(curbp, CHUNK)) fatal("%s: Failed to allocate required memory.\n");
	key_map = keymap;

#ifdef ENGINEBASIC
	curs_set(1);
#endif
	while (!done) {
		update_display();
		input = get_key(key_map, &key_return);

		if (key_return != NULL) {
			(key_return->func)();
		} else {
			/* allow TAB and NEWLINE, otherwise any Control Char is 'Not bound' */
			if (*input > 31 || *input == 10 || *input == 9)
				insert();
                        else {
				flushinp(); /* discard without writing in buffer */
				msg("Not bound");
			}
		}
	}

	if (scrap != NULL) free(scrap);
	move(LINES-1, 0);
	refresh();
#ifndef ENGINEBASIC
	noraw();
#endif
	endwin();
	return 0;
}

void fatal(char *msg)
{
#ifndef ENGINEBASIC
	if (curscr != NULL) {
		move(LINES-1, 0);
		refresh();
		noraw();
		endwin();
		putchar('\n');
	}
#endif
	fprintf(stderr, msg, PROG_NAME);
	exit(1);
}

void msg(char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	(void)vsprintf(msgline, msg, args);
	va_end(args);
	msgflag = TRUE;
}
