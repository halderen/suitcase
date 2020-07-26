#include "config.h"

#define HAVE_SQLITE3
#undef DYNAMIC_SQLITE3

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#if !defined(HAVE_SQLITE3) || defined(DYNAMIC_SQLITE3)
#include <dlfcn.h>
#endif
#ifdef HAVE_SQLITE3_H
#include "sqlite3.h"
#endif

#include "utilities.h"
#include "tree.h"
#include "dbsimple.h"
#include "dbsimplebase.h"

static int initialize(void);
static void reportError(sqlite3* handle, char* message);

#ifndef HAVE_SQLITE3_H

int dbsimple_initialize(char* hint) {
    return -1
}

int dbsimple_finalize(void) {
    return -1;
}

#else

#if !defined(HAVE_SQLITE3) || defined(DYNAMIC_SQLITE3)

#define sqlite3_errmsg      dynamicsqlite3_errmsg
#define sqlite3_errcode     dynamicsqlite3_errcode
#define sqlite3_open_v2     dynamicsqlite3_open_v2
#define sqlite3_close       dynamicsqlite3_close
#define sqlite3_free        dynamicsqlite3_free
#define sqlite3_threadsafe  dynamicsqlite3_threadsafe
#define sqlite3_config      dynamicsqlite3_config
#define sqlite3_step        dynamicsqlite3_step
#define sqlite3_column_int  dynamicsqlite3_column_int

const char* (*sqlite3_errmsg)(sqlite3*);
int (*sqlite3_errcode)(sqlite3*);
int (*sqlite3_open_v2)(const char *filename, sqlite3 **ppDb, int, consth char*);
int (*sqlite3_close)(sqlite3*);
void (*sqlite3_free)(void*);
int (*sqlite3_threadsafe)(void);
int (*sqlite3_config)(int, ...);
int (*sqlite3_step)(sqlite3_stmt*);
int (*sqlite3_column_int)(sqlite3_stmt*, int iCol);

static void* dlhandle = NULL;

int
dbsimple_initialize(char* hint)
{
    (void)hint;
    dlhandle = dlopen("libsqlite3.so.0", RTLD_LAZY);
    sqlite3_errmsg     = (const char* (*)(sqlite3*))functioncast(dlsym(dlhandle, "sqlite3_errmsg"));
    sqlite3_errcode    = (int (*)(sqlite3*))functioncast(dlsym(dlhandle, "sqlite3_errcode"));
    sqlite3_open_v2    = (int(*)(const char *, sqlite3 **, int, const char*))functioncast(dlsym(dlhandle, "sqlite3_open_v2"));
    sqlite3_close      = (int (*)(sqlite3*))functioncast(dlsym(dlhandle, "sqlite3_close"));
    sqlite3_free       = (void (*)(void*))functioncast(dlsym(dlhandle, "sqlite3_free"));
    sqlite3_threadsafe = (int (*)())functioncast(dlsym(dlhandle, "sqlite3_threadsafe"));
    sqlite3_config     = (int (*)(int,...))functioncast(dlsym(dlhandle, "sqlite3_config"));
    sqlite3_step       = (int (*)(int,...))functioncast(dlsym(dlhandle, "sqlite3_step"));
    sqlite3_column_int = (int (*)(int,...))functioncast(dlsym(dlhandle, "sqlite3_column_int"));
    return initialize();
}

int
dbsimple_finalize(void)
{
    sqlite3_errmsg     = NULL;
    sqlite3_errcode    = NULL;
    sqlite3_open_v2    = NULL;
    sqlite3_close      = NULL;
    sqlite3_free       = NULL;
    sqlite3_threadsafe = NULL;
    sqlite3_config     = NULL;
    sqlite3_step       = NULL;
    sqlite3_column_int = NULL;
    dlclose(dlhandle);
    return 0;
}

#else

int
dbsimple_initialize(__attribute__((unused)) char* hint)
{
    return initialize();
}

int
dbsimple_finalize(void)
{
    return 0;
}

#endif

struct dbsimple_connection_struct {
    sqlite3* handle;
    char* location;
    int nqueries;
    const char* const** queries;
    int ndefinitions;
    struct dbsimple_definition** definitions;
};

