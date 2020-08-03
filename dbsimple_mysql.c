#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#if !defined(HAVE_MYSQL) || defined(DYNAMIC_MYSQL)
#include <dlfcn.h>
#endif
#ifdef HAVE_MYSQL_H
#include <mysql/mysql.h>
#endif

#include "utilities.h"
#include "tree.h"
#include "dbsimple.h"
#include "dbsimplebase.h"

#ifdef HAVE_MYSQL_H

#if defined(HAVE_MYSQL) && (!defined(HAVE_MYSQL) || defined(DYNAMIC_MYSQL))

#define mysql_init    dynamicmysql_init

MYSQL* (*mysql_init)(MYSQL*);

static void* dlhandle = NULL;

#endif

struct dbsimple_connection_struct {
    struct dbsimple_connectionbase baseconnection;
    MYSQL* handle;
};

struct dbsimple_session_struct {
    struct dbsimple_sessionbase basesession;
};

void
dbsimple__fetchobject(struct dbsimple_definition* def, dbsimple_session_type session)
{
}

int
dbsimple__persistobject(struct object* object, dbsimple_session_type session)
{
    return 0; /* continue */
}

static int
commitdata(dbsimple_session_type session)
{
    return -1;
}

static int
openconnection(char* location,
               __attribute__((unused)) int nqueries, __attribute__((unused)) const char* const** queries,
               dbsimple_connection_type* connectionPtr)
{
    return -1;
}

static int
closeconnection(dbsimple_connection_type connection)
{
    return -1;
}

static int
opensession(dbsimple_connection_type connection, dbsimple_session_type* sessionPtr)
{
    return -1;
}

static int
closesession(dbsimple_session_type session)
{
    return -1;
}

static int
syncdata(dbsimple_session_type session, const char* const** query, void* data)
{
    return -1;
}

static void*
fetchdata(dbsimple_session_type session, const char* const** query, __attribute__((unused))  va_list args)
{
    return NULL;
}

static const struct dbsimple_module module = {
    "mysql", openconnection, closeconnection, opensession, closesession, syncdata, fetchdata, commitdata
};

#if !defined(HAVE_MYSQL) || defined(DYNAMIC_MYSQL)

int
dbsimple_sqlite3_initialize(char* hint)
{
    (void)hint;
    dlhandle = dlopen("libmysqlclient_r.so", RTLD_LAZY);
    mysql_init = (const MYSQL*(*)(MYSQL*))functioncast(dlsym(dlhandle, "mysql_init"));

    dbsimple_registermodule(&module);
    return 0;
}

int
dbsimple_sqlite3_finalize(void)
{
    sqlite3_init = NULL;
    dlclose(dlhandle);
    return 0;
}

#else

int
dbsimple_sqlite3_initialize(__attribute__((unused)) char* hint)
{
    dbsimple_registermodule(&module);
    return 0;
}

int
dbsimple_sqlite3_finalize(void)
{
    return 0;
}

#endif

#else

int dbsimple_mysql_initialize(char* hint) {
    return -1;
}

int dbsimple_mysql_finalize(void) {
    return -1;
}

#endif
