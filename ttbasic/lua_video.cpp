// SPDX-License-Identifier: MIT
// Copyright (c) 2019-2021 Ulrich Hecht

#include "basic.h"
#include "c_video.h"
#include "lua_defs.h"

static int l_locate(lua_State *l) {
  int32_t x = luaL_checknumber(l, 1);
  int32_t y = luaL_checknumber(l, 2);

  eb_locate(x, y);
  return 0;
}

static int l_window_off(lua_State *l) {
  eb_window_off();
  return 0;
}

static int l_window(lua_State *l) {
  int32_t x = luaL_checknumber(l, 1);
  int32_t y = luaL_checknumber(l, 2);
  int32_t w = luaL_checknumber(l, 3);
  int32_t h = luaL_checknumber(l, 4);

  lua_pushret(l, eb_window(x, y, w, h));
  return 1;
}

static int l_font(lua_State *l) {
  int32_t idx = luaL_checknumber(l, 1);
  lua_pushret(l, eb_font(idx));
  return 1;
}

static int l_screen(lua_State *l) {
  int32_t m = luaL_checknumber(l, 1);
  lua_pushret(l, eb_screen(m));
  return 1;
}

static int l_palette(lua_State *l) {
  int32_t p, hw = -1, sw = -1, vw = -1;
  bool f = false;

  p = luaL_checknumber(l, 1);
  if (lua_gettop(l) > 1) {
    hw = luaL_checknumber(l, 2);
    sw = luaL_checknumber(l, 3);
    vw = luaL_checknumber(l, 4);
    luaL_checktype(l, 5, LUA_TBOOLEAN);
    f = lua_toboolean(l, 5);
  }

  lua_pushret(l, eb_palette(p, hw, sw, vw, f));
  return 0;
}

int l_border(lua_State *l) {
  int32_t x = -1, w = -1;
  int32_t y = luaL_checknumber(l, 1);
  int32_t uv = luaL_checknumber(l, 2);
  if (lua_gettop(l) > 2) {
    x = luaL_checknumber(l, 3);
    w = luaL_checknumber(l, 4);
  }
  eb_border(y, uv, x, w);
  return 0;
}

int l_vsync(lua_State *l) {
  uint32_t tm = 0;
  if (lua_gettop(l) > 0) {
    tm = luaL_checkinteger(l, 1);
  }
  eb_vsync(tm);
  return 0;
}

int l_frame(lua_State *l) {
  lua_pushinteger(l, eb_frame());
  return 1;
}

int l_rgb(lua_State *l) {
  int32_t r = luaL_checknumber(l, 1);
  int32_t g = luaL_checknumber(l, 2);
  int32_t b = luaL_checknumber(l, 3);
  lua_pushinteger(l, eb_rgb(r, g, b));
  return 1;
}

int l_color(lua_State *l) {
  int32_t bgc = 0, cc;
  int32_t fc = luaL_checknumber(l, 1);
  if (lua_gettop(l) > 1) {
    bgc = luaL_checknumber(l, 1);
  }
  if (lua_gettop(l) > 2) {
    cc = luaL_checknumber(l, 2);
    eb_cursor_color(cc);
  }

  eb_color(fc, bgc);
  return 0;
}

int l_csize_height(lua_State *l) {
  lua_pushinteger(l, eb_csize_height());
  return 1;
}

int l_csize_width(lua_State *l) {
  lua_pushinteger(l, eb_csize_width());
  return 1;
}

int l_psize_height(lua_State *l) {
  lua_pushinteger(l, eb_psize_height());
  return 1;
}

int l_psize_width(lua_State *l) {
  lua_pushinteger(l, eb_psize_width());
  return 1;
}

int l_psize_lastline(lua_State *l) {
  lua_pushinteger(l, eb_psize_lastline());
  return 1;
}

int l_pos_x(lua_State *l) {
  lua_pushinteger(l, eb_pos_x());
  return 1;
}

int l_pos_y(lua_State *l) {
  lua_pushinteger(l, eb_pos_y());
  return 1;
}

int l_char_get(lua_State *l) {
  int32_t x = luaL_checknumber(l, 1);
  int32_t y = luaL_checknumber(l, 2);
  lua_pushinteger(l, eb_char_get(x, y));
  return 1;
}

int l_char_set(lua_State *l) {
  int32_t x = luaL_checknumber(l, 1);
  int32_t y = luaL_checknumber(l, 2);
  pixel_t c = luaL_checknumber(l, 3);

  eb_char_set(x, y, c);
  return 0;
}

int l_cscroll(lua_State *l) {
  int32_t x1 = luaL_checknumber(l, 1);
  int32_t y1 = luaL_checknumber(l, 2);
  int32_t x2 = luaL_checknumber(l, 3);
  int32_t y2 = luaL_checknumber(l, 4);
  int32_t d = luaL_checknumber(l, 5);
  lua_pushret(l, eb_cscroll(x1, y1, x2, y2, d));
  return 1;
}

int l_gscroll(lua_State *l) {
  int32_t x1 = luaL_checknumber(l, 1);
  int32_t y1 = luaL_checknumber(l, 2);
  int32_t x2 = luaL_checknumber(l, 3);
  int32_t y2 = luaL_checknumber(l, 4);
  int32_t d = luaL_checknumber(l, 5);
  lua_pushret(l, eb_gscroll(x1, y1, x2, y2, d));
  return 1;
}

