/*
** $Id: linit.c $
** Initialization of libraries for lua.c and other clients
** See Copyright Notice in lua.h
*/


#define linit_c
#define LUA_LIB

/*
** If you embed Lua in your program and need to open the standard
** libraries, call luaL_openlibs in your program. If you need a
** different set of libraries, copy this file to your project and edit
** it to suit your needs.
**
** You can also *preload* libraries, so that a later 'require' can
** open the library, which is already linked to the application.
** For that, do the following code:
**
**  luaL_getsubtable(L, LUA_REGISTRYINDEX, LUA_PRELOAD_TABLE);
**  lua_pushcfunction(L, luaopen_modname);
**  lua_setfield(L, -2, modname);
**  lua_pop(L, 1);  // remove PRELOAD table
*/

#include "lprefix.h"


#include <stddef.h>

#include "lua.h"

#include "lualib.h"
#include "lauxlib.h"


/*
** these libs are loaded by lua.c and are readily available to any Lua
** program
*/
static const char __LUA_GNAME[] PROGMEM = LUA_GNAME;
static const char __LUA_LOADLIBNAME[] PROGMEM = LUA_LOADLIBNAME;
static const char __LUA_COLIBNAME[] PROGMEM = LUA_COLIBNAME;
static const char __LUA_TABLIBNAME[] PROGMEM = LUA_TABLIBNAME;
static const char __LUA_IOLIBNAME[] PROGMEM = LUA_IOLIBNAME;
static const char __LUA_OSLIBNAME[] PROGMEM = LUA_OSLIBNAME;
static const char __LUA_STRLIBNAME[] PROGMEM = LUA_STRLIBNAME;
static const char __LUA_MATHLIBNAME[] PROGMEM = LUA_MATHLIBNAME;
static const char __LUA_UTF8LIBNAME[] PROGMEM = LUA_UTF8LIBNAME;
static const char __LUA_DBLIBNAME[] PROGMEM = LUA_DBLIBNAME;

static const luaL_Reg loadedlibs[] PROGMEM = {
  {__LUA_GNAME, luaopen_base},
  {__LUA_LOADLIBNAME, luaopen_package},
  {__LUA_COLIBNAME, luaopen_coroutine},
  {__LUA_TABLIBNAME, luaopen_table},
  {__LUA_IOLIBNAME, luaopen_io},
  {__LUA_OSLIBNAME, luaopen_os},
  {__LUA_STRLIBNAME, luaopen_string},
  {__LUA_MATHLIBNAME, luaopen_math},
  {__LUA_UTF8LIBNAME, luaopen_utf8},
  {__LUA_DBLIBNAME, luaopen_debug},
  {NULL, NULL}
};


LUALIB_API void luaL_openlibs (lua_State *L) {
  char buf[128];
  buf[127] = 0;
  const luaL_Reg *lib;
  /* "require" functions from 'loadedlibs' and set results to global table */
  for (lib = loadedlibs; lib->func; lib++) {
    strncpy_P(buf, lib->name, 127);
    luaL_requiref(L, buf, lib->func, 1);
    lua_pop(L, 1);  /* remove lib */
  }
}

