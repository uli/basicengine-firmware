/*
** $Id: lbaselib.c $
** Basic library
** See Copyright Notice in lua.h
*/

#define lbaselib_c
#define LUA_LIB

#include "lprefix.h"


#include <ctype.h>
//#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"

#include <basic.h>

static void be_lua_writestring(const char *s, int l) {
  while (l--) {
    c_putch(*s++);
  }
}

void be_lua_writestringerror(const char *fmt, const char *msg) {
  char str[strlen_P(fmt)+strlen(msg)+1];
  sprintf_P(str, fmt, msg);
  c_puts(str);
}

static int luaB_print (lua_State *L) {
  int n = lua_gettop(L);  /* number of arguments */
  int i;
  lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    size_t l;
    lua_pushvalue(L, -1);  /* function to be called */
    lua_pushvalue(L, i);   /* value to print */
    lua_call(L, 1, 1);
    s = lua_tolstring(L, -1, &l);  /* get result */
    if (s == NULL)
      return luaL_error(L, "'tostring' must return a string to 'print'");
    if (i>1) lua_writestring("\t", 1);
    lua_writestring(s, l);
    lua_pop(L, 1);  /* pop result */
  }
  lua_writeline();
  return 0;
}


static int luaB_warn (lua_State *L) {
  const char *msg = luaL_checkstring(L, 1);
  lua_warning(L, msg);
  return 0;
}


#define SPACECHARS	" \f\n\r\t\v"

static const char *b_str2int (const char *s, int base, lua_Integer *pn) {
  lua_Unsigned n = 0;
  int neg = 0;
  s += strspn(s, SPACECHARS);  /* skip initial spaces */
  if (*s == '-') { s++; neg = 1; }  /* handle sign */
  else if (*s == '+') s++;
  if (!isalnum((unsigned char)*s))  /* no digit? */
    return NULL;
  do {
    int digit = (isdigit((unsigned char)*s)) ? *s - '0'
                   : (toupper((unsigned char)*s) - 'A') + 10;
    if (digit >= base) return NULL;  /* invalid numeral */
    n = n * base + digit;
    s++;
  } while (isalnum((unsigned char)*s));
  s += strspn(s, SPACECHARS);  /* skip trailing spaces */
  *pn = (lua_Integer)((neg) ? (0u - n) : n);
  return s;
}


static int luaB_tonumber (lua_State *L) {
  if (lua_isnoneornil(L, 2)) {  /* standard conversion? */
    if (lua_type(L, 1) == LUA_TNUMBER) {  /* already a number? */
      lua_settop(L, 1);  /* yes; return it */
      return 1;
    }
    else {
      size_t l;
      const char *s = lua_tolstring(L, 1, &l);
      if (s != NULL && lua_stringtonumber(L, s) == l + 1)
        return 1;  /* successful conversion to number */
      /* else not a number */
      luaL_checkany(L, 1);  /* (but there must be some parameter) */
    }
  }
  else {
    size_t l;
    const char *s;
    lua_Integer n = 0;  /* to avoid warnings */
    lua_Integer base = luaL_checkinteger(L, 2);
    luaL_checktype(L, 1, LUA_TSTRING);  /* no numbers as strings */
    s = lua_tolstring(L, 1, &l);
    luaL_argcheck(L, 2 <= base && base <= 36, 2, "base out of range");
    if (b_str2int(s, (int)base, &n) == s + l) {
      lua_pushinteger(L, n);
      return 1;
    }  /* else not a number */
  }  /* else not a number */
  lua_pushnil(L);  /* not a number */
  return 1;
}


static int luaB_error (lua_State *L) {
  int level = (int)luaL_optinteger(L, 2, 1);
  lua_settop(L, 1);
  if (lua_type(L, 1) == LUA_TSTRING && level > 0) {
    luaL_where(L, level);   /* add extra information */
    lua_pushvalue(L, 1);
    lua_concat(L, 2);
  }
  return lua_error(L);
}


