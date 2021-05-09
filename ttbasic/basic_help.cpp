// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "help.h"
#include <utf8.h>

#define __(s) (s)
static const char *section_names[] = {
    [SECTION_BAS] = __("BASIC core"),
    [SECTION_M] = __("math"),
    [SECTION_OP] = __("numeric"),
    [SECTION_SOP] = __("string"),
    [SECTION_IO] = __("input/output"),
    [SECTION_FS] = __("file system"),
    [SECTION_BG] = __("sprite/background engine"),
    [SECTION_PIX] = __("pixel graphics"),
    [SECTION_SCR] = __("screen handling"),
    [SECTION_SND] = __("sound"),
    [SECTION_SYS] = __("system"),
};

static const char *type_names [] = {
    [TYPE_BC] = __("command"),
    [TYPE_BF] = __("function"),
    [TYPE_BN] = __("constant"),
    [TYPE_BO] = __("operator"),
};
#undef __

static void print_wrapped(const char *text) {
    int indent = sc0.c_x();
    int last_space = sc0.c_x();

    const char *tp = text;
    while (*tp) {
        utf8_int32_t c;
        utf8codepoint(tp, &c);

        if (isspace(c))
            last_space = sc0.c_x();

        if (sc0.c_x() == sc0.getWidth() - 1) {
            while (sc0.c_x() > last_space && tp > text) {
                c_putch('\b');
                utf8_int32_t dummy;
                tp = (const char *)utf8rcodepoint(tp, &dummy);
            }
            c_putch('\n');
            while (sc0.c_x() < indent)
                c_putch(' ');
            if (*tp == ' ')
                ++tp;
        }

        utf8codepoint(tp, &c);
        if (c == '\n') {
            c_putch(c);
            while (sc0.c_x() < indent)
                c_putch(' ');
        } else {
            c_putch(c);
        }
        tp += utf8codepointsize(c);
    }

    c_putch('\n');
}

static void print_help(const struct help_t *h) {
    pixel_t saved_fg_color = sc0.getFgColor();
    pixel_t saved_bg_color = sc0.getBgColor();

    c_printf("\n\\Fc%s ", h->command);
    c_printf("\\Fl(%s, %s)\\Ff\n\n", _(type_names[h->type]), _(section_names[h->section]));
    print_wrapped(h->brief);

    if (h->usage) {
        c_puts("\n\\Fk");
        c_puts(_("Usage:"));
        c_puts("\\Fn\n\n  ");
        print_wrapped(h->usage);
        c_puts("\\Fk");
    }

    if (h->args[0].name) {
        c_puts("\n\\Fk");
        c_puts(_("Arguments:"));
        c_puts("\n\n");
        for (int i = 0; h->args[i].name; ++i) {
            c_printf("  \\FL%s\t\\Ff", h->args[i].name);
            print_wrapped(_(h->args[i].descr));
        }
        sc0.setColor(COL(FG), COL(BG));
    }

    if (h->ret) {
        c_puts("\n\\Fk");
        c_puts(_("Return value:"));
        c_puts("\\Ff\n\n  ");
        print_wrapped(_(h->ret));
    }

    if (h->ret) {
        c_puts("\n\\Fk");
        c_puts(_("Types:"));
        c_puts("\\Ff\n\n  ");
        print_wrapped(_(h->types));
    }

    if (h->ret) {
        c_puts("\n\\Fk");
        c_puts(_("Handler:"));
        c_puts("\\Ff\n\n  ");
        print_wrapped(_(h->handler));
    }

    if (h->desc) {
        c_puts("\n\\Fk");
        c_puts(_("Description:"));
        c_puts("\\Ff\n\n  ");
        print_wrapped(_(h->desc));
    }

    if (h->note) {
        c_puts("\n\\Fk");
        c_puts(_("Note:"));
        c_puts("\\Ff\n\n  ");
        print_wrapped(_(h->note));
    }

    if (h->bugs) {
        c_puts("\n\\Fk");
        c_puts(_("Bugs:"));
        c_puts("\\Ff\n\n  ");
        print_wrapped(_(h->bugs));
    }

    if (h->ref[0]) {
        c_puts("\n\\Fk");
        c_puts(_("See also:"));
        c_puts("\\Ff\n\n");
        for (int i = 0; h->ref[i]; ++i) {
            c_printf("  HELP %s\n", h->ref[i]);
        }
    }

    c_putch('\n');

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

    bool found_something = false;
    screen_putch_paging_counter = 0;

    if (tokens.length() == 0) {
        c_puts("\n\\Fk");
        c_puts(_("Available commands:"));
        c_puts("\\Ff\n\n");
        for (int i = 0; help[i].command; ++i) {
            c_printf("%s\t", help[i].command);
        }
        c_putch('\n');
        goto out;
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
            found_something = true;
        }
    }

    if (found_something)
        goto out;

    if (hints.length() > 0) {
        pixel_t saved_fg_color = sc0.getFgColor();
        pixel_t saved_bg_color = sc0.getBgColor();

        c_puts("\n\\Fk");
        c_puts(_("Matching commands:"));
        c_puts("\\Ff\n\n");
        for (int i = 0; i < hints.length(); ++i) {
            if (hints[i]->command) {
                c_printf("  HELP %s\n", hints[i]->command);
            }
        }

        c_puts("\n\\Fk");
        c_puts(_("See also:"));
        c_puts("\\Ff\n\n");
        for (int i = 0; i < hints.length(); ++i) {
            if (hints[i]->ref[0]) {
                for (int j = 0; hints[i]->ref[j]; ++j) {
                    c_printf("  HELP %s\n", hints[i]->ref[j]);
                }
            }
        }
        c_putch('\n');
        sc0.setColor(saved_fg_color, saved_bg_color);
    } else {
        for (int i = 0; i < tokens.length(); ++i) {
            if ((tokens[i] < 256  && !kwtbl[tokens[i]]) ||
                (tokens[i] >= 256 && !kwtbl_ext[tokens[i] - 256])) {
                err = ERR_UNK;
                goto out;
            }
        }
        c_printf(_("No help available\n"));
    }

out:
    screen_putch_paging_counter = -1;
}
