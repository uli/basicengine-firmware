// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "help.h"
#include "config.h"
#include "eb_file.h"
#include <fonts.h>
#include <utf8.h>

#define SJSON_IMPLEMENT
#include "sjson.h"

#define __(s) (s)
struct {
    const char *abbrev;
    const char *name;
} section_names[] = {
    { "bas", __("BASIC core") },
    { "m", __("math") },
    { "op", __("numeric") },
    { "sop", __("string") },
    { "io", __("input/output") },
    { "fs", __("file system") },
    { "bg", __("sprite/background engine") },
    { "pix", __("pixel graphics") },
    { "scr", __("screen handling") },
    { "snd", __("sound") },
    { "sys", __("system") },
};

static const char *section_name(const char *abbrev) {
    if (abbrev) for (auto s : section_names) {
        if (!strcmp(abbrev, s.abbrev))
            return s.name;
    }
    return _("unknown section");
}

struct {
    const char *abbrev;
    const char *name;
} type_names [] = {
    { "bc", __("command") },
    { "bf", __("function") },
    { "bn", __("constant") },
    { "bo", __("operator") },
};

static const char *type_name(const char *abbrev) {
    if (abbrev) for (auto s : type_names) {
        if (!strcmp(abbrev, s.abbrev))
            return s.name;
    }
    return _("unknown type");
}

struct {
    const char *abbrev;
    const char *name;
} item_names [] = {
    { "usage", __("Usage") },
    { "args", __("Arguments") },
    { "ret", __("Return value") },
    { "type", __("Types") },
    { "handler", __("Handler") },
    { "desc", __("Description") },
    { "fonts", __("Fonts") },
    { "options", __("Options") },
    { "note", __("Note") },
    { "bugs", __("Bugs") },
    { "ref", __("See also") },
    { "expressions", __("Expressions") },
    { "modes", __("Modes") },
};

static const char *item_name(const char *abbrev) {
    if (abbrev) for (auto s : item_names) {
        if (!strcmp(abbrev, s.abbrev))
            return s.name;
    }
    return _(abbrev);
}

#undef __

static struct {
    const char *name;
    int value;
} constants[] = {
    { "CONFIG_COLS", CONFIG_COLS },
    { "MAX_BG", MAX_BG },
    { "MAX_PADS", MAX_PADS },
    { "MAX_RETVALS", MAX_RETVALS },
    { "MAX_SPRITE_W", MAX_SPRITE_W },
    { "MAX_SPRITE_H", MAX_SPRITE_H },
    { "MAX_SPRITES", MAX_SPRITES },
    { "MAX_USER_FILES", MAX_USER_FILES },
    { "MML_CHANNELS", MML_CHANNELS },
    { "NUM_FONTS", NUM_FONTS },
};

static BString resolve_macros(BString text) {
    for (auto c : constants) {
        BString pat = BString("{") + BString(c.name) + BString("}");
        char *repl;
        asprintf(&repl, "%d", c.value);
        text.replace(pat, repl);
        pat = BString("{") + BString(c.name) + BString("_m1}");
        asprintf(&repl, "%d", c.value - 1);
        text.replace(pat, repl);
    }
    return text;
}

