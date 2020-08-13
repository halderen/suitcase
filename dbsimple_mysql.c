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
#include <mysql.h>
#endif

#include "utilities.h"
#include "tree.h"
#include "dbsimple.h"
#include "dbsimplebase.h"

#if defined(HAVE_MYSQL_H)

#if!defined(HAVE_MYSQL) || defined(DYNAMIC_MYSQL)

#define mysql_init    dynamicmysql_init

MYSQL* (*mysql_init)(MYSQL*);

static void* dlhandle = NULL;

#endif

struct dbsimple_connection_struct {
    struct dbsimple_connectionbase baseconnection;
    char* location;
    char* hostname;
    char* basename;
    char* username;
    char* password;
    int port;
    int connecttimeout;
    char* socketname;
};

struct dbsimple_session_struct {
    struct dbsimple_sessionbase basesession;
    dbsimple_connection_type connection;
    MYSQL* handle;
    MYSQL_STMT* beginStmt;
    MYSQL_STMT* commitStmt;
    MYSQL_STMT* rollbackStmt;
    MYSQL_STMT* currentStmt;
    MYSQL_STMT*** stmts;
    int nstmts;
};

static inline int
preparestmt(MYSQL*mysql, const char* query, MYSQL_STMT** stmtPtr)
{
    MYSQL_STMT* stmt;
    stmt = mysql_stmt_init(mysql);
    mysql_stmt_prepare(stmt, query, -1);
    *stmtPtr = stmt;
    return 0;
}

static int
dostatement(dbsimple_session_type session, MYSQL_STMT* stmt, struct object* object, int style)
{
    int column = 0;
    static my_bool isnull = 1;
    struct object* targetobj;
    void* targetptr;
    struct dbsimple_field* field;

    int nparam = mysql_stmt_param_count(stmt);
    MYSQL_BIND bind[nparam];
    memset(bind, 0, sizeof(bind));
    column = 0;
    for(int i = 0; i<object->type->nfields; i++) {
        int valuetype = MYSQL_TYPE_NULL;
        void* valueptr;
        if(column == nparam)
            abort();
        if(style == 0) {
            if((object->type->flags & dbsimple_FLAG_HASREVISION) ? i>=2 : i>=1)
                break;
            field = &object->type->fields[i];
        } else if(style == -1) {
            if(i+2<object->type->nfields)
                field = &object->type->fields[i+2];
            else
                field = &object->type->fields[i-(object->type->nfields-2)];
        } else {
            field = &object->type->fields[i];
        }
        switch(field->type) {
            case dbsimple_INTEGER:
                valuetype = MYSQL_TYPE_LONG;
                switch(i) {
                    case 0:
                        valueptr = &object->keyid;
                        break;
                    case 1:
                        if(object->type->flags & dbsimple_FLAG_HASREVISION) {
                            valueptr = &object->revision;
                            break;
                        }
                        /* else deliberate fall through */
                        // fall through
                    default:
                        valueptr = (int*)&(object->data[field->fieldoffset]);
                }
                break;
            case dbsimple_STRING:
                valuetype = MYSQL_TYPE_STRING;
                switch(i) {
                    case 0:
                        valueptr = (void*) object->keyname;
                        break;
                    default:
                        valueptr = *(char**)&(object->data[field->fieldoffset]);
                }
                break;
            case dbsimple_REFERENCE:
                targetptr = *(void**)&(object->data[field->fieldoffset]);
                if(targetptr != NULL) {
                    targetobj = dbsimple__referencebyptr(&session->basesession, field->def, targetptr);
                    if(targetobj->keyname) {
                        valuetype = MYSQL_TYPE_LONG;
                        valueptr = (void*)targetobj->keyname;
                    } else {
                        valuetype = MYSQL_TYPE_STRING;
                        valueptr = &targetobj->keyid;
                    }
                } else {
                    switch(field->def->fields[0].type) {
                        case dbsimple_INTEGER:
                            valuetype = MYSQL_TYPE_LONG;
                            valueptr = NULL;
                            break;
                        case dbsimple_STRING:
                            valuetype = MYSQL_TYPE_STRING;
                            valueptr = NULL;
                            break;
                    }
                }
                break;
            case dbsimple_BACKREFERENCE:
            case dbsimple_MASTERREFERENCES:
            case dbsimple_OPENREFERENCES:
                valuetype = MYSQL_TYPE_NULL;
                break;
        }
        switch(valuetype) {
            case MYSQL_TYPE_LONG:
                bind[column].buffer = valueptr;
                bind[column].buffer_type = MYSQL_TYPE_LONG;
                if(!valueptr)
                    bind[column].is_null = &isnull;
                ++column;
                break;
            case MYSQL_TYPE_STRING:
                bind[column].buffer = valueptr;
                bind[column].buffer_length = -1;
                bind[column].length = NULL;
                bind[column].buffer_type = MYSQL_TYPE_STRING;
                if(!valueptr)
                    bind[column].is_null = &isnull;
                ++column;
                break;
        }
    }
    mysql_stmt_bind_param(stmt, bind);
    mysql_stmt_execute(stmt);
    return 0;
}

