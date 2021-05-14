// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#ifndef _HELP_H
#define _HELP_H

#include "basic.h"

enum section_t {
    SECTION_BAS,
    SECTION_M,
    SECTION_OP,
    SECTION_SOP,
    SECTION_IO,
    SECTION_FS,
    SECTION_BG,
    SECTION_PIX,
    SECTION_SCR,
    SECTION_SND,
    SECTION_SYS,
};

enum type_t {
    TYPE_BC,
    TYPE_BF,
    TYPE_BN,
    TYPE_BO,
};

struct arg_t {
    const char *name;
    const char *descr;
};

struct help_t {
    const char *command;
    const int tokens[4];
    const enum section_t section;
    const enum type_t type;
    const char *brief;
    const char *usage;
    struct arg_t args[16];
    const char *ret;
    const char *types;
    const char *handler;
    const char *desc;
    const char *fonts;
    const char *options;
    const char *note;
    const char *bugs;
    const char *ref[8];
};

extern const struct help_t help_en[];
extern const struct help_t help_de[];
extern const struct help_t help_fr[];
extern const struct help_t help_es[];

#define XSTR(x) STR(x)
#define STR(x) #x

#endif