static void print_wrapped(const char *text) {
    int indent = sc0.c_x();
    int last_space = sc0.c_x();

    BString tps = resolve_macros(BString(text));
    const char *tp = tps.c_str();

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

static void print_help(sjson_node *h) {
    pixel_t saved_fg_color = sc0.getFgColor();
    pixel_t saved_bg_color = sc0.getBgColor();

    c_printf("\n\\Fc%s ", sjson_get_string(h, "command", NULL));

    const char *type = sjson_get_string(h, "type", NULL);
    const char *section = sjson_get_string(h, "section", NULL);
    c_printf("\\Fl(%s, %s)\\Ff\n\n", _(type_name(type)), _(section_name(section)));

    print_wrapped(sjson_get_string(h, "brief", _("missing")));

    sjson_node *item;
    sjson_foreach(item, h) {
        if (!item->key || !strcmp(item->key, "command") || !strcmp(item->key, "type") ||
            !strcmp(item->key, "section") || !strcmp(item->key, "brief") || !strcmp(item->key, "tokens"))
            continue;

        c_puts("\n\\Fk");
        c_puts(_(item_name(item->key)));
        if (!strcmp(item->key, "usage"))
            c_puts("\\Fn\n\n");
        else
            c_puts("\\Ff\n\n");

        if (!strcmp(item->key, "args")) {
            sjson_node *it;
            sjson_foreach(it, item) {
                if (it->tag == SJSON_STRING) {
                    c_printf("  \\FL%s\t\\Ff", it->key);
                    print_wrapped(_(it->string_));
                }
            }
        } else if (!strcmp(item->key, "ref")) {
            sjson_node *r;
            sjson_foreach(r, item) {
                if (r->tag == SJSON_STRING)
                    c_printf("  HELP %s\n", r->string_);
            }
        } else if (item->tag == SJSON_STRING) {
                print_wrapped(item->string_);
        }

        sc0.setColor(COL(FG), COL(BG));

    }

    c_putch('\n');

    sc0.setColor(saved_fg_color, saved_bg_color);
}

#define NUM_LANGS 4

const char *helps[NUM_LANGS] = {
    "en",
    "de",
    "fr",
    "es",
};

sjson_node *helps_json[NUM_LANGS] = {};

static sjson_node *get_json(int lang) {
    if (helps_json[lang])
        return helps_json[lang];

    std::vector<BString> paths;
    paths.push_back("/help/");
#ifdef SDL
    paths.push_back(BString(getenv("ENGINEBASIC_ROOT")) + BString("/help/"));
#else
    paths.push_back(BString("/sd/help/"));
    paths.push_back(BString("/flash/help/"));
    paths.push_back(BString("/usb0/help/"));
#endif

    for (auto p : paths) {
        BString help_file = p + BString("/helptext_");
        if (lang < NUM_LANGS) {
            help_file += BString(helps[lang]);
        } else {
            help_file += BString("en");
            lang = 0;
        }
        help_file += BString(".json");

        sjson_node *root;
        FILE *fp = NULL;
        int size = eb_file_size(help_file.c_str());
        if (size > 0)
            fp = fopen(help_file.c_str(), "r");
        if (size < 0 || !fp) {
            err = ERR_FILE_OPEN;
            err_expected = _("could not open help file");
            continue;
        }

        char json_text[size+1];
        if (fread(json_text, 1, size, fp) != size) {
            err = ERR_FILE_READ;
            err_expected = _("could not read help file");
            fclose(fp);
            continue;
        }
        fclose(fp);
        json_text[size] = 0;

        sjson_context* json_ctx = sjson_create_context(0, 0, NULL);
        root = sjson_decode(json_ctx, json_text);
        if (!root) {
            err = ERR_FORMAT;
            err_expected = _("could not decode help file");
            sjson_destroy_context(json_ctx);
            continue;
        }

        err = ERR_OK;
        return root;
    }
    return NULL;
}

void Basic::ihelp() {
    QList<const char *> tokens;
    QList<sjson_node *> hints;

    sjson_node *root = get_json(CONFIG.language);
    if (!root)
        return;

    while (!end_of_statement()) {
        int token = *cip++;
        if (token == I_EXTEND)
            tokens.push_back(kwtbl_ext[*cip++]);
        else
            tokens.push_back(kwtbl[token]);
    }

    bool found_something = false;
    screen_putch_paging_counter = 0;

    sjson_node *commands = sjson_find_member(root, "commands");
    sjson_node *cmd;

    if (tokens.length() == 0) {
        c_puts("\n\\Fk");
        c_puts(_("Available commands:"));
        c_puts("\\Ff\n\n");
        sjson_foreach(cmd, commands) {
            c_printf("%s\t", sjson_get_string(cmd, "command", NULL));
        }
        c_putch('\n');
        goto out;
    }

    sjson_foreach(cmd, commands) {
        sjson_node *toks = sjson_find_member(cmd, "tokens");
        sjson_node *token;
        int tokens_found = 0;

        int k = 0;
        bool exact_match = true;
        sjson_foreach(token, toks) {
            if (token->tag != SJSON_STRING || k >= tokens.length() || strcmp(token->string_, tokens[k])) {
                exact_match = false;
                break;
            }
            k++;
        }
        if (k < tokens.length())
            exact_match = false;

        if (exact_match) {
            print_help(cmd);
            found_something = true;
        } else {
            for (int k = 0; k < tokens.length(); ++k) {
                sjson_foreach(token, toks) {
                    if (token->tag == SJSON_STRING && !strcmp(token->string_, tokens[k])) {
                        if (hints.indexOf(cmd) == -1)
                            hints.push_back(cmd);
                        break;
                    }
                }
            }
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
            const char *command = sjson_get_string(hints[i], "command", NULL);
            if (command) {
                c_printf("  HELP %s\n", command);
            }
        }

        c_puts("\n\\Fk");
        c_puts(_("See also:"));
        c_puts("\\Ff\n\n");
        for (int i = 0; i < hints.length(); ++i) {
            sjson_node *refs = sjson_find_member(hints[i], "ref");
            sjson_node *ref;
            sjson_foreach(ref, refs) {
                if (ref->tag == SJSON_STRING)
                    c_printf("  HELP %s\n", ref->string_);
            }
        }
        c_putch('\n');
        sc0.setColor(saved_fg_color, saved_bg_color);
    } else {
        c_printf(_("No help available\n"));
    }

out:
    screen_putch_paging_counter = -1;
}
