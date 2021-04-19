// SPDX-License-Identifier: MIT
// Copyright (c) 2021 Ulrich Hecht

#include "basic.h"
#include "eb_bg.h"
#include "lua_defs.h"

#ifdef USE_BG_ENGINE

static int l_bg_set_size(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);
  int32_t x = luaL_checknumber(l, 2);
  int32_t y = luaL_checknumber(l, 3);

  lua_pushret(l, eb_bg_set_size(bg, x, y));
  return 1;
}

static int l_bg_set_pattern(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);
  int32_t x = luaL_checknumber(l, 2);
  int32_t y = luaL_checknumber(l, 3);
  int32_t w = luaL_checknumber(l, 4);

  eb_bg_set_pattern(bg, x, y, w);
  return 0;
}

static int l_bg_set_tile_size(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);
  int32_t x = luaL_checknumber(l, 2);
  int32_t y = luaL_checknumber(l, 3);

  eb_bg_set_tile_size(bg, x, y);
  return 0;
}

static int l_bg_set_priority(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);
  int32_t p = luaL_checknumber(l, 2);

  eb_bg_set_priority(bg, p);
  return 0;
}

static int l_bg_enable(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);

  lua_pushret(l, eb_bg_enable(bg));
  return 1;
}

static int l_bg_disable(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);

  lua_pushret(l, eb_bg_disable(bg));
  return 1;
}

static int l_bg_off(lua_State *l) {
  eb_bg_off();
  return 0;
}

static int l_bg_load(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);
  const char *file = luaL_checkstring(l, 2);

  lua_pushret(l, eb_bg_load(bg, file));
  return 1;
}

static int l_bg_save(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);
  const char *file = luaL_checkstring(l, 2);

  lua_pushret(l, eb_bg_save(bg, file));
  return 1;
}

static int l_bg_move(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);
  int32_t x = luaL_checknumber(l, 2);
  int32_t y = luaL_checknumber(l, 3);

  eb_bg_move(bg, x, y);
  return 0;
}

static int l_bg_map_tile(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);
  int32_t from = luaL_checknumber(l, 2);
  int32_t to = luaL_checknumber(l, 3);

  lua_pushret(l, eb_bg_map_tile(bg, from, to));
  return 1;
}

// XXX: bg_set_tiles, bg_get_tiles

static int l_bg_set_tile(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);
  int32_t x = luaL_checknumber(l, 2);
  int32_t y = luaL_checknumber(l, 3);
  int32_t t = luaL_checknumber(l, 4);

  lua_pushret(l, eb_bg_set_tile(bg, x, y, t));
  return 1;
}

static int l_bg_x(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);

  lua_pushret(l, eb_bg_x(bg));
  return 1;
}

static int l_bg_y(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);

  lua_pushret(l, eb_bg_y(bg));
  return 1;
}

static int l_bg_win_x(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);

  lua_pushret(l, eb_bg_win_x(bg));
  return 1;
}

static int l_bg_win_y(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);

  lua_pushret(l, eb_bg_win_y(bg));
  return 1;
}

static int l_bg_win_width(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);

  lua_pushret(l, eb_bg_win_width(bg));
  return 1;
}

static int l_bg_win_height(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);

  lua_pushret(l, eb_bg_win_height(bg));
  return 1;
}

static int l_bg_enabled(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);

  lua_pushret(l, eb_bg_enabled(bg));
  return 1;
}

static int l_bg_set_window(lua_State *l) {
  int32_t bg = luaL_checknumber(l, 1);
  int32_t x = luaL_checknumber(l, 2);
  int32_t y = luaL_checknumber(l, 3);
  int32_t w = luaL_checknumber(l, 4);
  int32_t h = luaL_checknumber(l, 5);

  lua_pushret(l, eb_bg_set_window(bg, x, y, w, h));
  return 1;
}


static int l_sprite_off(lua_State *l) {
  vs23.resetSprites();
  return 0;
}

static int l_sprite_set_pattern(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);
  int32_t x = luaL_checknumber(l, 2);
  int32_t y = luaL_checknumber(l, 3);

  lua_pushret(l, eb_sprite_set_pattern(s, x, y));
  return 1;
}

static int l_sprite_set_size(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);
  int32_t w = luaL_checknumber(l, 2);
  int32_t h = luaL_checknumber(l, 3);

  lua_pushret(l, eb_sprite_set_size(s, w, h));
  return 1;
}

static int l_sprite_enable(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);

  lua_pushret(l, eb_sprite_enable(s));
  return 1;
}

static int l_sprite_disable(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);

  lua_pushret(l, eb_sprite_disable(s));
  return 1;
}

static int l_sprite_set_key(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);
  ipixel_t key = luaL_checknumber(l, 2);

  lua_pushret(l, eb_sprite_set_key(s, key));
  return 1;
}

