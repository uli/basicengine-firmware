// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2021 Ulrich Hecht

#ifndef __LUA_DEFS_H
#define __LUA_DEFS_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

extern lua_State *lua;

int luaopen_be(lua_State *l);
int luaopen_video(lua_State *l);

static void lua_pushret(lua_State *l, int ret) {
    if (ret)
        lua_pushnil(l);
    else
        lua_pushboolean(l, 1);
}

void do_lua_line(lua_State *lua, const char *lbuf);

#endif
