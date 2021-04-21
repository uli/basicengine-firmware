// SPDX-License-Identifier: MIT
// Copyright (c) 2019 Ulrich Hecht

#include "basic.h"
#include "lua_defs.h"
#include "eb_img.h"

static int l_load_image(lua_State *l) {
  const char *file = luaL_checkstring(l, 1);
  uint32_t key = 0;
  int32_t dx = -1, dy = -1;
  int32_t sx = 0, sy = 0;
  int32_t w = -1, h = -1;

  if (lua_gettop(l) > 1) {
    key = luaL_checkinteger(l, 2);
  }
  if (lua_gettop(l) > 2) {
    dx = luaL_checkinteger(l, 3);
    dy = luaL_checkinteger(l, 4);
  }
  if (lua_gettop(l) > 4) {
    sx = luaL_checkinteger(l, 5);
    sy = luaL_checkinteger(l, 6);
  }
  if (lua_gettop(l) > 6) {
    w = luaL_checkinteger(l, 7);
    h = luaL_checkinteger(l, 8);
  }

  struct eb_image_spec loc = {
    .dst_x = dx,
    .dst_y = dy,
    .src_x = sx,
    .src_y = sy,
    .w = w,
    .h = h,
    .scale_x = 1,
    .scale_y = 1,
    .key = key,
  };

  int ret = eb_load_image(file, &loc);

  if (ret == 0) {
    lua_pushinteger(l, loc.dst_x);
    lua_pushinteger(l, loc.dst_y);
    lua_pushinteger(l, loc.w);
    lua_pushinteger(l, loc.h);
  } else {
    lua_pushnil(l);
    lua_pushnil(l);
    lua_pushnil(l);
    lua_pushnil(l);
  }

  return 4;
}

int luaopen_img(lua_State *l) {
  lua_pushcfunction(l, l_load_image);
  lua_setglobal(l, "load_image");
  return 0;
}
