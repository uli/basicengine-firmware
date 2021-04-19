// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2021 Ulrich Hecht

#ifndef __LUA_DEFS_H
#define __LUA_DEFS_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

extern lua_State *lua;

int luaopen_be(lua_State *l);
int luaopen_bg(lua_State *l);
int luaopen_img(lua_State *l);
int luaopen_input(lua_State *l);
int luaopen_video(lua_State *l);
int luaopen_hwio(lua_State *l);

static void lua_pushret(lua_State *l, int ret) {
    if (ret)
        lua_pushnil(l);
    else
        lua_pushboolean(l, 1);
}

void do_lua_line(const char *lbuf);

#endif
