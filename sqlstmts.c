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

struct dbsimple_implementation {
    int index;
};

struct dbsimple_connection_struct {
    sqlite3* handle;
    char* location;
    int nqueries;
    const char* const** queries;
    int ndefinitions;
    struct dbsimple_definition** definitions;
};

struct dbsimple_session_struct {
    dbsimple_connection_type connection;
    sqlite3* handle;
    int nstmts;
    sqlite3_stmt*** stmts;
    sqlite3_stmt* beginStmt;
    sqlite3_stmt* commitStmt;
    sqlite3_stmt* rollbackStmt;
    tree_type pointermap;
    tree_type objectmap;
    struct object* firstmodified;
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

enum objectstate { OBJUNKNOWN=0, OBJNEW, OBJCLEAN, OBJREMOVED, OBJMODIFIED };

struct object {
    struct dbsimple_definition* type;
    char* data;
    int keyid;
    const char* keyname;
    int revision;
    enum objectstate state;
    struct object* next;
};

static int
pointermapcompare(const void* a, const void* b, __attribute__((unused)) void* user)
{
    struct object* o1 = (struct object*) a;
    struct object* o2 = (struct object*) b;
    return o1->data - o2->data;
}

static int
objectmapcompare(const void* a, const void* b, __attribute__((unused)) void* user)
{
    int result;
    struct object* o1 = (struct object*) a;
    struct object* o2 = (struct object*) b;
    result = o1->type - o2->type;
    if(result == 0) {
        if(o1->keyname)
            if(o2->keyname)
                result = strcmp(o1->keyname, o2->keyname);
            else
                result = -1;
        else
            if(o2->keyname)
                result = 1;
            else
                result = o1->keyid - o2->keyid;
    }
    return result;
}

static inline struct object*
getobject(dbsimple_session_type session, struct dbsimple_definition* def, int id, char* name)
{
    struct object lookup;
    struct object* object;
    lookup.type = def;
    lookup.keyid = id;
    lookup.keyname = name;
    object = tree_lookup(session->objectmap, &lookup);
    return object;
}
        
static inline struct object*
getorcreate(dbsimple_session_type session, struct dbsimple_definition* def, int id, const char* name)
{
    tree_reference_type cursor;
    struct object lookup;
    struct object* object;
    lookup.type = def;
    lookup.keyid = id;
    lookup.keyname = name;
    object = tree_lookupref(session->objectmap, &lookup, &cursor);
    if (object == NULL) {
        object = malloc(sizeof (struct object));
        object->state = OBJUNKNOWN;
        object->type = def;
        object->data = calloc(1, def->size);
        object->keyid = id;
        object->keyname = NULL;
        object->revision = -1;
        object->next = NULL;
        tree_insertref(session->objectmap, object, &cursor);
        tree_insert(session->pointermap, object);
    }
    return object;
}

static inline struct object*
referencebyptr(dbsimple_session_type session, struct dbsimple_definition* def, void* ptr)
{
    struct object lookup;
    struct object* object;
    lookup.data = ptr;
    object = tree_lookup(session->pointermap, &lookup);
    if(object == NULL) {
        object = malloc(sizeof (struct object));
        object->type = def;
        object->data = ptr;
        object->state = OBJNEW;
        object->revision = 0;
        object->next = NULL;
        tree_insert(session->pointermap, object);
    }
    return object;
}

static inline void
assignreference(dbsimple_session_type session, struct dbsimple_definition* def, int id, const char* name, void** destination)
{
    struct object* object;
    object = getorcreate(session, def, id, name);
    if(object) {
        *destination = object->data;
    } else {
        // BERRY FIXME backpatch subscription
    }
}

static inline void
assignbackreference(dbsimple_session_type session, struct dbsimple_definition* def, int id, const char* name, struct dbsimple_field* field, struct object* object)
{
    struct object* targetobject;
    char* target;
    int* refarraycount;
    void*** refarray;
    targetobject = getorcreate(session, def, id, name);
    target = targetobject->data;
    if(target) {
        refarraycount = (int*)&(target[field->countoffset]);
        refarray = (void***)&(target[field->arrayoffset]);
        *refarray = realloc(*refarray, sizeof (void*) * (1+ *refarraycount));
        (*refarray)[*refarraycount] = object->data;
        *refarraycount += 1;
    } else {
        // BERRY FIXME backpatch subscription
    }
}

static void
fetchobject(dbsimple_session_type session, struct dbsimple_definition* def, sqlite3* handle, sqlite3_stmt* stmt)
{
    int status;
    int column;
    int targetkeyid;
    const char* targetkeystr;
    struct object* object = NULL;
    int intvalue;
    char* cstrvalue;
    
    sqlite3_reset(stmt);
    sqlite3_clear_bindings(session->beginStmt);

    do {
        switch((status = sqlite3_step(stmt))) {
            case SQLITE_ROW:
                column = 0;
                for(int i = 0; i<def->nfields; i++) {
                    switch(def->fields[i].type) {
                        case dbsimple_INTEGER:
                            intvalue = sqlite3_column_int(stmt, column++);
                            if (i==0) {
                                object = getorcreate(session, def, intvalue, NULL);
                                object->state = OBJCLEAN;
                            } else if(i==1 && (def->flags & dbsimple_FLAG_HASREVISION)) {
                                object->revision = intvalue;
                            }
                            if(def->fields[i].offset >= 0)
                                *(int*)&(object->data[def->fields[i].offset]) = intvalue;
                            break;
                        case dbsimple_STRING:
                            cstrvalue = strdup((char*)sqlite3_column_text(stmt, column++));
                            if (i==0) {
                                object = getorcreate(session, def, 0, cstrvalue);
                                object->state = OBJCLEAN;
                            }
                            if(def->fields[i].offset >= 0)
                                *(char**)&(object->data[def->fields[i].offset]) = cstrvalue;
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
                                if(def->fields[i].offset >= 0)
                                    assignreference(session, def->fields[i].def, targetkeyid, targetkeystr, (void**)&(object->data[def->fields[i].offset]));
                            } else {
                                ++column;
                                *(void**)&(object->data[def->fields[i].offset]) = NULL;
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
                                    assignbackreference(session, def->fields[i].def, targetkeyid, targetkeystr, &def->fields[i], object);
                                } else {
                                    ++column;
                                }
                            } else {
                                assignbackreference(session, def->fields[i].def, targetkeyid, targetkeystr, &def->fields[i], object);
                            }
                            break;
                        case dbsimple_FUTUREREFERENCE:
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

static sqlite3_stmt**
instantiatestmt(dbsimple_session_type session, const char* const** query)
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
                if(SQLITE_OK!=(errorCode = sqlite3_prepare_v2(session->handle, (*query)[i], strlen((*query)[i])+1, &stmts[i], NULL))) {
                    reportError(session->handle, "");
                    while(i-- >0) {
                        sqlite3_finalize(stmts[i]);
                    }
                    free(session->stmts[stmtindex]);
                    session->stmts[stmtindex] = NULL;
                    return NULL;
                }
            }
            stmts[count] = NULL;
        }
        return stmts;
    } else {
        return NULL;
    }
}

