#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#ifdef HAVE_LUALIB_H
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#endif
#include "utilities.h"
#include "settings.h"
#include "modules.h"
#include "logging.h"
#include "examplelua.h"

static int
suitcasefunc(lua_State* l)
{
    lua_pushnumber(l, 0.0);
    lua_pushinteger(l, 0);
    double arg = luaL_checknumber(l, 1);
    (void)arg;
    return 1; /* number of results */
}

static const struct luaL_reg luareg[] = {
    { "suitcasefunc", suitcasefunc },
    { NULL, NULL }  /* sentinel */
};

int
executelua(char* command)
{
    int rcode;
    lua_State* l = NULL;
    l = lua_open();
    luaL_openlibs(l);
    luaL_openlib(l, "suitcase", luareg, 0);

    lua_setglobal(l, "suitcase");

    rcode = luaL_loadbuffer(l, command, strlen(command), "line");
    if(rcode)
        goto failure;
    lua_pcall(l, 0, 0, 0);
    if(rcode)
        goto failure;
    lua_getglobal(l, "suitcase");
    if(!lua_isnumber(l, -1))
        fprintf(stderr, "%s\n", "should be number");
    lua_tonumber(l, -1);
    lua_close(l);
    return 0;

  failure:
    if(l) {
        fprintf(stderr, "%s", lua_tostring(l, -1));
        lua_pop(l, 1);
        lua_close(l);
    }
    return 1;
}