struct dbsimple_session_struct {
    struct dbsimple_sessionbase basesession;
    dbsimple_connection_type connection;
    sqlite3* handle;
    int nstmts;
    sqlite3_stmt*** stmts;
    sqlite3_stmt* beginStmt;
    sqlite3_stmt* commitStmt;
    sqlite3_stmt* rollbackStmt;
    sqlite3_stmt** currentStmts;
};

static void errorcallback(__attribute__((unused)) void *data, __attribute__((unused)) int errorCode, __attribute__((unused)) const char *errorMessage) {
    //fprintf(stderr, "ERROR %s (%d)\n", errorMessage, errorCode);
}

static int initialize(void)
{
    if(!sqlite3_threadsafe()) {
        dbsimple_finalize();
        return -1;
    }
    sqlite3_config(SQLITE_CONFIG_SERIALIZED);
    sqlite3_config(SQLITE_CONFIG_LOG, errorcallback, NULL);

    return 0;
}

static void reportError(sqlite3* handle, char* message)
{
    fprintf(stderr, "SQLERROR%s%s: %s (%d, %d)\n", ((message&&*message)?" ":""), message, sqlite3_errmsg(handle), sqlite3_errcode(handle), sqlite3_extended_errcode(handle));
}

static int gobbleResult(void* a, int b, char**c, char**d)
{
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    return 0;
}

static int eatResult(sqlite3* handle, sqlite3_stmt* stmt, void* data)
{
    int status;
    int column;
    int affected = 0;
    do {
        switch ((status = sqlite3_step(stmt))) {
            case SQLITE_ROW:
                ++affected;
                column = 0;
                if(data) {
                    *((int*) data) = sqlite3_column_int(stmt, column++);
                }
                break;
            case SQLITE_DONE:
                affected = sqlite3_changes(handle);
                return affected;
            case SQLITE_BUSY:
                sleep(1);
                break;
            case SQLITE_ERROR:
                reportError(handle, "error");
                return -1;
            case SQLITE_MISUSE:
                reportError(handle, "misuse");
                return -1;
            default:
                break;
        }
    } while(status == SQLITE_BUSY || status == SQLITE_ROW);
    return affected;
}

#endif

