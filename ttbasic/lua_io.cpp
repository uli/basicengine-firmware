// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "eb_io.h"
#include "lua_defs.h"
#include "ltable.h"
#include <stdint.h>

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

static int l_i2c_write(lua_State *l) {
  int addr = luaL_checkinteger(l, 1);

  luaL_checktype(l, 2, LUA_TTABLE);
  int count = lua_rawlen(l, 2);

  char out_data[count];
  for (int i = 1; i <= count; ++i) {
    lua_rawgeti(l, 2, i);
    out_data[i - 1] = lua_tointeger(l, -1);
    lua_pop(l, 1);
    //printf("%02X ", out_data[i]);
  }

  eb_i2c_write(addr, out_data, count);

  return 0;
}

static int l_i2c_read(lua_State *l) {
  int addr = luaL_checkinteger(l, 1);
  int count = luaL_checkinteger(l, 2);

  char in_data[count];

  if (eb_i2c_read(addr, in_data, count) != 0) {
    lua_pushnil(l);
  } else {
    lua_newtable(l);
    for (int i = 1; i <= count; ++i) {
      lua_pushinteger(l, i);
      lua_pushinteger(l, in_data[i - 1]);
      lua_rawset(l, -3);
    }
  }

  return 1;
}

static int l_spi_write(lua_State *l) {
  luaL_checktype(l, 1, LUA_TTABLE);
  int count = lua_rawlen(l, 1);

  char out_data[count];
  for (int i = 1; i <= count; ++i) {
    lua_rawgeti(l, 1, i);
    out_data[i - 1] = lua_tointeger(l, -1);
    lua_pop(l, 1);
    //printf("%02X ", out_data[i]);
  }

  eb_spi_write(out_data, count);

  return 0;
}

static int l_spi_transfer(lua_State *l) {
  luaL_checktype(l, 1, LUA_TTABLE);
  int count = lua_rawlen(l, 1);

  char out_data[count];
  for (int i = 1; i <= count; ++i) {
    lua_rawgeti(l, 1, i);
    out_data[i - 1] = lua_tointeger(l, -1);
    lua_pop(l, 1);
    //printf("%02X ", out_data[i]);
  }

  char in_data[count];
  eb_spi_transfer(out_data, in_data, count);

  lua_newtable(l);
  for (int i = 1; i <= count; ++i) {
    lua_pushinteger(l, i);
    lua_pushinteger(l, in_data[i - 1]);
    lua_rawset(l, -3);
  }

  return 1;
}

int luaopen_hwio(lua_State *l) {
  lua_pushcfunction(l, l_gpio_set_pin);
  lua_setglobal(l, "gpio_set_pin");
  lua_pushcfunction(l, l_gpio_get_pin);
  lua_setglobal(l, "gpio_get_pin");
  lua_pushcfunction(l, l_gpio_set_pin_mode);
  lua_setglobal(l, "gpio_get_pin_mode");
  lua_pushcfunction(l, l_i2c_write);
  lua_setglobal(l, "i2c_write");
  lua_pushcfunction(l, l_i2c_read);
  lua_setglobal(l, "i2c_read");
  lua_pushcfunction(l, l_spi_write);
  lua_setglobal(l, "spi_write");
  lua_pushcfunction(l, l_spi_transfer);
  lua_setglobal(l, "spi_transfer");

  return 0;
}
