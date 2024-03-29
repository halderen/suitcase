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
#include <stddef.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include <dlfcn.h>
#include "utilities.h"
#include "modules.h"

static void* modulesymbol;
static void* modulessymbol;

static functioncast_type
functioncastx(void*generic) {
    functioncast_type* function = (functioncast_type*)&generic;
    return *function;
}

__attribute__((constructor))
void
init(void)
{    
    int (*modules)(void*);
    modulesymbol = dlsym(NULL, "module");
    modulessymbol = dlsym(NULL, "modules");
    modules = (int (*)(void*))functioncastx(modulessymbol);

    modules(&module);
}

__attribute__((destructor))
void
fini(void)
{
    int (*modules)(void*);
    modules = (int (*)(void*)) functioncastx(modulessymbol);
    modules(&module);
}