void*
dbsimple_fetch(dbsimple_session_type session, const char* const** fetchmodel, ...)
{
    sqlite3* handle = session->connection->handle;
    sqlite3_stmt** stmts;
    struct object* object = NULL;
    session->pointermap = tree_create(pointermapcompare, NULL);
    session->objectmap = tree_create(objectmapcompare, NULL);
    session->firstmodified = NULL;
    stmts = instantiatestmt(session, fetchmodel);
    for(int i=0; i<session->connection->ndefinitions; i++) {
        if((session->connection->definitions[i]->flags & dbsimple_FLAG_SINGLETON) == dbsimple_FLAG_SINGLETON) {
            object = getorcreate(session, session->connection->definitions[i], 0, NULL);
            object->state = OBJCLEAN;
        } else {
            fetchobject(session, session->connection->definitions[i], handle, *stmts);
            ++stmts;
        }
    }
    return object->data;
}

void
dbsimple_dirty(dbsimple_session_type session, void *ptr)
{
    tree_reference_type cursor;
    struct object lookup;
    struct object* object;
    assert(ptr != NULL);
    lookup.data = ptr;
    object = tree_lookupref(session->pointermap, &lookup, &cursor);
    if(object == NULL) {
        object = malloc(sizeof (struct object));
        object->type = NULL;
        object->state = OBJNEW;
        object->data = ptr;
        object->keyid = -1;
        object->keyname = NULL;
        object->revision = 0;
        object->next = NULL;
        tree_insert(session->pointermap, object);
    } else {
        object->state = OBJMODIFIED;
    }
}

