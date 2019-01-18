#ifndef __LUA_DEFS_H
#define __LUA_DEFS_H

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

extern lua_State *lua;

int luaopen_be(lua_State *l);

#endif
