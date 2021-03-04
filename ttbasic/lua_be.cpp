// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#include "basic.h"
#include "lua_defs.h"

static int l_cls(lua_State *l) {
  sc0.cls();
  sc0.locate(0, 0);
  return 0;
}

static int l_basic(lua_State *l) {
  luaL_error(l, "return to basic");
  return 0;
}

int luaopen_be(lua_State *l) {
  lua_pushcfunction(l, l_cls);
  lua_setglobal_P(l, "cls");
  lua_pushcfunction(l, l_basic);
  lua_setglobal_P(l, "basic");
  return 0;
}

void do_lua_line(lua_State *lua, const char *lbuf) {
  BString retline("return ");
  retline += lbuf;
  retline += ";";
  if ((luaL_loadstring(lua, retline.c_str()) != LUA_OK &&luaL_loadstring(lua, lbuf) != LUA_OK) ||
      lua_pcall(lua, 0, LUA_MULTRET, 0) != LUA_OK) {
    const char *err_str = lua_tostring(lua, -1);
    if (strstr_P(err_str, PSTR("return to basic"))) {
      lua_close(lua);
      lua = NULL;
    } else {
      PRINT_P("error: ");
      c_puts(err_str);
      newline();
    }
  } else {
    int retvals = lua_gettop(lua);
    if (retvals > 0) {
      luaL_checkstack(lua, LUA_MINSTACK, "too many results to print");
      lua_getglobal(lua, "print");
      lua_insert(lua, 1);
      if (lua_pcall(lua, retvals, 0, 0) != LUA_OK) {
        PRINT_P("error calling 'print' (");
        c_puts(lua_tostring(lua, -1));
        c_putch(')');
        newline();
      }
    }
  }
}