static void
invalidatesession(dbsimple_session_type session)
{
    MYSQL_STMT** stmts;
    for(int i=0; i<session->nstmts; i++) {
        if(session->stmts[i]) {
            for(stmts=session->stmts[i]; *stmts; stmts++) {
                mysql_stmt_close(*stmts);
            }
        }
    }
    free(session->stmts);
    mysql_stmt_close(session->beginStmt);
    mysql_stmt_close(session->beginStmt);
    mysql_stmt_close(session->rollbackStmt);
    session->stmts = NULL;
    session->beginStmt = session->commitStmt = session->rollbackStmt = NULL;
}

static int
validatesession(dbsimple_session_type session)
{
    int returnCode = 0;
    const char* const* queries;
    int i, j, count;
    const char* beginQuery = "BEGIN TRANSACTION";
    const char* commitQuery = "COMMIT";
    const char* rollbackQuery = "ROLLBACK";
    MYSQL_STMT** stmts;
    MYSQL_RES *result;
    if(session->handle && !mysql_query(session->handle, "SELECT 1")) {
          result = mysql_store_result(session->handle);
          mysql_free_result(result);
          return 0;
    }
    if(session->stmts) {
        invalidatesession(session);
    }
    
    session->handle = mysql_init(NULL);
    if(session->connection->connecttimeout >= 0)
        mysql_options(session->handle, MYSQL_OPT_CONNECT_TIMEOUT, &session->connection->connecttimeout);
    mysql_real_connect(session->handle, session->connection->hostname, session->connection->username, session->connection->password,
                           session->connection->basename, session->connection->port, session->connection->socketname, 0);
    mysql_autocommit(session->handle, 0);
    preparestmt(session->handle, beginQuery, &session->beginStmt);
    preparestmt(session->handle, commitQuery, &session->commitStmt);
    preparestmt(session->handle, rollbackQuery, &session->rollbackStmt);
    session->nstmts = session->connection->baseconnection.nqueries;
    session->stmts = malloc(sizeof(MYSQL_STMT**) * session->nstmts);
    for(i = 0; i<session->connection->baseconnection.nqueries; i++) {
        queries = session->connection->baseconnection.queries[i];
        for(count=0; queries[count]; count++)
            ;
        stmts = session->stmts[i] = malloc(sizeof (MYSQL_STMT*) * (count+1));
        for(j = 0; j<count; j++) {
            if(preparestmt(session->handle, queries[j], &stmts[j])) {
                while(j-- >0) {
                    mysql_stmt_close(stmts[j]);
                }
                free(session->stmts[i]);
                session->stmts[i] = NULL;
            }
        }
        stmts[count] = NULL;
    }
    return returnCode;
}