int l_point(lua_State *l) {
  int32_t x = luaL_checknumber(l, 1);
  int32_t y = luaL_checknumber(l, 2);
  lua_pushinteger(l, eb_point(x, y));
  return 1;
}

int l_pset(lua_State *l) {
  int32_t x = luaL_checknumber(l, 1);
  int32_t y = luaL_checknumber(l, 2);
  pixel_t c = luaL_checkinteger(l, 3);
  eb_pset(x, y, c);
  return 0;
}

int l_line(lua_State *l) {
  int32_t x1 = luaL_checknumber(l, 1);
  int32_t y1 = luaL_checknumber(l, 2);
  int32_t x2 = luaL_checknumber(l, 3);
  int32_t y2 = luaL_checknumber(l, 4);
  pixel_t c = (pixel_t)-1;
  if (lua_gettop(l) > 4)
    c = luaL_checkinteger(l, 5);
  eb_line(x1, y1, x2, y2, c);
  return 0;
}

int l_circle(lua_State *l) {
  int32_t x = luaL_checknumber(l, 1);
  int32_t y = luaL_checknumber(l, 2);
  int32_t r = luaL_checknumber(l, 3);
  pixel_t c = luaL_checkinteger(l, 4);
  pixel_t f = luaL_checkinteger(l, 5);
  eb_circle(x, y, r, c, f);
  return 0;
}

int l_rect(lua_State *l) {
  int32_t x1 = luaL_checknumber(l, 1);
  int32_t y1 = luaL_checknumber(l, 2);
  int32_t x2 = luaL_checknumber(l, 3);
  int32_t y2 = luaL_checknumber(l, 4);
  pixel_t c = luaL_checkinteger(l, 5);
  pixel_t f = luaL_checkinteger(l, 6);
  eb_rect(x1, y1, x2, y2, c, f);
  return 0;
}

int l_blit(lua_State *l) {
  int32_t x = luaL_checknumber(l, 1);
  int32_t y = luaL_checknumber(l, 2);
  int32_t dx = luaL_checknumber(l, 3);
  int32_t dy = luaL_checkinteger(l, 4);
  int32_t w = luaL_checkinteger(l, 5);
  int32_t h = luaL_checkinteger(l, 6);
  lua_pushret(l, eb_blit(x, y, dx, dy, w, h));
  return 1;
}

int luaopen_video(lua_State *l) {
  lua_pushcfunction(l, l_locate);
  lua_setglobal_P(l, "locate");
  lua_pushcfunction(l, l_window_off);
  lua_setglobal_P(l, "window_off");
  lua_pushcfunction(l, l_window);
  lua_setglobal_P(l, "window");
  lua_pushcfunction(l, l_font);
  lua_setglobal_P(l, "font");
  lua_pushcfunction(l, l_screen);
  lua_setglobal_P(l, "screen");
  lua_pushcfunction(l, l_palette);
  lua_setglobal_P(l, "palette");
  lua_pushcfunction(l, l_border);
  lua_setglobal_P(l, "border");
  lua_pushcfunction(l, l_vsync);
  lua_setglobal_P(l, "vsync");
  lua_pushcfunction(l, l_frame);
  lua_setglobal_P(l, "frame");
  lua_pushcfunction(l, l_rgb);
  lua_setglobal_P(l, "rgb");
  lua_pushcfunction(l, l_color);
  lua_setglobal_P(l, "color");
  lua_pushcfunction(l, l_csize_height);
  lua_setglobal_P(l, "csize_height");
  lua_pushcfunction(l, l_csize_width);
  lua_setglobal_P(l, "csize_width");
  lua_pushcfunction(l, l_psize_height);
  lua_setglobal_P(l, "psize_height");
  lua_pushcfunction(l, l_psize_width);
  lua_setglobal_P(l, "psize_width");
  lua_pushcfunction(l, l_psize_lastline);
  lua_setglobal_P(l, "psize_lastline");
  lua_pushcfunction(l, l_pos_x);
  lua_setglobal_P(l, "pos_x");
  lua_pushcfunction(l, l_pos_y);
  lua_setglobal_P(l, "pos_y");
  lua_pushcfunction(l, l_char_get);
  lua_setglobal_P(l, "char_get");
  lua_pushcfunction(l, l_char_set);
  lua_setglobal_P(l, "char_set");
  lua_pushcfunction(l, l_cscroll);
  lua_setglobal_P(l, "cscroll");
  lua_pushcfunction(l, l_gscroll);
  lua_setglobal_P(l, "gscroll");
  lua_pushcfunction(l, l_point);
  lua_setglobal_P(l, "point");
  lua_pushcfunction(l, l_pset);
  lua_setglobal_P(l, "pset");
  lua_pushcfunction(l, l_line);
  lua_setglobal_P(l, "line");
  lua_pushcfunction(l, l_circle);
  lua_setglobal_P(l, "circle");
  lua_pushcfunction(l, l_rect);
  lua_setglobal_P(l, "rect");
  lua_pushcfunction(l, l_blit);
  lua_setglobal_P(l, "blit");

  return 0;
}