static int luaB_getmetatable (lua_State *L) {
  luaL_checkany(L, 1);
  if (!lua_getmetatable(L, 1)) {
    lua_pushnil(L);
    return 1;  /* no metatable */
  }
  luaL_getmetafield(L, 1, "__metatable");
  return 1;  /* returns either __metatable field (if present) or metatable */
}


static int luaB_setmetatable (lua_State *L) {
  int t = lua_type(L, 2);
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_argexpected(L, t == LUA_TNIL || t == LUA_TTABLE, 2, "nil or table");
  if (luaL_getmetafield(L, 1, "__metatable") != LUA_TNIL)
    return luaL_error(L, "cannot change a protected metatable");
  lua_settop(L, 2);
  lua_setmetatable(L, 1);
  return 1;
}


static int luaB_rawequal (lua_State *L) {
  luaL_checkany(L, 1);
  luaL_checkany(L, 2);
  lua_pushboolean(L, lua_rawequal(L, 1, 2));
  return 1;
}


static int luaB_rawlen (lua_State *L) {
  int t = lua_type(L, 1);
  luaL_argexpected(L, t == LUA_TTABLE || t == LUA_TSTRING, 1,
                      "table or string");
  lua_pushinteger(L, lua_rawlen(L, 1));
  return 1;
}


static int luaB_rawget (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checkany(L, 2);
  lua_settop(L, 2);
  lua_rawget(L, 1);
  return 1;
}

static int luaB_rawset (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  luaL_checkany(L, 2);
  luaL_checkany(L, 3);
  lua_settop(L, 3);
  lua_rawset(L, 1);
  return 1;
}


static int pushmode (lua_State *L, int oldmode) {
  __lua_pushstring_P(L, (oldmode == LUA_GCINC) ? PSTR("incremental") : PSTR("generational"));
  return 1;
}

static const char __stop[] PROGMEM = "stop";
static const char __restart[] PROGMEM = "restart";
static const char __collect[] PROGMEM = "collect";
static const char __count[] PROGMEM = "count";
static const char __step[] PROGMEM = "step";
static const char __setpause[] PROGMEM = "setpause";
static const char __setstepmul[] PROGMEM = "setstepmul";
static const char __isrunning[] PROGMEM = "isrunning";
static const char __generational[] PROGMEM = "generational";
static const char __incremental[] PROGMEM = "incremental";

static int __attribute__((optimize ("no-jump-tables"))) luaB_collectgarbage (lua_State *L) {
  static const char *const opts[] PROGMEM = {__stop, __restart, __collect,
    __count, __step, __setpause, __setstepmul,
    __isrunning, __generational, __incremental, NULL};
  static const int optsnum[] PROGMEM = {LUA_GCSTOP, LUA_GCRESTART, LUA_GCCOLLECT,
    LUA_GCCOUNT, LUA_GCSTEP, LUA_GCSETPAUSE, LUA_GCSETSTEPMUL,
    LUA_GCISRUNNING, LUA_GCGEN, LUA_GCINC};
  int o = optsnum[luaL_checkoption(L, 1, "collect", opts)];
  switch (o) {
    case LUA_GCCOUNT: {
      int k = lua_gc(L, o);
      int b = lua_gc(L, LUA_GCCOUNTB);
      lua_pushnumber(L, (lua_Number)k + ((lua_Number)b/1024));
      return 1;
    }
    case LUA_GCSTEP: {
      int step = (int)luaL_optinteger(L, 2, 0);
      int res = lua_gc(L, o, step);
      lua_pushboolean(L, res);
      return 1;
    }
    case LUA_GCSETPAUSE:
    case LUA_GCSETSTEPMUL: {
      int p = (int)luaL_optinteger(L, 2, 0);
      int previous = lua_gc(L, o, p);
      lua_pushinteger(L, previous);
      return 1;
    }
    case LUA_GCISRUNNING: {
      int res = lua_gc(L, o);
      lua_pushboolean(L, res);
      return 1;
    }
    case LUA_GCGEN: {
      int minormul = (int)luaL_optinteger(L, 2, 0);
      int majormul = (int)luaL_optinteger(L, 3, 0);
      return pushmode(L, lua_gc(L, o, minormul, majormul));
    }
    case LUA_GCINC: {
      int pause = (int)luaL_optinteger(L, 2, 0);
      int stepmul = (int)luaL_optinteger(L, 3, 0);
      int stepsize = (int)luaL_optinteger(L, 4, 0);
      return pushmode(L, lua_gc(L, o, pause, stepmul, stepsize));
    }
    default: {
      int res = lua_gc(L, o);
      lua_pushinteger(L, res);
      return 1;
    }
  }
}