static int l_sprite_set_priority(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);
  int32_t prio = luaL_checknumber(l, 2);

  lua_pushret(l, eb_sprite_set_priority(s, prio));
  return 1;
}

static int l_sprite_set_frame(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);
  int32_t frame_x = luaL_checknumber(l, 2);
  int32_t frame_y = luaL_checknumber(l, 3);
  luaL_checktype(l, 4, LUA_TBOOLEAN);
  bool flip_x = lua_toboolean(l, 4);
  luaL_checktype(l, 5, LUA_TBOOLEAN);
  bool flip_y = lua_toboolean(l, 5);

  lua_pushret(l, eb_sprite_set_frame(s, frame_x, frame_y, flip_x, flip_y));
  return 1;
}

static int l_sprite_set_opacity(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);
  luaL_checktype(l, 2, LUA_TBOOLEAN);
  bool onoff = lua_toboolean(l, 2);

  lua_pushret(l, eb_sprite_set_opacity(s, onoff));
  return 1;
}

static int l_sprite_set_angle(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);
  double angle = luaL_checknumber(l, 2);

  eb_sprite_set_angle(s, angle);
  return 0;
}

static int l_sprite_set_scale_x(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);
  double scale_x = luaL_checknumber(l, 2);

  eb_sprite_set_scale_x(s, scale_x);
  return 0;
}

static int l_sprite_set_scale_y(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);
  double scale_y = luaL_checknumber(l, 2);

  eb_sprite_set_scale_y(s, scale_y);
  return 0;
}

static int l_sprite_set_alpha(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);
  int32_t alpha = luaL_checkinteger(l, 2);

  eb_sprite_set_alpha(s, alpha);
  return 0;
}

static int l_sprite_reload(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);

  lua_pushret(l, eb_sprite_reload(s));
  return 1;
}

static int l_sprite_move(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);
  int32_t x = luaL_checknumber(l, 2);
  int32_t y = luaL_checknumber(l, 3);

  lua_pushret(l, eb_sprite_move(s, x, y));
  return 1;
}

static int l_sprite_tile_collision(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);
  int32_t bg = luaL_checknumber(l, 2);
  int32_t tile = luaL_checknumber(l, 3);

  lua_pushret(l, eb_sprite_tile_collision(s, bg, tile));
  return 1;
}

static int l_sprite_collision(lua_State *l) {
  int32_t s1 = luaL_checknumber(l, 1);
  int32_t s2 = luaL_checknumber(l, 2);

  lua_pushret(l, eb_sprite_collision(s1, s2));
  return 1;
}

static int l_sprite_enabled(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);

  lua_pushboolean(l, eb_sprite_enabled(s));
  return 1;
}

static int l_sprite_x(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);

  lua_pushinteger(l, eb_sprite_x(s));
  return 1;
}

static int l_sprite_y(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);

  lua_pushinteger(l, eb_sprite_y(s));
  return 1;
}

static int l_sprite_w(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);

  lua_pushinteger(l, eb_sprite_w(s));
  return 1;
}

static int l_sprite_h(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);

  lua_pushinteger(l, eb_sprite_h(s));
  return 1;
}

static int l_sprite_frame_x(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);

  lua_pushinteger(l, eb_sprite_frame_x(s));
  return 1;
}

static int l_sprite_frame_y(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);

  lua_pushinteger(l, eb_sprite_frame_y(s));
  return 1;
}

static int l_sprite_flip_x(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);

  lua_pushinteger(l, eb_sprite_flip_x(s));
  return 1;
}

static int l_sprite_flip_y(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);

  lua_pushinteger(l, eb_sprite_flip_y(s));
  return 1;
}

static int l_sprite_opaque(lua_State *l) {
  int32_t s = luaL_checknumber(l, 1);

  lua_pushinteger(l, eb_sprite_opaque(s));
  return 1;
}

static int l_frameskip(lua_State *l) {
  int32_t skip = luaL_checknumber(l, 1);

  lua_pushinteger(l, eb_frameskip(skip));
  return 1;
}

