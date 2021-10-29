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