static int luaB_type (lua_State *L) {
  int t = lua_type(L, 1);
  luaL_argcheck(L, t != LUA_TNONE, 1, "value expected");
  lua_pushstring(L, lua_typename(L, t));
  return 1;
}


static int luaB_next (lua_State *L) {
  luaL_checktype(L, 1, LUA_TTABLE);
  lua_settop(L, 2);  /* create a 2nd argument if there isn't one */
  if (lua_next(L, 1))
    return 2;
  else {
    lua_pushnil(L);
    return 1;
  }
}


static int luaB_pairs (lua_State *L) {
  luaL_checkany(L, 1);
  if (luaL_getmetafield(L, 1, "__pairs") == LUA_TNIL) {  /* no metamethod? */
    lua_pushcfunction(L, luaB_next);  /* will return generator, */
    lua_pushvalue(L, 1);  /* state, */
    lua_pushnil(L);  /* and initial value */
  }
  else {
    lua_pushvalue(L, 1);  /* argument 'self' to metamethod */
    lua_call(L, 1, 3);  /* get 3 values from metamethod */
  }
  return 3;
}


/*
** Traversal function for 'ipairs'
*/
static int ipairsaux (lua_State *L) {
  lua_Integer i = luaL_checkinteger(L, 2) + 1;
  lua_pushinteger(L, i);
  return (lua_geti(L, 1, i) == LUA_TNIL) ? 1 : 2;
}


/*
** 'ipairs' function. Returns 'ipairsaux', given "table", 0.
** (The given "table" may not be a table.)
*/
static int luaB_ipairs (lua_State *L) {
  luaL_checkany(L, 1);
  lua_pushcfunction(L, ipairsaux);  /* iteration function */
  lua_pushvalue(L, 1);  /* state */
  lua_pushinteger(L, 0);  /* initial value */
  return 3;
}


static int load_aux (lua_State *L, int status, int envidx) {
  if (status == LUA_OK) {
    if (envidx != 0) {  /* 'env' parameter? */
      lua_pushvalue(L, envidx);  /* environment for loaded function */
      if (!lua_setupvalue(L, -2, 1))  /* set it as 1st upvalue */
        lua_pop(L, 1);  /* remove 'env' if not used by previous call */
    }
    return 1;
  }
  else {  /* error (message is on top of the stack) */
    lua_pushnil(L);
    lua_insert(L, -2);  /* put before error message */
    return 2;  /* return nil plus error message */
  }
}


static int luaB_loadfile (lua_State *L) {
  const char *fname = luaL_optstring(L, 1, NULL);
  const char *mode = luaL_optstring(L, 2, NULL);
  int env = (!lua_isnone(L, 3) ? 3 : 0);  /* 'env' index or 0 if no 'env' */
  int status = luaL_loadfilex(L, fname, mode);
  return load_aux(L, status, env);
}


/*
** {======================================================
** Generic Read function
** =======================================================
*/


/*
** reserved slot, above all arguments, to hold a copy of the returned
** string to avoid it being collected while parsed. 'load' has four
** optional arguments (chunk, source name, mode, and environment).
*/
#define RESERVEDSLOT	5