int luaopen_bg(lua_State *l) {
  lua_pushcfunction(l, l_bg_set_size);
  lua_setglobal_P(l, "bg_set_size");
  lua_pushcfunction(l, l_bg_set_pattern);
  lua_setglobal_P(l, "bg_set_pattern");
  lua_pushcfunction(l, l_bg_set_tile_size);
  lua_setglobal_P(l, "bg_set_tile_size");
  lua_pushcfunction(l, l_bg_set_priority);
  lua_setglobal_P(l, "bg_set_priority");
  lua_pushcfunction(l, l_bg_set_window);
  lua_setglobal_P(l, "bg_set_window");
  lua_pushcfunction(l, l_bg_enable);
  lua_setglobal_P(l, "bg_enable");
  lua_pushcfunction(l, l_bg_disable);
  lua_setglobal_P(l, "bg_disable");
  lua_pushcfunction(l, l_bg_off);
  lua_setglobal_P(l, "bg_off");
  lua_pushcfunction(l, l_bg_load);
  lua_setglobal_P(l, "bg_load");
  lua_pushcfunction(l, l_bg_save);
  lua_setglobal_P(l, "bg_save");
  lua_pushcfunction(l, l_bg_move);
  lua_setglobal_P(l, "bg_move");
  lua_pushcfunction(l, l_bg_map_tile);
  lua_setglobal_P(l, "bg_map_tile");
  lua_pushcfunction(l, l_bg_set_tile);
  lua_setglobal_P(l, "bg_set_tile");
  lua_pushcfunction(l, l_bg_x);
  lua_setglobal_P(l, "bg_x");
  lua_pushcfunction(l, l_bg_y);
  lua_setglobal_P(l, "bg_y");
  lua_pushcfunction(l, l_bg_win_x);
  lua_setglobal_P(l, "bg_win_x");
  lua_pushcfunction(l, l_bg_win_y);
  lua_setglobal_P(l, "bg_win_y");
  lua_pushcfunction(l, l_bg_win_width);
  lua_setglobal_P(l, "bg_win_width");
  lua_pushcfunction(l, l_bg_win_height);
  lua_setglobal_P(l, "bg_win_height");
  lua_pushcfunction(l, l_bg_enabled);
  lua_setglobal_P(l, "bg_enabled");
  lua_pushcfunction(l, l_sprite_off);
  lua_setglobal_P(l, "sprite_off");
  lua_pushcfunction(l, l_sprite_set_pattern);
  lua_setglobal_P(l, "sprite_set_pattern");
  lua_pushcfunction(l, l_sprite_set_size);
  lua_setglobal_P(l, "sprite_set_size");
  lua_pushcfunction(l, l_sprite_enable);
  lua_setglobal_P(l, "sprite_enable");
  lua_pushcfunction(l, l_sprite_disable);
  lua_setglobal_P(l, "sprite_disable");
  lua_pushcfunction(l, l_sprite_set_key);
  lua_setglobal_P(l, "sprite_set_key");
  lua_pushcfunction(l, l_sprite_set_priority);
  lua_setglobal_P(l, "sprite_set_priority");
  lua_pushcfunction(l, l_sprite_set_frame);
  lua_setglobal_P(l, "sprite_set_frame");
  lua_pushcfunction(l, l_sprite_set_opacity);
  lua_setglobal_P(l, "sprite_set_opacity");
  lua_pushcfunction(l, l_sprite_reload);
  lua_setglobal_P(l, "sprite_reload");
  lua_pushcfunction(l, l_sprite_set_angle);
  lua_setglobal_P(l, "sprite_set_angle");
  lua_pushcfunction(l, l_sprite_set_scale_x);
  lua_setglobal_P(l, "sprite_set_scale_x");
  lua_pushcfunction(l, l_sprite_set_scale_y);
  lua_setglobal_P(l, "sprite_set_scale_y");
  lua_pushcfunction(l, l_sprite_set_alpha);
  lua_setglobal_P(l, "sprite_set_alpha");
  lua_pushcfunction(l, l_sprite_move);
  lua_setglobal_P(l, "sprite_move");
  lua_pushcfunction(l, l_sprite_tile_collision);
  lua_setglobal_P(l, "sprite_tile_collision");
  lua_pushcfunction(l, l_sprite_collision);
  lua_setglobal_P(l, "sprite_collision");
  lua_pushcfunction(l, l_sprite_enabled);
  lua_setglobal_P(l, "sprite_enabled");
  lua_pushcfunction(l, l_sprite_x);
  lua_setglobal_P(l, "sprite_x");
  lua_pushcfunction(l, l_sprite_y);
  lua_setglobal_P(l, "sprite_y");
  lua_pushcfunction(l, l_sprite_w);
  lua_setglobal_P(l, "sprite_w");
  lua_pushcfunction(l, l_sprite_h);
  lua_setglobal_P(l, "sprite_h");
  lua_pushcfunction(l, l_sprite_frame_x);
  lua_setglobal_P(l, "sprite_frame_x");
  lua_pushcfunction(l, l_sprite_frame_y);
  lua_setglobal_P(l, "sprite_frame_y");
  lua_pushcfunction(l, l_sprite_flip_x);
  lua_setglobal_P(l, "sprite_flip_x");
  lua_pushcfunction(l, l_sprite_flip_y);
  lua_setglobal_P(l, "sprite_flip_y");
  lua_pushcfunction(l, l_sprite_opaque);
  lua_setglobal_P(l, "sprite_opaque");
  lua_pushcfunction(l, l_frameskip);
  lua_setglobal_P(l, "frameskip");

  return 0;
}

#endif