void
dbsimple_delete(dbsimple_session_type session, void *ptr)
{
    struct object lookup;
    struct object* object;
    assert(ptr != NULL);
    lookup.data = ptr;
    object = tree_lookup(session->pointermap, &lookup);
    if(object != NULL) {
        object->state = OBJREMOVED;
    }
}

extern const char* const** sqlstmts_sqlite_update_zone;

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
                        intvalue = *(int*)&(object->data[field->offset]);
                }
                sqlite3_bind_int(stmt, ++column, intvalue);
                break;
            case dbsimple_STRING:
                switch(i) {
                    case 0:
                        cstrvalue = object->keyname;
                        break;
                    default:
                        cstrvalue = *(char**)&(object->data[field->offset]);
                }
                sqlite3_bind_text(stmt, ++column, cstrvalue, -1, SQLITE_STATIC);
                break;
            case dbsimple_REFERENCE:
                targetptr = *(void**)&(object->data[field->offset]);
                if(targetptr != NULL) {
                    targetobj = referencebyptr(session, field->def, targetptr);
                    intvalue = targetobj->keyid;
                    sqlite3_bind_int(stmt, ++column, intvalue);
                } else
                    sqlite3_bind_null(stmt, ++column);
                break;
            case dbsimple_BACKREFERENCE:
            case dbsimple_FUTUREREFERENCE:
                break;
        }
    }
    return 0;
}

static int
commitobject(struct object* object, dbsimple_session_type session)
{
    int affected = -1;

    if((object->type->flags & dbsimple_FLAG_SINGLETON) == dbsimple_FLAG_SINGLETON)
        return 0;

    sqlite3_stmt** stmts = instantiatestmt(session, sqlstmts_sqlite_update_zone);

    if(object->next == NULL) {
        /* this object isn't reachable */
        switch(object->state) {
            case OBJMODIFIED:
                if((object->type->flags & dbsimple_FLAG_EXPLICITREMOVE) != dbsimple_FLAG_EXPLICITREMOVE)
                    object->state = OBJREMOVED;
                break;
            case OBJCLEAN:
                if(object->type->flags & dbsimple_FLAG_AUTOREMOVE)
                    object->state = OBJREMOVED;
                break;
            case OBJREMOVED:
                break;
            default:
                abort();
        }
    }
    switch(object->state) {
        case OBJUNKNOWN:
            abort();
        case OBJCLEAN:
            break;
        case OBJNEW:
            object->revision = 1;
            object->keyid = random();
            bindstatement(session, stmts[1], object, 1);
            eatResult(session->handle, stmts[1], NULL);
            break;
        case OBJMODIFIED:
            bindstatement(session, stmts[0], object, 0);
            eatResult(session->handle, stmts[0], NULL);
            object->revision += 1;
            bindstatement(session, stmts[1], object, 1);
            eatResult(session->handle, stmts[1], NULL);
            break;
        case OBJREMOVED:
            bindstatement(session, stmts[0], object, 0);
            eatResult(session->handle, stmts[0], &affected);
            object->data = NULL;
            if(affected != 1)
                return -1;
            break;
    }
    return 0; /* continue */
}

static void
committraverse(dbsimple_session_type session, struct object* object)
{
    void* targetptr;
    struct object* targetobj;
    int* refarraycount;
    void*** refarray;
    if(object->next != NULL)
        return;
    switch(object->state) {
        case OBJUNKNOWN:
            abort();
        case OBJNEW:
        case OBJMODIFIED:
        case OBJREMOVED:
            object->next = (session->firstmodified ? session->firstmodified : object);
            session->firstmodified = object;
            break;
        case OBJCLEAN:
            object->next = object;
            break;
    }
    for(int i=0; i<object->type->nfields; i++) {
        switch(object->type->fields[i].type) {
            case dbsimple_INTEGER:
            case dbsimple_STRING:
                break;
            case dbsimple_REFERENCE:
                targetptr = *(void**)&(object->data[object->type->fields[i].offset]);
                if(targetptr) {
                    targetobj = referencebyptr(session, object->type->fields[i].def, targetptr);
                    if(targetobj->type == NULL)
                        targetobj->type = object->type->fields[i].def;
                    committraverse(session, targetobj);
                }
                break;
            case dbsimple_BACKREFERENCE:
                break;
            case dbsimple_FUTUREREFERENCE:
                refarraycount = (int*)&(object->data[object->type->fields[i].countoffset]);
                refarray = (void***)&(object->data[object->type->fields[i].arrayoffset]);
                for(int j=0; j<*refarraycount; j++) {
                    targetptr = (*refarray)[j];
                    if(targetptr) {
                        targetobj = referencebyptr(session, object->type->fields[i].def, targetptr);
                        if(targetobj->type == NULL)
                            targetobj->type = object->type->fields[i].def;
                        committraverse(session, targetobj);
                    }
                }
                break;
            default:
                abort();
        }
    }
}

