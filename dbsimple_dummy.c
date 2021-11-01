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
#include <unistd.h>
#include <assert.h>

#include "dbsimple.h"
#include "dbsimplebase.h"

struct dbsimple_connection_struct {
    struct dbsimple_connectionbase baseconnection;
};

struct dbsimple_session_struct {
    struct dbsimple_sessionbase basesession;
};

static int
commitdata(__attribute__((unused)) dbsimple_session_type session)
{
    return 0;
}

static int
openconnection(__attribute__((unused)) char* location,
               __attribute__((unused)) int nqueries, __attribute__((unused)) const char* const** queries,
               dbsimple_connection_type* connectionPtr)
{
    *connectionPtr = malloc(sizeof(struct dbsimple_connection_struct));
    return 0;
}

static int
closeconnection(dbsimple_connection_type connection)
{
    free(connection);
    return 0;
}

static int
opensession(__attribute__((unused)) dbsimple_connection_type connection, dbsimple_session_type* sessionPtr)
{
    *sessionPtr = malloc(sizeof(struct dbsimple_session_struct));
    return 0;
}

static int
closesession(dbsimple_session_type session)
{
    free(session);
    return 0;
}

static int
syncdata(__attribute__((unused)) dbsimple_session_type session, __attribute__((unused)) const char* const** query, __attribute__((unused)) void* data)
{
    return 0;
}

static void*
fetchdata(__attribute__((unused)) dbsimple_session_type session, __attribute__((unused)) const char* const** query, __attribute__((unused))  va_list args)
{
    return NULL;
}

static const struct dbsimple_module module = {
    "dummy", openconnection, closeconnection, opensession, closesession, syncdata, fetchdata, commitdata, NULL, NULL
};

int
dbsimple_dummy_initialize(void)
{
    dbsimple_registermodule(&module);
    return 0;
}

int
dbsimple_dummy_finalize(void)
{
    return 0;
}
