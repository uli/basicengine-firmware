// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#ifndef __LUA_DEFS_H
#define __LUA_DEFS_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

extern lua_State *lua;

int luaopen_be(lua_State *l);
int luaopen_video(lua_State *l);

#endif