static int
openconnection(char* location,
               __attribute__((unused)) int nqueries, __attribute__((unused)) const char* const** queries,
               dbsimple_connection_type* connectionPtr)
{
    char* s;
    char* hostname = NULL;
    char* username = NULL;
    char* password = NULL;
    char* basename = NULL;
    char* options = NULL;
    char* socketname = NULL;
    char* connecttimeout = NULL;;
    int port;
    int returnCode = 0;
    *connectionPtr = malloc(sizeof(struct dbsimple_connection_struct));
    if(!strncasecmp(location,"mysql:",strlen("mysql:")))
        location += strlen("mysql:");
    if(!strcmp(location,"//"))
        location += strlen("//");
    location = strdup(location);
    if((options = strchr(location,'?'))) {
        *(options++) = '\0';
        do {
            if(!strncasecmp(options, "socket=", strlen("socket="))) {
                socketname = &options[strlen("socket=")+1];
            } else if(!strncasecmp(options, "connecttimeout=", strlen("connecttimeout"))) {
                connecttimeout = &options[strlen("connecttimeout=")+1];
            }
            if((options = strchr(options, ','))) {
                *(options++) = '\0';
            }
        } while(options && *options);
    }
    if((basename = strchr(location,'/'))) {
        *(basename++) = '\0';
        if((hostname = strchr(location,'@'))) {
            *(hostname++) = '\0';
            username = location;
            if((password = strchr(username,':'))) {
                *(password++) = '\0';
            }
        } else {
            hostname = location;
            username = NULL;
            password = NULL;
        }
        if(hostname) {
            if((s = strchr(hostname,':'))) {
                *(s++) = '\0';
                port = atoi(s);
            } else {
                port = -1;
            }
        }
    } else {
        basename = location;
        hostname = NULL;
        username = NULL;
        port = -1;
    }
    (*connectionPtr)->location = strdup(location);
    (*connectionPtr)->hostname = (hostname ? strdup(hostname) : NULL);
    (*connectionPtr)->username = (username ? strdup(username) : NULL);
    (*connectionPtr)->password = (password ? strdup(password) : NULL);
    (*connectionPtr)->basename = (basename ? strdup(basename) : NULL);
    (*connectionPtr)->port = port;
    (*connectionPtr)->connecttimeout = (connecttimeout ? atoi(connecttimeout) : -1);
    (*connectionPtr)->socketname = socketname;
    free(location);
    return returnCode;
}

static int
closeconnection(dbsimple_connection_type connection)
{
    free(connection->location);
    free(connection->hostname);
    free(connection->password);
    free(connection->basename);
    free(connection->location);
    free(connection->socketname);
    free(connection);
    return 0;
}

static int
opensession(dbsimple_connection_type connection, dbsimple_session_type* sessionPtr)
{
    int returnCode = 0;
    *sessionPtr = malloc(sizeof(struct dbsimple_session_struct));
    (*sessionPtr)->handle = NULL;
    (*sessionPtr)->connection = connection;
    (*sessionPtr)->stmts = NULL;
    (*sessionPtr)->beginStmt = NULL;
    (*sessionPtr)->commitStmt = NULL;
    (*sessionPtr)->rollbackStmt = NULL;
    returnCode = validatesession(*sessionPtr);
    return returnCode;
}

static int
closesession(dbsimple_session_type session)
{
    invalidatesession(session);
    free(session);
    return 0;
}

static int
syncdata(dbsimple_session_type session, const char* const** query, void* data)
{
    MYSQL_STMT** stmts = NULL;
    int stmtindex;
    MYSQL_RES* result;
    stmtindex = query - session->connection->baseconnection.queries;
    if(stmtindex>=0&&stmtindex<session->nstmts) {
        stmts = session->stmts[stmtindex];
        for(int i = 0; stmts[i]; i++) {
            mysql_stmt_execute(stmts[i]);
            result = mysql_store_result(session->handle);
            mysql_free_result(result);
        }
    } else {
        mysql_query(session->handle, **query);
        result = mysql_store_result(session->handle);
        mysql_free_result(result);
    }
    return 0;
}

