// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "basic.h"

const enum token_t *eb_command_syntax(enum token_t token);
const enum token_t *eb_numfun_syntax(enum token_t token);
const enum token_t *eb_strfun_syntax(enum token_t token);

void eb_handle_command(enum token_t token, const eb_param_t *params);
num_t eb_handle_numfun(enum token_t token, const eb_param_t *params);
BString eb_handle_strfun(enum token_t token, const eb_param_t *params);
