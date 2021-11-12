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
#include <string.h>
#include <assert.h>

#include "utilities.h"
#include "tree.h"
#include "dbsimple.h"
#include "dbsimplebase.h"

int dbsimple_initialize(void) {
    return 0;
}

int dbsimple_finalize(void) {
    return 0;
}

struct dbsimple_connection_struct {
    struct dbsimple_connectionbase baseconnection;
};

struct dbsimple_session_struct {
    struct dbsimple_sessionbase basesession;
};

static int nmodules = 0;
static const struct dbsimple_module** modules = NULL;

int
dbsimple_registermodule(const struct dbsimple_module* bindings)
{
    if(!alloc(&modules, sizeof(struct dbsimple_module*), &nmodules, nmodules + 1)) {
        modules[nmodules - 1] = bindings;
        return 0;
    } else
        return -1;
}

int
dbsimple_openconnection(char* location,
                        int nqueries, const char* const** queries,
                        int ndefinitions, struct dbsimple_definition** definitions,
                        dbsimple_connection_type* connectionPtr)
{
    int returnCode = 0;
    int count;
    dbsimple_connection_type connection;
    const struct dbsimple_module* module = NULL;

    if(nqueries < 0) {
        for(count=0; queries[count]; count++)
            ;
        nqueries = count;
    }
    if(ndefinitions < 0) {
        for(count=0; definitions[count]; count++)
            ;
        ndefinitions = count;
    }

    for (int i=0; i<nmodules; i++) {
        if (!strncasecmp(location, modules[i]->identifier, strlen(modules[i]->identifier)) &&
           location[strlen(modules[i]->identifier)] == ':') {
            module = modules[i];
            location = &location[strlen(modules[i]->identifier) + 1];
            break;
        } else if (modules[i]->identifier == NULL) {
            module = modules[i];
        }
    }
    if (!module) {
        return -1;
    }

    returnCode = module->openconnection(location, nqueries, queries, connectionPtr);
    connection = *connectionPtr;
    connection->baseconnection.location     = strdup(location);
    connection->baseconnection.nqueries     = nqueries;
    connection->baseconnection.queries      = queries;
    connection->baseconnection.ndefinitions = ndefinitions;
    connection->baseconnection.definitions  = definitions;
    if(module->closeconnection) connection->baseconnection.closeconnection = module->closeconnection;
    if(module->opensession)     connection->baseconnection.opensession     = module->opensession;
    if(module->closesession)    connection->baseconnection.closesession    = module->closesession;
    if(module->syncdata)        connection->baseconnection.syncdata        = module->syncdata;
    if(module->fetchdata)       connection->baseconnection.fetchdata       = module->fetchdata;
    if(module->commitdata)      connection->baseconnection.commitdata      = module->commitdata;
    if(module->fetchobject)     connection->baseconnection.fetchobject     = module->fetchobject;
    if(module->persistobject)   connection->baseconnection.persistobject   = module->persistobject;
    return returnCode;
}

int
dbsimple_closeconnection(dbsimple_connection_type connection)
{
    free(connection->baseconnection.location);
    connection->baseconnection.location = NULL;
    connection->baseconnection.closeconnection(connection);
    return 0;
}

int
dbsimple_opensession(dbsimple_connection_type connection, dbsimple_session_type* sessionPtr)
{
    int returnCode = 0;
    dbsimple_session_type session;
    if(!connection) {
        sessionPtr = NULL;
        return -1;
    }
    returnCode = connection->baseconnection.opensession(connection, sessionPtr);
    session = *sessionPtr;
    session->basesession.closesession  = connection->baseconnection.closesession;
    session->basesession.syncdata      = connection->baseconnection.syncdata;
    session->basesession.fetchdata     = connection->baseconnection.fetchdata;
    session->basesession.commitdata    = connection->baseconnection.commitdata;
    session->basesession.fetchobject   = connection->baseconnection.fetchobject;
    session->basesession.persistobject = connection->baseconnection.persistobject;
    return returnCode;
}

int
dbsimple_closesession(dbsimple_session_type session)
{
    int returnCode = 0;
    returnCode = session->basesession.closesession(session);
    return returnCode;
}

int
dbsimple_sync(dbsimple_session_type session, const char* const** query, void* data)
{
    int returnCode = 0;
    returnCode = session->basesession.syncdata(session, query, data);
    return returnCode;
}

void*
dbsimple_fetch(dbsimple_session_type session, const char* const** query, ...)
{
    va_list args;
    void* returnValue;
    va_start(args, query);
    returnValue = session->basesession.fetchdata(session, query, args);
    va_end(args);
    return returnValue;
}

int
dbsimple_commit(dbsimple_session_type session)
{
    int returnCode = 0;
#ifdef BERRY
    struct object* object;
    for(int i=0; i<session->connection->ndefinitions; i++) {
        if((session->connection->definitions[i]->flags & dbsimple_FLAG_SINGLETON) == dbsimple_FLAG_SINGLETON) {
            object = dbsimple__getobject(&session->basesession, session->connection->definitions[i], 0, NULL);
            dbsimple__committraverse(&session->basesession, object);
        }
    }
#endif
    returnCode = session->basesession.commitdata(session);
    return returnCode;
}
