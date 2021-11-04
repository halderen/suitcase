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
#ifdef HAVE_TCL_H
#include <tcl.h>
#endif
#include "utilities.h"
#include "settings.h"
#include "modules.h"
#include "logging.h"
#include "executetcl.h"

struct userdata {
    void* ptr;
};

int
suitcaseCmd(ClientData clientData, Tcl_Interp* interp, int argc, char* argv[])
{
    if(!strcmp(argv[1], "configure")) {
        return TCL_OK;
    }
    Tcl_SetResult(interp, "wrong # arguments", TCL_STATIC);
    Tcl_SetResult(interp, "bad # arguments", TCL_STATIC);
    Tcl_SetResult(interp, "bad command", TCL_STATIC);
    return TCL_ERROR;
}

void
suitcaseDeleteProc(ClientData clientData)
{
    free(clientData);
}

int
appInit(Tcl_Interp* interp)
{
    struct userdata* userdata;
    if(Tcl_Init(interp) == TCL_ERROR)
        return TCL_ERROR;
    userdata = malloc(sizeof(userdata));
    userdata->ptr = NULL;
    if(Tcl_CreateCommand(interp, "suitcase", &suitcaseCmd, userdata, &suitcaseDeleteProc) == NULL) {
        return TCL_OK;
    }
}

int
executetcl(char* command, va_list ap)
{
    int argc = 1;
    char *argv[1] = { command };
    int rcode;
    Tcl_SetStartupScript(NULL, NULL);
    Tcl_GlobalEval(interp, command);
    Tcl_Main(argc, argv, appInit);
    return 0;

  failure:
    return 1;
}