/*
** Reader for generic 'load' function: 'lua_load' uses the
** stack for internal stuff, so the reader cannot change the
** stack top. Instead, it keeps its resulting string in a
** reserved slot inside the stack.
*/
static const char *generic_reader (lua_State *L, void *ud, size_t *size) {
  (void)(ud);  /* not used */
  luaL_checkstack_P(L, 2, "too many nested functions");
  lua_pushvalue(L, 1);  /* get function */
  lua_call(L, 0, 1);  /* call it */
  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);  /* pop result */
    *size = 0;
    return NULL;
  }
  else if (!lua_isstring(L, -1))
    luaL_error(L, "reader function must return a string");
  lua_replace(L, RESERVEDSLOT);  /* save string in reserved slot */
  return lua_tolstring(L, RESERVEDSLOT, size);
}


static int luaB_load (lua_State *L) {
  int status;
  size_t l;
  const char *s = lua_tolstring(L, 1, &l);
  const char *mode = luaL_optstring(L, 3, "bt");
  int env = (!lua_isnone(L, 4) ? 4 : 0);  /* 'env' index or 0 if no 'env' */
  if (s != NULL) {  /* loading a string? */
    const char *chunkname = luaL_optstring(L, 2, s);
    status = luaL_loadbufferx(L, s, l, chunkname, mode);
  }
  else {  /* loading from a reader function */
    const char *chunkname = luaL_optstring(L, 2, "=(load)");
    luaL_checktype(L, 1, LUA_TFUNCTION);
    lua_settop(L, RESERVEDSLOT);  /* create reserved slot */
    status = lua_load(L, generic_reader, NULL, chunkname, mode);
  }
  return load_aux(L, status, env);
}

/* }====================================================== */


static int dofilecont (lua_State *L, int d1, lua_KContext d2) {
  (void)d1;  (void)d2;  /* only to match 'lua_Kfunction' prototype */
  return lua_gettop(L) - 1;
}


static int luaB_dofile (lua_State *L) {
  const char *fname = luaL_optstring(L, 1, NULL);
  lua_settop(L, 1);
  if (luaL_loadfile(L, fname) != LUA_OK)
    return lua_error(L);
  lua_callk(L, 0, LUA_MULTRET, 0, dofilecont);
  return dofilecont(L, 0, 0);
}


static int luaB_assert (lua_State *L) {
  if (lua_toboolean(L, 1))  /* condition is true? */
    return lua_gettop(L);  /* return all arguments */
  else {  /* error */
    luaL_checkany(L, 1);  /* there must be a condition */
    lua_remove(L, 1);  /* remove it */
    lua_pushliteral(L, "assertion failed!");  /* default message */
    lua_settop(L, 1);  /* leave only message (default if no other one) */
    return luaB_error(L);  /* call 'error' */
  }
}


static int luaB_select (lua_State *L) {
  int n = lua_gettop(L);
  if (lua_type(L, 1) == LUA_TSTRING && *lua_tostring(L, 1) == '#') {
    lua_pushinteger(L, n-1);
    return 1;
  }
  else {
    lua_Integer i = luaL_checkinteger(L, 1);
    if (i < 0) i = n + i;
    else if (i > n) i = n;
    luaL_argcheck(L, 1 <= i, 1, "index out of range");
    return n - (int)i;
  }
}


/*
** Continuation function for 'pcall' and 'xpcall'. Both functions
** already pushed a 'true' before doing the call, so in case of success
** 'finishpcall' only has to return everything in the stack minus
** 'extra' values (where 'extra' is exactly the number of items to be
** ignored).
*/
static int finishpcall (lua_State *L, int status, lua_KContext extra) {
  if (status != LUA_OK && status != LUA_YIELD) {  /* error? */
    lua_pushboolean(L, 0);  /* first result (false) */
    lua_pushvalue(L, -2);  /* error message */
    return 2;  /* return false, msg */
  }
  else
    return lua_gettop(L) - (int)extra;  /* return all results */
}


static int luaB_pcall (lua_State *L) {
  int status;
  luaL_checkany(L, 1);
  lua_pushboolean(L, 1);  /* first result if no errors */
  lua_insert(L, 1);  /* put it in place */
  status = lua_pcallk(L, lua_gettop(L) - 2, LUA_MULTRET, 0, 0, finishpcall);
  return finishpcall(L, status, 0);
}