static void
fetchobject(struct dbsimple_definition* def, dbsimple_session_type session)
{
    int column;
    int targetkeyid;
    long intvalue;
    char* cstrvalue;
    const char* targetkeystr;
    struct object* object = NULL;
    MYSQL_STMT* stmt = session->currentStmt;

    MYSQL_BIND bind[def->nfields];
    long intvalues[def->nfields];
    char* cstrvalues[def->nfields];
    my_bool nullvalues[def->nfields];
    memset(bind, 0, sizeof(bind));
    memset(cstrvalues, 0, sizeof(cstrvalues));
    column = 0;
    for(int i=0; i<def->nfields; i++) {
        bind[column].is_null = &nullvalues[column];
        switch(def->fields[i].type) {
            case dbsimple_INTEGER:
                bind[column].buffer  = &intvalues[column];
                bind[column].buffer_type = MYSQL_TYPE_LONG;
                column += 1;
                break;
            case dbsimple_STRING:
                bind[column].buffer = &cstrvalues[column];
                bind[column].buffer_type = MYSQL_TYPE_STRING;
                bind[column].buffer_length = 10240;
                cstrvalues[column] = malloc(bind[column].buffer_length);
                column += 1;
                break;
            case dbsimple_REFERENCE:
            case dbsimple_BACKREFERENCE:
                switch(def->fields[i].def->fields[0].type) {
                    case dbsimple_INTEGER:
                        bind[column].buffer = &intvalues[column];
                        bind[column].buffer_type = MYSQL_TYPE_LONG;
                        break;
                    case dbsimple_STRING:
                        bind[column].buffer = &cstrvalues[column];
                        bind[column].buffer_type = MYSQL_TYPE_STRING;
                        bind[column].buffer_length = 10240;
                        cstrvalues[column] = malloc(bind[column].buffer_length);
                        break;
                    default:
                        abort();
                }
                column += 1;
                break;
            case dbsimple_MASTERREFERENCES:
            case dbsimple_OPENREFERENCES:
                break;
        }
    }
    mysql_stmt_bind_result(stmt, bind);

    while(mysql_stmt_fetch(stmt)) {
        column = 0;
        for(int i = 0; i<def->nfields; i++) {
            switch(def->fields[i].type) {
                case dbsimple_INTEGER:
                    intvalue = intvalues[column++];
                    if(i==0) {
                        object = dbsimple__getobject(&session->basesession, def, intvalue, NULL);
                        object->state = OBJCLEAN;
                    } else if(i==1&&(def->flags&dbsimple_FLAG_HASREVISION)) {
                        object->revision = intvalue;
                    }
                    if(def->fields[i].fieldoffset>=0)
                        *(int*)&(object->data[def->fields[i].fieldoffset]) = intvalue;
                    break;
                case dbsimple_STRING:
                    cstrvalue = strdup(cstrvalues[column++]);
                    if(i==0) {
                        object = dbsimple__getobject(&session->basesession, def, 0, cstrvalue);
                        object->state = OBJCLEAN;
                    }
                    if(def->fields[i].fieldoffset>=0)
                        *(char**)&(object->data[def->fields[i].fieldoffset]) = cstrvalue;
                    break;
                case dbsimple_REFERENCE:
                    targetkeyid = 0;
                    targetkeystr = NULL;
                    if(nullvalues[column]) {
                        ++column;
                        *(void**)&(object->data[def->fields[i].fieldoffset]) = NULL;
                    } else {
                        if((def->fields[i].def->flags&dbsimple_FLAG_SINGLETON)!=dbsimple_FLAG_SINGLETON) {
                            switch(def->fields[i].def->fields[0].type) {
                                case dbsimple_INTEGER:
                                    targetkeyid = intvalues[column++];
                                    break;
                                case dbsimple_STRING:
                                    targetkeystr = strdup(cstrvalues[column++]);
                                    break;
                                default:
                                    abort();
                            }
                        } else
                            abort();
                        dbsimple__assignreference(&session->basesession, &def->fields[i], targetkeyid, targetkeystr, object);
                    }
                    break;
                case dbsimple_BACKREFERENCE:
                    targetkeyid = 0;
                    targetkeystr = NULL;
                    if((def->fields[i].def->flags&dbsimple_FLAG_SINGLETON)!=dbsimple_FLAG_SINGLETON) {
                        if(nullvalues[column]) {
                            switch(def->fields[i].def->fields[0].type) {
                                case dbsimple_INTEGER:
                                    targetkeyid = intvalues[column++];
                                    break;
                                case dbsimple_STRING:
                                    targetkeystr = strdup(cstrvalues[column++]);
                                    break;
                                default:
                                    abort();
                            }
                            dbsimple__assignbackreference(&session->basesession, def->fields[i].def, targetkeyid, targetkeystr, &def->fields[i], object);
                        } else {
                            ++column;
                        }
                    } else {
                        dbsimple__assignbackreference(&session->basesession, def->fields[i].def, targetkeyid, targetkeystr, &def->fields[i], object);
                    }
                    break;
                case dbsimple_MASTERREFERENCES:
                case dbsimple_OPENREFERENCES:
                    break;
            }
        }
    }
    mysql_stmt_next_result(stmt);

    for(int i=0; i<def->nfields; i++) {
        free(cstrvalues[i]);
    }
}

