// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "eb_io.h"
#include "lua_defs.h"

static int l_gpio_set_pin(lua_State *l) {
  uint32_t portno = luaL_checkinteger(l, 1);
  uint32_t pinno = luaL_checkinteger(l, 2);
  uint32_t data = luaL_checkinteger(l, 3);

  lua_pushret(l, eb_gpio_set_pin(portno, pinno, data));
  return 1;
}

static int l_gpio_get_pin(lua_State *l) {
  uint32_t portno = luaL_checkinteger(l, 1);
  uint32_t pinno = luaL_checkinteger(l, 2);

  lua_pushret(l, eb_gpio_get_pin(portno, pinno));
  return 1;
}

static int l_gpio_set_pin_mode(lua_State *l) {
  uint32_t portno = luaL_checkinteger(l, 1);
  uint32_t pinno = luaL_checkinteger(l, 2);
  uint32_t mode = luaL_checkinteger(l, 3);

  lua_pushret(l, eb_gpio_set_pin_mode(portno, pinno, mode));
  return 1;
}

int luaopen_hwio(lua_State *l) {
  lua_pushcfunction(l, l_gpio_set_pin);
  lua_setglobal_P(l, "gpio_set_pin");
  lua_pushcfunction(l, l_gpio_get_pin);
  lua_setglobal_P(l, "gpio_get_pin");
  lua_pushcfunction(l, l_gpio_set_pin_mode);
  lua_setglobal_P(l, "gpio_get_pin_mode");

  return 0;
}
