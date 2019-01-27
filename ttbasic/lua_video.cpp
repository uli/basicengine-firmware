#include "basic.h"
#include "lua_defs.h"

static int l_locate(lua_State *l)
{
    int32_t x = luaL_checknumber(l, -1);
    int32_t y = luaL_checknumber(l, -2);
    if (x >= sc0.getWidth())
        x = sc0.getWidth() - 1;
    else if (x < 0)
        x = 0;
    if (y >= sc0.getHeight())
        y = sc0.getHeight() - 1;
    else if (y < 0)
        y = 0;

    sc0.locate((uint16_t)x, (uint16_t)y);

    return 0;
}

int luaopen_video(lua_State *l)
{
    lua_pushcfunction(l, l_locate);
    lua_setglobal_P(l, "locate");
    return 0;
}
