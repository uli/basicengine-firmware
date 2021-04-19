// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#include "basic.h"
#include "lua_defs.h"
#include "eb_input.h"

static int l_inkey(lua_State *l) {
  lua_pushret(l, eb_inkey());
  return 1;
}

static int l_pad_state(lua_State *l) {
  int32_t num = luaL_checkinteger(l, 1);
  lua_pushinteger(l, eb_pad_state(num));
  return 1;
}

static int l_key_state(lua_State *l) {
  int32_t scancode = luaL_checkinteger(l, 1);
  lua_pushinteger(l, eb_key_state(scancode));
  return 1;
}

int luaopen_input(lua_State *l) {
  lua_pushcfunction(l, l_inkey);
  lua_setglobal(l, "inkey");
  lua_pushcfunction(l, l_pad_state);
  lua_setglobal(l, "pad_state");
  lua_pushcfunction(l, l_key_state);
  lua_setglobal(l, "key_state");
  lua_pushinteger(l, joyUp);
  lua_setglobal(l, "PAD_UP");
  lua_pushinteger(l, joyDown);
  lua_setglobal(l, "PAD_DOWN");
  lua_pushinteger(l, joyLeft);
  lua_setglobal(l, "PAD_LEFT");
  lua_pushinteger(l, joyRight);
  lua_setglobal(l, "PAD_RIGHT");
  return 0;
}