static int
persistobject(struct object* object, dbsimple_session_type session)
{
    int affected = -1;
    MYSQL_STMT* insertStmt = ((MYSQL_STMT**)(object->type->implementation.p))[0];
    MYSQL_STMT* deleteStmt = ((MYSQL_STMT**)(object->type->implementation.p))[1];  
    switch(object->state) {
        case OBJUNKNOWN:
            abort();
        case OBJCLEAN:
            break;
        case OBJNEW:
            object->revision = 1;
            object->keyid = random();
            dostatement(session, insertStmt, object, 1);
            mysql_stmt_execute(insertStmt);
            break;
        case OBJMODIFIED:
            dostatement(session, deleteStmt, object, 0);
            mysql_stmt_execute(deleteStmt);
            object->revision += 1;
            dostatement(session, insertStmt, object, 1);
            mysql_stmt_execute(insertStmt);
            break;
        case OBJREMOVED:
            dostatement(session, deleteStmt, object, 0);
            mysql_stmt_execute(deleteStmt);
            affected = mysql_affected_rows(session->handle);
            object->data = NULL; // BERRY free data?
            if(affected != 1)
                return -1;
            break;
    }
    return 0; /* continue */
}

static void*
fetchdata(dbsimple_session_type session, const char* const** query, __attribute__((unused))  va_list args)
{
    MYSQL_STMT** stmts = NULL;
    int stmtindex = query - session->connection->baseconnection.queries;
    int position = 1;
    if(stmtindex >=0 && stmtindex < session->nstmts) {
        stmts = session->stmts[stmtindex];
        for(int i=0; i<session->connection->baseconnection.ndefinitions; i++) {
            if((session->connection->baseconnection.definitions[i]->flags & dbsimple_FLAG_SINGLETON) != dbsimple_FLAG_SINGLETON) {
                session->connection->baseconnection.definitions[i]->implementation.p = &stmts[position];
                position += 2;
            }
        }
        session->currentStmt = stmts[0];
        mysql_stmt_execute(session->currentStmt);
        return dbsimple__fetch(&session->basesession, session->connection->baseconnection.ndefinitions, session->connection->baseconnection.definitions);
    } else {
        return NULL;
    }
}

static int
commitdata(dbsimple_session_type session)
{
    struct object* object;
    mysql_stmt_execute(session->beginStmt);

    for(int i=0; i<session->connection->baseconnection.ndefinitions; i++) {
        if((session->connection->baseconnection.definitions[i]->flags & dbsimple_FLAG_SINGLETON) == dbsimple_FLAG_SINGLETON) {
            object = dbsimple__getobject(&session->basesession, session->connection->baseconnection.definitions[i], 0, NULL);
            dbsimple__committraverse(&session->basesession, object);
        }
    }
    dbsimple__commit(&session->basesession);

    mysql_stmt_execute(session->commitStmt);

    return 0;
}

static const struct dbsimple_module module = {
    "mysql", openconnection, closeconnection, opensession, closesession, syncdata, fetchdata, commitdata, fetchobject, persistobject
};

#if !defined(HAVE_MYSQL) || defined(DYNAMIC_MYSQL)

int
dbsimple_mysql_initialize(char* hint)
{
    (void)hint;
    dlhandle = dlopen("libmysqlclient_r.so", RTLD_LAZY);
    if(dlhandle == NULL) {
        dlhandle = dlopen("libmysqlclient.so", RTLD_LAZY);
    }
    mysql_init = (const MYSQL*(*)(MYSQL*))functioncast(dlsym(dlhandle, "mysql_init"));

    dbsimple_registermodule(&module);
    return 0;
}

int
dbsimple_mysql_finalize(void)
{
    dlclose(dlhandle);
    return 0;
}

#else

int
dbsimple_mysql_initialize(__attribute__((unused)) char* hint)
{
    dbsimple_registermodule(&module);
    return 0;
}

int
dbsimple_mysql_finalize(void)
{
    return 0;
}

#endif

#else

int dbsimple_mysql_initialize(__attribute__((unused)) char* hint) {
    return -1;
}

int dbsimple_mysql_finalize(void) {
    return -1;
}

#endif
