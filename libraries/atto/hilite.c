/* hlite.c, generic syntax hilighting, Atto Emacs, Hugh Barney, Public Domain, 2016 */

#include "header.h"
#include <eb_basic.h>

int state = ID_DEFAULT;
int next_state = ID_DEFAULT;
int skip_count = 0;

char_t get_at(buffer_t *bp, point_t pt)
{
	return (*ptr(bp, pt));
}

static char_t symbols[] = "{}[]()!'£$%^&*-+=:;@~#<>,.?/\\|";

int is_symbol(char_t c)
{
	register char_t *p = symbols;

	for (p = symbols; *p != '\0'; p++)
		if (*p == c) return 1;
	return 0;
}

void set_parse_state(buffer_t *bp, point_t pt)
{
	register point_t po;

	state = ID_DEFAULT;
	next_state = ID_LINENUM;
	skip_count = 0;

	for (po =0; po < pt; po++)
		parse_text_basic(bp, po);
//		parse_text_c(bp, po);
}

int parse_text_c(buffer_t *bp, point_t pt)
{
	if (skip_count-- > 0)
		return state;

	char_t c_now = get_at(bp, pt);
	char_t c_next = get_at(bp, pt + 1);
	state = next_state;

	if (state == ID_DEFAULT && c_now == '/' && c_next == '*') {
		skip_count = 1;
		return (next_state = state = ID_BLOCK_COMMENT);
	}

	if (state == ID_BLOCK_COMMENT && c_now == '*' && c_next == '/') {
		skip_count = 1;
		next_state = ID_DEFAULT;
		return ID_BLOCK_COMMENT;
	}

	if (state == ID_DEFAULT && c_now == '/' && c_next == '/') {
		skip_count = 1;
		return (next_state = state = ID_LINE_COMMENT);
	}

	if (state == ID_LINE_COMMENT && c_now == '\n')
		return (next_state = ID_DEFAULT);

	if (state == ID_DEFAULT && c_now == '"')
		return (next_state = ID_DOUBLE_STRING);

	if (state == ID_DOUBLE_STRING && c_now == '\\') {
		skip_count = 1;
		return (next_state = ID_DOUBLE_STRING);
	}

	if (state == ID_DOUBLE_STRING && c_now == '"') {
		next_state = ID_DEFAULT;
		return ID_DOUBLE_STRING;
	}

	if (state == ID_DEFAULT && c_now == '\'')
		return (next_state = ID_SINGLE_STRING);

	if (state == ID_SINGLE_STRING && c_now == '\\') {
		skip_count = 1;
		return (next_state = ID_SINGLE_STRING);
	}

	if (state == ID_SINGLE_STRING && c_now == '\'') {
		next_state = ID_DEFAULT;
		return ID_SINGLE_STRING;
	}

	if (state != ID_DEFAULT)
		return (next_state = state);

	if (state == ID_DEFAULT && c_now >= '0' && c_now <= '9') {
		next_state = ID_DEFAULT;
		return (state = ID_DIGITS);
	}

	if (state == ID_DEFAULT && 1 == is_symbol(c_now)) {
		next_state = ID_DEFAULT;
		return (state = ID_SYMBOL);
	}

	return (next_state = state);
}

int parse_text_basic(buffer_t *bp, point_t pt)
{
	if (skip_count-- > 0)
		return state;

	char_t c_now = get_at(bp, pt);
	char_t c_next = get_at(bp, pt + 1);
	state = next_state;

	if (state == ID_LINENUM && !isdigit(c_now))
		state = ID_DEFAULT;

	if (state == ID_DEFAULT && c_now == '/' && c_next == '*') {
		skip_count = 1;
		return (next_state = state = ID_BLOCK_COMMENT);
	}

	if (state == ID_BLOCK_COMMENT && c_now == '*' && c_next == '/') {
		skip_count = 1;
		next_state = ID_DEFAULT;
		return ID_BLOCK_COMMENT;
	}

	if (c_now == '\'') {
		return (next_state = state = ID_LINE_COMMENT);
	}

	if (state == ID_DEFAULT && c_now == '@')
		return (next_state = state = ID_LVAR);

	if (c_now == '\n')
		return (next_state = ID_LINENUM);

	if (state == ID_DEFAULT && c_now == '"')
		return (next_state = ID_DOUBLE_STRING);

	if (state == ID_DOUBLE_STRING && c_now == '"') {
		next_state = ID_DEFAULT;
		return ID_DOUBLE_STRING;
	}

	if (state == ID_DEFAULT && c_now >= '0' && c_now <= '9') {
		next_state = ID_DEFAULT;
		return (state = ID_DIGITS);
	}

	if ((state == ID_DEFAULT || state == ID_PROC || state == ID_LVAR) && 1 == is_symbol(c_now)) {
		next_state = ID_DEFAULT;
		return (state = ID_SYMBOL);
	}

	if (state == ID_DEFAULT) {
		const char **k;
		int i;
		for (i = 0, k = eb_kwtbl(); i < eb_kwtbl_size(); ++k, ++i) {
			const char *kk = *k;
			if (!kk)
				continue;
			int ppt = pt;
			while (*kk) {
				if (tolower(get_at(bp, ppt++)) != tolower(*kk))
					break;
				++kk;
			}
			if (!*kk) {
				skip_count = strlen(*k) - 1;
				if (!strcmp(*k, "REM"))
					next_state = ID_LINE_COMMENT;
				else if (!strcmp(*k, "PROC") || !strcmp(*k, "CALL") || !strcmp(*k, "FN"))
					next_state = ID_PROC;
				else
					next_state = ID_DEFAULT;
				return (state = ID_KEYWORD);
			}
		}
	}

	return (next_state = state);
}