static int
deleteobject(struct object* object, dbsimple_session_type session)
{
    void*** refarray;
    if(object->data) {
        for(int i=0; i<object->type->nfields; i++) {
            switch (object->type->fields[i].type) {
                case dbsimple_INTEGER:
                case dbsimple_STRING:
                case dbsimple_REFERENCE:
                case dbsimple_BACKREFERENCE:
                    break;
                case dbsimple_FUTUREREFERENCE:
                    refarray = (void***) &(object->data[object->type->fields[i].arrayoffset]);
                    free(*refarray);
                    break;
                default:
                    abort();
            }
        }
        free(object->data);
    }
    return 0; /* continue */
}

int
dbsimple_commit(dbsimple_session_type session)
{
    struct object* object = NULL;

    for(int i=0; i<session->connection->ndefinitions; i++) {
        if((session->connection->definitions[i]->flags & dbsimple_FLAG_SINGLETON) == dbsimple_FLAG_SINGLETON) {
            object = getobject(session, session->connection->definitions[i], 0, NULL);
            committraverse(session, object);
        }
    }

    sqlite3_reset(session->beginStmt);
    eatResult(session->handle, session->beginStmt, NULL);
    
    tree_foreach(session->pointermap, (tree_visitor_type) commitobject, (void*) session);
    tree_foreach(session->objectmap, deleteobject, NULL);
    tree_destroy(session->objectmap);
    tree_destroy(session->pointermap);

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
    (*sessionPtr)->firstmodified = NULL;
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


struct data;
struct policy;
struct zone;

struct data {
    int npolicies;
    struct policy** policies;
    int nzones;
    struct zone** zones;
};

struct policy {
    char* name;
    struct zone** zones;
    int nzones;
};

struct zone {
    char* name;
    struct policy* policy;
    struct zone* parent;
    struct zone** subzones;
    int nsubzones;
};

struct dbsimple_definition datadefinition;
struct dbsimple_definition policydefinition;
struct dbsimple_definition zonedefinition;
struct dbsimple_field datafields[] = {
    { dbsimple_FUTUREREFERENCE, -1, &policydefinition, offsetof(struct data, npolicies), offsetof(struct data, policies) },
    { dbsimple_FUTUREREFERENCE, -1, &zonedefinition,   offsetof(struct data, nzones), offsetof(struct data, zones) }
};
struct dbsimple_field policyfields[] = {
    { dbsimple_INTEGER,       -1,                            NULL, 0, 0 },
    { dbsimple_STRING,        offsetof(struct policy, name), NULL, 0, 0 },
    { dbsimple_BACKREFERENCE, -1,                            &datadefinition,   offsetof(struct data, npolicies), offsetof(struct data, policies) }
};
struct dbsimple_field zonefields[] = {
    { dbsimple_INTEGER,       -1,                            NULL, 0, 0 },
    { dbsimple_INTEGER,       -1,                            NULL, 0, 0 },
    { dbsimple_STRING,        offsetof(struct zone, name),   NULL, 0, 0 },
    { dbsimple_REFERENCE,     offsetof(struct zone, policy), &policydefinition, 0, 0 },
    { dbsimple_BACKREFERENCE, -1,                            &policydefinition, offsetof(struct policy, nzones), offsetof(struct policy, zones) },
    { dbsimple_REFERENCE,     offsetof(struct zone, parent), &zonedefinition,   0, 0 },
    { dbsimple_BACKREFERENCE, -1,                            &zonedefinition,   offsetof(struct zone, nsubzones), offsetof(struct zone, subzones) },
    { dbsimple_BACKREFERENCE, -1,                            &datadefinition,   offsetof(struct data, nzones), offsetof(struct data, zones) }
};
struct dbsimple_definition datadefinition =   { sizeof(struct data),   dbsimple_FLAG_SINGLETON,   sizeof(datafields)/sizeof(struct dbsimple_field), datafields,    NULL };
struct dbsimple_definition policydefinition = { sizeof(struct policy), 0,                         sizeof(policyfields)/sizeof(struct dbsimple_field), policyfields, NULL };
struct dbsimple_definition zonedefinition =   { sizeof(struct zone),   dbsimple_FLAG_HASREVISION, sizeof(zonefields)/sizeof(struct dbsimple_field),   zonefields,   NULL };

static struct dbsimple_definition* mydefinitions[] = {
    &datadefinition, &policydefinition, &zonedefinition
};

#include "sqlstmts_sqlite.inc"

int storage()
{
    int exists;

    dbsimple_initialize(NULL);

    dbsimple_session_type session;
    dbsimple_connection_type connection;

    dbsimple_openconnection("suitcase.db",
                            sizeof(sqlstmts_sqlite_array)/sizeof(const char*) - 1 , sqlstmts_sqlite_array,
                            sizeof(mydefinitions)/sizeof(struct dbsimple_definition*), mydefinitions, &connection);
    dbsimple_opensession(connection, &session);

    exists = 0;
    dbsimple_sync(session, sqlstmts_sqlite_qschema, &exists);
    if(exists == 0) {
        fprintf(stderr,"initializing schema\n");
        dbsimple_sync(session, sqlstmts_sqlite_schema, &exists);
        dbsimple_closesession(session);
        dbsimple_opensession(connection, &session);
    } else {
        dbsimple_sync(session, sqlstmts_sqlite_qschema1, &exists);
        if(exists == 0) {
            fprintf(stderr,"updating schema[1]\n");
            dbsimple_sync(session, sqlstmts_sqlite_schema1, &exists);
            dbsimple_closesession(session);
            dbsimple_opensession(connection, &session);
        }
        dbsimple_sync(session, sqlstmts_sqlite_qschema2, &exists);
        if(exists == 0) {
            fprintf(stderr,"updating schema[2]\n");        
            dbsimple_sync(session, sqlstmts_sqlite_schema2, &exists);
            dbsimple_closesession(session);
            dbsimple_opensession(connection, &session);
        }
    }
    struct data* data;

    data = dbsimple_fetch(session, sqlstmts_sqlite_default);
    free(data->zones[0]->name);
    data->zones[0]->name = strdup("example.net");
    data->zones[0]->policy = data->policies[0];
    dbsimple_dirty(session, data->zones[0]);
    dbsimple_commit(session);

    data = dbsimple_fetch(session, sqlstmts_sqlite_default);
    struct zone* zone = malloc(sizeof(struct zone));
    zone->name = strdup("example.nl");
    zone->policy =  data->policies[0];
    zone->nsubzones = 0;
    zone->subzones = NULL;
    zone->parent = NULL;
    data->nzones += 1;
    data->zones = realloc(data->zones, sizeof(struct zone*) * data->nzones);
    data->zones[data->nzones-1] = zone;
    dbsimple_dirty(session, zone);
    dbsimple_commit(session);

    data = dbsimple_fetch(session, sqlstmts_sqlite_default);
    dbsimple_delete(session, data->zones[3]);
    //free(data->zones[3]);
    data->zones[3] = NULL;
    dbsimple_commit(session);

    data = dbsimple_fetch(session, sqlstmts_sqlite_default);
    dbsimple_dirty(session, data->zones[3]);
    free(data->zones[3]);
    data->zones[3] = NULL;
    dbsimple_commit(session);

    data = dbsimple_fetch(session, sqlstmts_sqlite_default);
    printf("policies:\n");
    for(int i=0; i<data->npolicies; i++) {
      printf("  %s %d\n",data->policies[i]->name, data->policies[i]->nzones);
      for(int j=0; j<data->policies[i]->nzones; j++)
          printf("    %s\n",data->policies[i]->zones[j]->name);
    }
    printf("zones:\n");
    for(int i=0; i<data->nzones; i++) {
      printf("  %s",data->zones[i]->name);
      if(data->zones[i]->parent) {
        printf(" %s",data->zones[i]->parent->name);
      }
      printf(" %s", data->zones[i]->policy->name);
      for(int j=0; j<data->zones[i]->nsubzones; j++) {
          printf(" %s",data->zones[i]->subzones[j]->name);
      }
      printf("\n");
    }
    dbsimple_commit(session);
    
    
    dbsimple_closesession(session);
    dbsimple_closeconnection(connection);

    return 0;
}
