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