/*
** Do a protected call with error handling. After 'lua_rotate', the
** stack will have <f, err, true, f, [args...]>; so, the function passes
** 2 to 'finishpcall' to skip the 2 first values when returning results.
*/
static int luaB_xpcall (lua_State *L) {
  int status;
  int n = lua_gettop(L);
  luaL_checktype(L, 2, LUA_TFUNCTION);  /* check error function */
  lua_pushboolean(L, 1);  /* first result */
  lua_pushvalue(L, 1);  /* function */
  lua_rotate(L, 3, 2);  /* move them below function's arguments */
  status = lua_pcallk(L, n - 2, LUA_MULTRET, 2, 2, finishpcall);
  return finishpcall(L, status, 2);
}


static int luaB_tostring (lua_State *L) {
  luaL_checkany(L, 1);
  luaL_tolstring(L, 1, NULL);
  return 1;
}

static const char __l_assert[] PROGMEM = "assert";
static const char __collectgarbage[] PROGMEM = "collectgarbage";
static const char __dofile[] PROGMEM = "dofile";
static const char __error[] PROGMEM = "error";
static const char __getmetatable[] PROGMEM = "getmetatable";
static const char __ipairs[] PROGMEM = "ipairs";
static const char __loadfile[] PROGMEM = "loadfile";
static const char __load[] PROGMEM = "load";
static const char __next[] PROGMEM = "next";
static const char __pairs[] PROGMEM = "pairs";
static const char __pcall[] PROGMEM = "pcall";
static const char __print[] PROGMEM = "print";
static const char __warn[] PROGMEM = "warn";
static const char __rawequal[] PROGMEM = "rawequal";
static const char __rawlen[] PROGMEM = "rawlen";
static const char __rawget[] PROGMEM = "rawget";
static const char __rawset[] PROGMEM = "rawset";
static const char __select[] PROGMEM = "select";
static const char __setmetatable[] PROGMEM = "setmetatable";
static const char __tonumber[] PROGMEM = "tonumber";
static const char __tostring[] PROGMEM = "tostring";
static const char __type[] PROGMEM = "type";
static const char __xpcall[] PROGMEM = "xpcall";
  /* placeholders */
static const char __LUA_GNAME[] PROGMEM = LUA_GNAME;
static const char ___VERSION[] PROGMEM = "_VERSION";

static const luaL_Reg base_funcs[] PROGMEM = {
  {__l_assert, luaB_assert},
  {__collectgarbage, luaB_collectgarbage},
  {__dofile, luaB_dofile},
  {__error, luaB_error},
  {__getmetatable, luaB_getmetatable},
  {__ipairs, luaB_ipairs},
  {__loadfile, luaB_loadfile},
  {__load, luaB_load},
  {__next, luaB_next},
  {__pairs, luaB_pairs},
  {__pcall, luaB_pcall},
  {__print, luaB_print},
  {__warn, luaB_warn},
  {__rawequal, luaB_rawequal},
  {__rawlen, luaB_rawlen},
  {__rawget, luaB_rawget},
  {__rawset, luaB_rawset},
  {__select, luaB_select},
  {__setmetatable, luaB_setmetatable},
  {__tonumber, luaB_tonumber},
  {__tostring, luaB_tostring},
  {__type, luaB_type},
  {__xpcall, luaB_xpcall},
  /* placeholders */
  {__LUA_GNAME, NULL},
  {___VERSION, NULL},
  {NULL, NULL}
};


LUAMOD_API int luaopen_base (lua_State *L) {
  /* open lib into global table */
  lua_pushglobaltable(L);
  luaL_setfuncs(L, base_funcs, 0);
  /* set global _G */
  lua_pushvalue(L, -1);
  lua_setfield_P(L, -2, LUA_GNAME);
  /* set global _VERSION */
  lua_pushliteral(L, LUA_VERSION);
  lua_setfield_P(L, -2, "_VERSION");
  return 1;
}

