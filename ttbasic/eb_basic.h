// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#ifndef _EB_BASIC_H
#define _EB_BASIC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "kwenum.h"

typedef struct {
    int type;
    union {
        double num;
        char *str;
    };
} eb_param_t;

typedef double (*eb_numfun_handler_t)(const eb_param_t *params);
typedef void (*eb_command_handler_t)(const eb_param_t *params);
typedef const char *(*eb_strfun_handler_t)(const eb_param_t *params);

int eb_add_command(const char *name, enum token_t *syntax, eb_command_handler_t handler);
int eb_add_numfun(const char *name, enum token_t *syntax, eb_numfun_handler_t handler);
int eb_add_strfun(const char *name, enum token_t *syntax, eb_strfun_handler_t handler);

const char **eb_kwtbl(void);
int eb_kwtbl_size(void);

int eb_exec_basic(void *bc, const char *filename);
void *eb_new_basic_context(void);
void eb_delete_basic_context(void *bc);

#ifdef __cplusplus
}
#endif

#endif
