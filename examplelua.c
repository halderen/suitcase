/*
 * Copyright (c) 2021 A.W. van Halderen
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

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
