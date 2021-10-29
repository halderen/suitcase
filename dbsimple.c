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
    int nmodules2;
    const struct dbsimple_module** modules2;
    nmodules2 = nmodules + 1;
    modules2 = realloc(modules, sizeof(struct dbsimple_module*) * nmodules2);
    if(modules2) {
        modules2[nmodules2 - 1] = bindings;
        modules = modules2;
        nmodules = nmodules2;
    } else
        return -1;
    return 0;
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
    // stmtindex = query - session->connection->queries;
    returnCode = session->basesession.syncdata(session, query, data);
    return returnCode;
}

void*
dbsimple_fetch(dbsimple_session_type session, const char* const** query, ...)
{
    va_list args;
    void* returnValue;
    va_start(args, query);
    // stmtindex = query - session->connection->queries;
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