void
dbsimple__fetchobject(struct dbsimple_definition* def, dbsimple_session_type session)
{
    int status;
    int column;
    int targetkeyid;
    const char* targetkeystr;
    struct object* object = NULL;
    int intvalue;
    char* cstrvalue;

    sqlite3* handle = session->connection->handle;
    sqlite3_stmt* stmt = session->currentStmts[def->implementation*3+0];
    sqlite3_reset(stmt);

    do {
        switch((status = sqlite3_step(stmt))) {
            case SQLITE_ROW:
                column = 0;
                for(int i = 0; i<def->nfields; i++) {
                    switch(def->fields[i].type) {
                        case dbsimple_INTEGER:
                            intvalue = sqlite3_column_int(stmt, column++);
                            if (i==0) {
                                object = dbsimple__getobject(&session->basesession, def, intvalue, NULL);
                                object->state = OBJCLEAN;
                            } else if(i==1 && (def->flags & dbsimple_FLAG_HASREVISION)) {
                                object->revision = intvalue;
                            }
                            if(def->fields[i].fieldoffset >= 0)
                                *(int*)&(object->data[def->fields[i].fieldoffset]) = intvalue;
                            break;
                        case dbsimple_STRING:
                            cstrvalue = strdup((char*)sqlite3_column_text(stmt, column++));
                            if (i==0) {
                                object = dbsimple__getobject(&session->basesession, def, 0, cstrvalue);
                                object->state = OBJCLEAN;
                            }
                            if(def->fields[i].fieldoffset >= 0)
                                *(char**)&(object->data[def->fields[i].fieldoffset]) = cstrvalue;
                            break;
                        case dbsimple_REFERENCE:
                            targetkeyid = 0;
                            targetkeystr = NULL;
                            if(sqlite3_column_type(stmt, column) != SQLITE_NULL) {
                                if((def->fields[i].def->flags & dbsimple_FLAG_SINGLETON) != dbsimple_FLAG_SINGLETON) {
                                    switch(def->fields[i].def->fields[0].type) {
                                        case dbsimple_INTEGER:
                                            targetkeyid = sqlite3_column_int(stmt, column++);
                                            break;
                                        case dbsimple_STRING:
                                            targetkeystr = strdup((char*)sqlite3_column_text(stmt, column++));
                                            break;
                                        default:
                                            abort();
                                    }
                                } else
                                    abort();
                                dbsimple__assignreference(&session->basesession, &def->fields[i], targetkeyid, targetkeystr, object);
                            } else {
                                ++column;
                                *(void**)&(object->data[def->fields[i].fieldoffset]) = NULL;
                            }
                            break;
                        case dbsimple_BACKREFERENCE:
                            targetkeyid = 0;
                            targetkeystr = NULL;
                            if((def->fields[i].def->flags & dbsimple_FLAG_SINGLETON) != dbsimple_FLAG_SINGLETON) {
                                if(sqlite3_column_type(stmt, column) != SQLITE_NULL) {
                                    switch(def->fields[i].def->fields[0].type) {
                                        case dbsimple_INTEGER:
                                            targetkeyid = sqlite3_column_int(stmt, column++);
                                            break;
                                        case dbsimple_STRING:
                                            targetkeystr = strdup((char*)sqlite3_column_text(stmt, column++));
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
                break;
            case SQLITE_DONE:
                return;
            case SQLITE_BUSY:
                sleep(1);
                break;
            case SQLITE_ERROR:
                reportError(handle, "error");
                return;
            case SQLITE_MISUSE:
                reportError(handle, "misuse");
                return;
            default:
                break;
        }
    } while(status==SQLITE_BUSY||status==SQLITE_ROW);
}

static int
bindstatement(dbsimple_session_type session, sqlite3_stmt* stmt, struct object* object, int style)
{
    int column = 0;
    int intvalue;
    const char* cstrvalue;
    struct object* targetobj;
    void* targetptr;
    struct dbsimple_field* field;

    sqlite3_clear_bindings(stmt);
    sqlite3_reset(stmt);
    column = 0;
    for(int i = 0; i<object->type->nfields; i++) {
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
                switch(i) {
                    case 0:
                        intvalue = object->keyid;
                        break;
                    case 1:
                        if(object->type->flags & dbsimple_FLAG_HASREVISION) {
                            intvalue = object->revision;
                            break;
                        }
                        /* else deliberate fall through */
                        // fall through
                    default:
                        intvalue = *(int*)&(object->data[field->fieldoffset]);
                }
                sqlite3_bind_int(stmt, ++column, intvalue);
                break;
            case dbsimple_STRING:
                switch(i) {
                    case 0:
                        cstrvalue = object->keyname;
                        break;
                    default:
                        cstrvalue = *(char**)&(object->data[field->fieldoffset]);
                }
                sqlite3_bind_text(stmt, ++column, cstrvalue, -1, SQLITE_STATIC);
                break;
            case dbsimple_REFERENCE:
                targetptr = *(void**)&(object->data[field->fieldoffset]);
                if(targetptr != NULL) {
                    targetobj = dbsimple__referencebyptr(&session->basesession, field->def, targetptr);
                    if(targetobj->keyname) {
                        cstrvalue = targetobj->keyname;
                        sqlite3_bind_text(stmt, ++column, cstrvalue, -1, SQLITE_STATIC);
                    } else {
                        intvalue = targetobj->keyid;
                        sqlite3_bind_int(stmt, ++column, intvalue);
                    }
                } else
                    sqlite3_bind_null(stmt, ++column);
                break;
            case dbsimple_BACKREFERENCE:
            case dbsimple_MASTERREFERENCES:
            case dbsimple_OPENREFERENCES:
                break;
        }
    }
    return 0;
}

int
dbsimple__persistobject(struct object* object, dbsimple_session_type session)
{
    int affected = -1;
    sqlite3_stmt* insertStmt = session->currentStmts[object->type->implementation*3+1];
    sqlite3_stmt* deleteStmt = session->currentStmts[object->type->implementation*3+2];  
    switch(object->state) {
        case OBJUNKNOWN:
            abort();
        case OBJCLEAN:
            break;
        case OBJNEW:
            object->revision = 1;
            object->keyid = random();
            bindstatement(session, insertStmt, object, 1);
            eatResult(session->handle, insertStmt, NULL);
            break;
        case OBJMODIFIED:
            bindstatement(session, deleteStmt, object, 0);
            eatResult(session->handle, deleteStmt, NULL);
            object->revision += 1;
            bindstatement(session, insertStmt, object, 1);
            eatResult(session->handle, insertStmt, NULL);
            break;
        case OBJREMOVED:
            bindstatement(session, deleteStmt, object, 0);
            eatResult(session->handle, deleteStmt, &affected);
            object->data = NULL;
            if(affected != 1)
                return -1;
            break;
    }
    return 0; /* continue */
}

int
dbsimple_commit(dbsimple_session_type session)
{
    struct object* object;
    sqlite3_reset(session->beginStmt);
    eatResult(session->handle, session->beginStmt, NULL);

    for(int i=0; i<session->connection->ndefinitions; i++) {
        if((session->connection->definitions[i]->flags & dbsimple_FLAG_SINGLETON) == dbsimple_FLAG_SINGLETON) {
            object = dbsimple__getobject(&session->basesession, session->connection->definitions[i], 0, NULL);
            dbsimple__committraverse(&session->basesession, object);
        }
    }
    dbsimple__commitprocedure(&session->basesession);

    sqlite3_reset(session->commitStmt);
    eatResult(session->handle, session->commitStmt, NULL);

    return 0;
}

int
dbsimple_openconnection(char* location,
                        int nqueries, const char* const** queries,
                        int ndefinitions, struct dbsimple_definition** definitions,
                        dbsimple_connection_type* connectionPtr)
{
    int returnCode = 0;
    int errorCode;
    int count;
    sqlite3* handle;
    //const char* query;
    (void)location;
    /* :memory: */
    if (SQLITE_OK != (errorCode = sqlite3_open_v2(location, &handle, SQLITE_OPEN_READWRITE|SQLITE_OPEN_URI|SQLITE_OPEN_CREATE, NULL))) {
        reportError(handle, "error opening database");
        connectionPtr = NULL;
        returnCode = -1;
        if(errorCode != SQLITE_CANTOPEN)
            return -1;
    }

    if(nqueries < 0) {
        for(count=0; queries[count]; count++)
            ;
        nqueries = count;
    }
    if(ndefinitions < 0) {
        for(count=0; queries[count]; count++)
            ;
        ndefinitions = count;
    }
    *connectionPtr = malloc(sizeof(struct dbsimple_connection_struct));
    (*connectionPtr)->handle    = handle;
    (*connectionPtr)->location  = strdup(location);
    (*connectionPtr)->nqueries     = nqueries;
    (*connectionPtr)->queries      = queries;
    (*connectionPtr)->ndefinitions = ndefinitions;
    (*connectionPtr)->definitions  = definitions;
    return returnCode;
}

int
dbsimple_closeconnection(dbsimple_connection_type connection)
{
    sqlite3_close(connection->handle);
    free(connection->location);
    free(connection);
    return 0;
}

int
dbsimple_opensession(dbsimple_connection_type connection, dbsimple_session_type* sessionPtr)
{
    int returnCode = 0;
    int errorCode;
    sqlite3_stmt* beginStmt;
    sqlite3_stmt* commitStmt;
    sqlite3_stmt* rollbackStmt;
    const char* beginQuery = "BEGIN TRANSACTION";
    const char* commitQuery = "COMMIT";
    const char* rollbackQuery = "ROLLBACK";

    if(SQLITE_OK != (errorCode = sqlite3_prepare_v2(connection->handle, beginQuery, strlen(beginQuery)+1, &beginStmt, NULL))) {
        reportError(connection->handle, "");
        returnCode = 1;
    }
    if(SQLITE_OK != (errorCode = sqlite3_prepare_v2(connection->handle, commitQuery, strlen(commitQuery)+1, &commitStmt, NULL))) {
        reportError(connection->handle, "");
        returnCode = 1;
    }
    if(SQLITE_OK != (errorCode = sqlite3_prepare_v2(connection->handle, rollbackQuery, strlen(rollbackQuery)+1, &rollbackStmt, NULL))) {
        reportError(connection->handle, "");
        returnCode = 1;
    }

    *sessionPtr = malloc(sizeof(struct dbsimple_session_struct));
    (*sessionPtr)->connection = connection;
    (*sessionPtr)->handle = connection->handle;
    (*sessionPtr)->nstmts = connection->nqueries;
    (*sessionPtr)->stmts = malloc(sizeof(sqlite3_stmt**) * (*sessionPtr)->nstmts);
    (*sessionPtr)->beginStmt = beginStmt;
    (*sessionPtr)->commitStmt = commitStmt;
    (*sessionPtr)->rollbackStmt = rollbackStmt;
    for(int i=0; i<connection->nqueries; i++) {
        (*sessionPtr)->stmts[i] = NULL;
    }
    return returnCode;
}

int
dbsimple_closesession(dbsimple_session_type session)
{
    sqlite3_stmt** stmts;
    for(int i=0; i<session->nstmts; i++) {
        if(session->stmts[i]) {
            for(stmts=session->stmts[i]; *stmts; stmts++) {
                sqlite3_finalize(*stmts);
            }
        }
    }
    sqlite3_finalize(session->beginStmt);
    sqlite3_finalize(session->commitStmt);
    sqlite3_finalize(session->rollbackStmt);
    free(session->stmts);
    free(session);
    return 0;
}

int
dbsimple_sync(dbsimple_session_type session, const char* const** query,  void* data)
{
    sqlite3_stmt** stmts = NULL;
    int stmtindex;
    int errorCode;
    char* errorMessage;
    stmtindex = query - session->connection->queries;
    if(stmtindex >= 0 && stmtindex < session->nstmts) {
        stmts = session->stmts[stmtindex];
        if(stmts == NULL) {
            int count = 0;
            while((*query)[count])
                ++count;
            stmts = session->stmts[stmtindex] = malloc(sizeof(sqlite3_stmt*) * (count+1));
            for(int i=0; i<count; i++) {
                if (SQLITE_OK != (errorCode = sqlite3_prepare_v2(session->handle, (*query)[i], strlen((*query)[i])+1, &stmts[i], NULL))) {
                    reportError(session->handle, "");
                    while(i-- > 0) {
                      sqlite3_finalize(stmts[i]);
                    }
                    free(session->stmts[stmtindex]);
                    session->stmts[stmtindex] = NULL;
                    return -1;
                } else {
                    sqlite3_reset(stmts[i]);
                    sqlite3_clear_bindings(stmts[i]);
                    // sqlite3_bind_text(stmt, 1, arg, -1, SQLITE_STATIC);
                    eatResult(session->handle, stmts[i], data);
                }
            }
            stmts[count] = NULL;
        } else {
          for(int i=0; stmts[i]; i++) {
            sqlite3_reset(stmts[i]);
            sqlite3_clear_bindings(stmts[i]);
            eatResult(session->handle, stmts[i], data);
          }
        }
    } else {
        if (SQLITE_OK != (errorCode = sqlite3_exec(session->handle, **query, gobbleResult, NULL, &errorMessage))) {
            reportError(session->handle, "");
            return -1;
        }
    }
    return 0;
}

void*
dbsimple_fetch(dbsimple_session_type session, const char* const** query, ...)
{
    sqlite3_stmt** stmts = NULL;
    int stmtindex;
    int errorCode;
    stmtindex = query - session->connection->queries;
    if(stmtindex >=0 && stmtindex < session->nstmts) {
        stmts = session->stmts[stmtindex];
        if(stmts == NULL) {
            int count = 0;
            while((*query)[count])
                ++count;
            stmts = session->stmts[stmtindex] = malloc(sizeof (sqlite3_stmt*) * (count+1));
            for(int i = 0; i<count; i++) {
                if(SQLITE_OK != (errorCode = sqlite3_prepare_v2(session->handle, (*query)[i], strlen((*query)[i])+1, &stmts[i], NULL))) {
                    reportError(session->handle, "");
                    while(i-- >0) {
                        sqlite3_finalize(stmts[i]);
                    }
                    free(session->stmts[stmtindex]);
                    session->stmts[stmtindex] = NULL;
                }
            }
            stmts[count] = NULL;
        }
        for(int i=stmtindex=0; i<session->connection->ndefinitions; i++)
            if((session->connection->definitions[i]->flags & dbsimple_FLAG_SINGLETON) != dbsimple_FLAG_SINGLETON) {
                session->connection->definitions[i]->implementation = stmtindex++;
            }
        session->currentStmts = stmts;
        return dbsimple__fetch(&session->basesession, session->connection->ndefinitions, session->connection->definitions);
    } else {
        return NULL;
    }
}
