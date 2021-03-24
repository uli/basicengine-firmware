// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "help.h"

static const char *section_names[] = {
    [SECTION_BAS] = "BASIC core",
    [SECTION_M] = "math",
    [SECTION_OP] = "numeric",
    [SECTION_SOP] = "string",
    [SECTION_IO] = "input/output",
    [SECTION_FS] = "file system",
    [SECTION_BG] = "sprite/background engine",
    [SECTION_PIX] = "pixel graphics",
    [SECTION_SCR] = "screen handling",
    [SECTION_SND] = "sound",
    [SECTION_SYS] = "system",
};

static const char *type_names [] = {
    [TYPE_BC] = "command",
    [TYPE_BF] = "function",
    [TYPE_BN] = "constant",
    [TYPE_BO] = "operator",
};

static void print_wrapped(const char *text) {
    int indent = sc0.c_x();
    int last_space = sc0.c_x();

    const char *tp = text;
    while (*tp) {
        if (isspace(*tp))
            last_space = sc0.c_x();

        if (sc0.c_x() == sc0.getWidth() - 1) {
            while (sc0.c_x() > last_space && tp > text) {
                c_putch('\b');
                --tp;
            }
            newline();
            while (sc0.c_x() < indent)
                c_putch(' ');
            if (*tp == ' ')
                ++tp;
        }

        if (*tp == '\n') {
            c_putch(*tp++);
            while (sc0.c_x() < indent)
                c_putch(' ');
        } else
            c_putch(*tp++);
    }

    newline();
}

static void print_help(const struct help_t *h) {
    pixel_t saved_fg_color = sc0.getFgColor();
    pixel_t saved_bg_color = sc0.getBgColor();

    sc0.setColor(COL(COMMENT), COL(BG));
    c_printf("\n%s ", h->command);
    sc0.setColor(COL(LINENUM), COL(BG));
    c_printf("(%s, %s)\n\n", type_names[h->type], section_names[h->section]);
    sc0.setColor(COL(FG), COL(BG));
    print_wrapped(h->brief);

    if (h->usage) {
        sc0.setColor(COL(KEYWORD), COL(BG));
        c_puts("\nUsage:\n\n  ");
        sc0.setColor(COL(NUM), COL(BG));
        print_wrapped(h->usage);
        sc0.setColor(COL(FG), COL(BG));
    }

    if (h->args[0].name) {
        sc0.setColor(COL(KEYWORD), COL(BG));
        c_puts("\nArguments:\n\n");
        for (int i = 0; h->args[i].name; ++i) {
            sc0.setColor(COL(LVAR), COL(BG));
            c_printf("  %s\t", h->args[i].name);
            sc0.setColor(COL(FG), COL(BG));
            print_wrapped(h->args[i].descr);
        }
        sc0.setColor(COL(FG), COL(BG));
    }

    if (h->ret) {
        sc0.setColor(COL(KEYWORD), COL(BG));
        c_puts("\nReturn value:\n\n  ");
        sc0.setColor(COL(FG), COL(BG));
        print_wrapped(h->ret);
    }

    if (h->note) {
        sc0.setColor(COL(KEYWORD), COL(BG));
        c_puts("\nNote:\n\n  ");
        sc0.setColor(COL(FG), COL(BG));
        print_wrapped(h->note);
    }

    if (h->bugs) {
        sc0.setColor(COL(KEYWORD), COL(BG));
        c_puts("\nBugs:\n\n  ");
        sc0.setColor(COL(FG), COL(BG));
        print_wrapped(h->bugs);
    }

    if (h->ref[0]) {
        sc0.setColor(COL(KEYWORD), COL(BG));
        c_puts("\nSee also:\n\n");
        sc0.setColor(COL(FG), COL(BG));
        for (int i = 0; h->ref[i]; ++i) {
            c_printf("  HELP %s\n", h->ref[i]);
        }
    }

    newline();

    sc0.setColor(saved_fg_color, saved_bg_color);
}

void Basic::ihelp() {
    QList<int> tokens;
    QList<const struct help_t *> hints;

    while (!end_of_statement()) {
        int token = *cip++;
        if (token == I_EXTEND) {
            token = 256 + *cip++;
        }
        tokens.push_back(token);
    }

    for (int i = 0; help[i].command; ++i) {
        int tokens_found = 0;
        for (int k = 0; k < tokens.length(); ++k) {
            int j;
            for (j = 0; help[i].tokens[j]; ++j) {
                if (help[i].tokens[j] == tokens[k]) {
                    tokens_found++;
                    if (hints.indexOf(&help[i]) == -1)
                        hints.push_back(&help[i]);
                }
            }
            if (j != tokens.length()) {
                tokens_found = 0;
                break;
            }
        }
        if (tokens_found == tokens.length()) {
            print_help(&help[i]);
            return;
        }
    }

    if (hints.length() > 0) {
        pixel_t saved_fg_color = sc0.getFgColor();
        pixel_t saved_bg_color = sc0.getBgColor();

        sc0.setColor(COL(KEYWORD), COL(BG));
        c_puts("\nMatching commands:\n\n");
        sc0.setColor(COL(FG), COL(BG));
        for (int i = 0; i < hints.length(); ++i) {
            if (hints[i]->command) {
                c_printf("  HELP %s\n", hints[i]->command);
            }
        }

        sc0.setColor(COL(KEYWORD), COL(BG));
        c_puts("\nSee also:\n\n");
        sc0.setColor(COL(FG), COL(BG));
        for (int i = 0; i < hints.length(); ++i) {
            if (hints[i]->ref[0]) {
                for (int j = 0; hints[i]->ref[j]; ++j) {
                    c_printf("  HELP %s\n", hints[i]->ref[j]);
                }
            }
        }
        newline();
        sc0.setColor(saved_fg_color, saved_bg_color);
    } else {
        for (int i = 0; i < tokens.length(); ++i) {
            if ((tokens[i] < 256  && !kwtbl[tokens[i]]) ||
                (tokens[i] >= 256 && !kwtbl_ext[tokens[i] - 256])) {
                err = ERR_UNK;
                return;
            }
        }
        c_printf("No help available\n");
    }
}
