// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "eb_basic.h"
#include "eb_basic_int.h"
#include "basic.h"
#include <vector>

struct eb_command_t {
    token_t token;
    token_t *syntax;
    eb_command_handler_t handler;
};

struct eb_numfun_t {
    token_t token;
    token_t *syntax;
    eb_numfun_handler_t handler;
};

struct eb_strfun_t {
    token_t token;
    token_t *syntax;
    eb_strfun_handler_t handler;
};

std::vector<struct eb_command_t> custom_commands;
std::vector<struct eb_numfun_t> custom_numfuns;
std::vector<struct eb_strfun_t> custom_strfuns;

int eb_add_command(const char *name, token_t *syntax, eb_command_handler_t handler) {
    eb_command_t cmd = { (token_t)kwtbl.size(), syntax, handler };
    custom_commands.push_back(cmd);
    kwtbl.push_back(strdup(name));
    return 0;
}

int eb_add_numfun(const char *name, token_t *syntax, eb_numfun_handler_t handler) {
    eb_numfun_t nf = { (token_t)kwtbl.size(), syntax, handler };
    custom_numfuns.push_back(nf);
    kwtbl.push_back(strdup(name));
    return 0;
}

int eb_add_strfun(const char *name, token_t *syntax, eb_strfun_handler_t handler) {
    eb_strfun_t sf = { (token_t)kwtbl.size(), syntax, handler };
    custom_strfuns.push_back(sf);
    kwtbl.push_back(strdup(name));
    return 0;
}

token_t *eb_command_syntax(token_t token) {
    for (auto cmd : custom_commands) {
        if (cmd.token == token)
            return cmd.syntax;
    }
    return NULL;
}

token_t *eb_numfun_syntax(token_t token) {
    for (auto nf : custom_numfuns) {
        if (nf.token == token)
            return nf.syntax;
    }
    return NULL;
}

token_t *eb_strfun_syntax(token_t token) {
    for (auto sf : custom_strfuns) {
        if (sf.token == token)
            return sf.syntax;
    }
    return NULL;
}

void eb_handle_command(enum token_t token, const eb_param_t *params) {
    for (auto cmd : custom_commands) {
        if (cmd.token == token) {
            cmd.handler(params);
            return;
        }
    }
    err = ERR_SYS;
}

num_t eb_handle_numfun(enum token_t token, const eb_param_t *params) {
    for (auto nf : custom_numfuns) {
        if (nf.token == token) {
            return nf.handler(params);
        }
    }
    err = ERR_SYS;
    return 0;
}

BString eb_handle_strfun(enum token_t token, const eb_param_t *params) {
    for (auto sf : custom_strfuns) {
        if (sf.token == token) {
            return sf.handler(params);
        }
    }
    err = ERR_SYS;
    return BString();
}

const char **eb_kwtbl(void) {
    return &kwtbl[0];
}

int eb_kwtbl_size(void) {
    return kwtbl.size();
}
