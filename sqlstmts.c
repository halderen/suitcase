#include "config.h"

#define HAVE_SQLITE3
#undef DYNAMIC_SQLITE3

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
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
    sqlite3_stmt** objectstmts;
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

static void eatResult(sqlite3* handle, sqlite3_stmt* stmt, void* data)
{
    int status;
    int column;
    do {
        switch ((status = sqlite3_step(stmt))) {
            case SQLITE_ROW:
                column = 0;
                *((int*) data) = sqlite3_column_int(stmt, column++);
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
    } while(status == SQLITE_BUSY || status == SQLITE_ROW);
}

#endif

enum objectstate { OBJUNKNOWN=0, OBJNEW, OBJCLEAN, OBJREMOVED, OBJMODIFIED, OBJOBSOLETE, OBJREFERENCE };

struct object {
    struct dbsimple_definition* type;
    char* data;
    int keyid;
    char* keyname;
    int revision;
    enum objectstate state;
    struct object* next;
};

static tree_type pointermap;
static tree_type objectmap;

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
                result = o1->keyname - o2->keyname;
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
getobject(struct dbsimple_definition* def, int id, char* name)
{
    struct object lookup;
    struct object* object;
    lookup.type = def;
    lookup.keyid = id;
    lookup.keyname = name;
    object = tree_lookup(objectmap, &lookup);
    return object;
}
        
static inline struct object*
getorcreate(struct dbsimple_definition* def, int id, char* name)
{
    tree_reference_type cursor;
    struct object lookup;
    struct object* object;
    lookup.type = def;
    lookup.keyid = id;
    lookup.keyname = name;
    object = tree_lookupref(objectmap, &lookup, &cursor);
    if(object==NULL) {
        object = malloc(sizeof (struct object));
        object->type = def;
        object->data = calloc(1, def->size);
        object->keyid = id;
        object->keyname = NULL;
        object->revision = -1;
        tree_insertref(objectmap, object, &cursor);
        tree_insert(pointermap, object);
    }
    return object;
}

static inline void*
referencebyid(struct dbsimple_definition* def, int id, char* name)
{
    struct object* object;
    object = getorcreate(def, id, name);
    return object ? object->data : NULL;
}

static inline struct object*
referencebyptr(struct dbsimple_definition* def, void* ptr)
{
    struct object lookup;
    struct object* object;
    lookup.data = ptr;
    object = tree_lookup(pointermap, &lookup);
    if(object == NULL) {
        object = malloc(sizeof (struct object));
        object->type = def;
        object->data = ptr;
        object->state = OBJNEW;
        // tree_insert(objectmap, object); cannot do this as we have no key
        tree_insert(pointermap, object);
    }
    return object;
}

static void
fetchobject(dbsimple_session_type session, struct dbsimple_definition* def)
{
    sqlite3* handle = session->connection->handle;
    sqlite3_stmt* stmt;
    int status;
    int column;
    int targetkeyid;
    struct object* object;
    char** target;
    int* refarraycount;
    void*** refarray;
    int intvalue;
    char* cstrvalue;
    
    stmt = session->objectstmts[def->implementation->index];
    sqlite3_reset(stmt);
    
    do {
        switch((status = sqlite3_step(stmt))) {
            case SQLITE_ROW:
                column = 0;
                for(int i = 0; i<def->nfields; i++) {
                    switch(def->fields[i].type) {
                        case 0:
                            intvalue = sqlite3_column_int(stmt, column++);
                            if (i==0) {
                                object = getorcreate(def, intvalue, NULL);
                                object->state = OBJCLEAN;
                            } else if(i==1 && (def->flags&dbsimple_FLAG_HASREVISION)) {
                                object->revision = intvalue;
                            }
                            if(def->fields[i].offset >= 0)
                                *(int*)&(object->data[def->fields[i].offset]) = intvalue;
                            break;
                        case 1:
                            cstrvalue = strdup((char*)sqlite3_column_text(stmt, column++));
                            if (i==0) {
                                object = getorcreate(def, 0, cstrvalue);
                                object->state = OBJCLEAN;
                            }
                            if(def->fields[i].offset >= 0)
                                *(char**)&(object->data[def->fields[i].offset]) = cstrvalue;
                            break;
                        case 2: // reference
                            targetkeyid = sqlite3_column_int(stmt, column++);
                            if (i==0) {
                                object = getorcreate(def->fields[i].def, targetkeyid, NULL);
                            }
                            if(def->fields[i].offset >= 0)
                                *(void**)&(object->data[def->fields[i].offset]) = referencebyid(def->fields[i].def, targetkeyid, NULL);
                            break;
                        case 3: // back-reference
                            targetkeyid = sqlite3_column_int(stmt, column++);
                            if (i==0) {
                                object = getorcreate(def->fields[i].def, targetkeyid, NULL);
                            }
                            target = referencebyid(def->fields[i].def, targetkeyid, NULL);
                            refarraycount = (int*)&(target[def->fields[i].countoffset]);
                            refarray = (void***)&(target[def->fields[i].arrayoffset]);
                            *refarray = realloc(*refarray, sizeof (void*) * (1 + *refarraycount));
                            (*refarray)[*refarraycount] = object->data;
                            *refarraycount += 1;
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

void*
dbsimple_fetch(dbsimple_session_type session, __attribute__((unused)) const char* fetchmodel, ...)
{
    struct object* object = NULL;
    pointermap = tree_create(pointermapcompare, NULL);
    objectmap = tree_create(objectmapcompare, NULL);
    for(int i=0; i<session->connection->ndefinitions; i++) {
        if(session->connection->definitions[i]->nfields == 0) {
            object = getorcreate(session->connection->definitions[i], 0, NULL);
        } else {
            fetchobject(session, session->connection->definitions[i]);
        }
    }
    return object;
}

void
dbsimple_dirty(void *ptr)
{
    tree_reference_type cursor;
    struct object lookup;
    struct object* object;
    lookup.data = ptr;
    object = tree_lookupref(pointermap, &lookup, &cursor);
    if(object==NULL) {
        object = malloc(sizeof (struct object));
        object->type = NULL;
        object->data = ptr;
        object->keyid = -1;
        object->keyname = NULL;
        object->revision = -1;
        tree_insert(pointermap, object);
    }
}

static void
commitobject(dbsimple_session_type session, struct object* object)
{
    sqlite3_stmt* stmt = NULL; //BERRY
    int column = 0;
    int intvalue;
    char* cstrvalue;
    struct object* targetobj;
    void* targetptr;
    int refarraycount;
    void** refarray;
    if(object->type->nfields <= 0)
        return;

    column = 0;
    if(object->state != OBJNEW) {
        sqlite3_reset(stmt);
        // sqlite3_bind_text(stmt, 1, arg, -1, SQLITE_STATIC);

        if(object->type->fields[0].type == 0) {
            sqlite3_bind_int(stmt, column++, object->keyid);
        } else if(object->type->fields[0].type == 1) {
            sqlite3_bind_text(stmt, column++, object->keyname, -1, SQLITE_STATIC);
        } else {
            abort();
        }
        if(object->type->flags & dbsimple_FLAG_HASREVISION) {
            sqlite3_bind_int(stmt, column++, object->revision);
            object->revision += 1;
        }
        eatResult(session->handle, stmt, NULL);
    } else {
        object->keyid = random();
    }

    column = 0;
    switch(object->type->fields[0].type) {
        case 0:
            sqlite3_bind_int(stmt, column++, object->keyid);
            break;
        case 1:
            sqlite3_bind_text(stmt, column++, object->keyname, -1, SQLITE_STATIC);
            break;
    }
    for(int i = 1; i<object->type->nfields; i++) {
        switch(object->type->fields[i].type) {
            case 0:
                if(i==1 && (object->type->flags & dbsimple_FLAG_HASREVISION))
                    intvalue = object->revision;
                else
                    intvalue = *(int*)&(object->data[object->type->fields[i].offset]);
                sqlite3_bind_int(stmt, column++, intvalue);
                break;
            case 1:
                cstrvalue = *(char**)&(object->data[object->type->fields[i].offset]);
                sqlite3_bind_text(stmt, column++, cstrvalue, -1, SQLITE_STATIC);
                break;
            case 2: // reference
                targetptr = *(void**)&(object->data[object->type->fields[i].offset]);
                targetobj = referencebyptr(object->type->fields[i].def, targetptr);
                intvalue = targetobj->keyid;
                sqlite3_bind_int(stmt, column++, intvalue);
                break;
            case 3:
                break;
        }
    }    
    for(int i = 1; i<object->type->nfields; i++) {
        switch(object->type->fields[i].type) {
            case 2:
                targetptr = *(void**)&(object->data[object->type->fields[i].offset]);
                targetobj = referencebyptr(object->type->fields[i].def, targetptr);
                commitobject(session, targetobj);
                break;
            case 4:
                refarraycount = *(int*)&(object->data[object->type->fields[i].offset]);
                refarray = *(void***)&(object->data[object->type->fields[i].offset]);
                for(int j=0; j<refarraycount; j++) {
                    targetobj = referencebyptr(object->type->fields[j].def, refarray[j]);
                    if(targetobj)
                        commitobject(session, targetobj);
                }
                break;
        }
    }    
}

int
dbsimple_commit(dbsimple_session_type session)
{
    struct object* object = NULL;
    for(int i=0; i<session->connection->ndefinitions; i++) {
        if(session->connection->definitions[i]->nfields == 0) {
            object = getobject(session->connection->definitions[i], 0, NULL);
            commitobject(session, object);
        }
    }
    if(object)
        tree_remove(objectmap, object);
    // BERRY tree_foreach(objectmap, deletefunc, NULL);
    tree_destroy(objectmap);
    tree_destroy(pointermap);
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
    *sessionPtr = malloc(sizeof(struct dbsimple_session_struct));
    (*sessionPtr)->connection = connection;
    (*sessionPtr)->handle = connection->handle;
    (*sessionPtr)->nstmts = connection->nqueries;
    (*sessionPtr)->stmts = malloc(sizeof(sqlite3_stmt**) * (*sessionPtr)->nstmts);
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
    free(session->stmts);
    free(session);
    return 0;
}

int
dbsimple_sync(dbsimple_session_type session, const char* const** query,  __attribute__((unused)) void* data)
{
    sqlite3_stmt** stmts = NULL;
    int stmtindex;
    int errorCode;
    char* errorMessage;
    stmtindex = query - session->connection->queries;
    /*
    if(stmtindex < 0 && stmtindex >= session->nstmts) {
        for(int i=0; i<session->connection->nqueries; i++) {
          if(session->connection->queries[i] == *query) {
              *query = session->connection->queries[i];
              stmtindex = i;
              break;
          }
    }
    */
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
                    // sqlite3_bind_text(stmt, 1, arg, -1, SQLITE_STATIC);
                    eatResult(session->handle, stmts[i], data);
                }
            }
            stmts[count] = NULL;
        } else {
          for(int i=0; stmts[i]; i++) {
            sqlite3_reset(stmts[i]);
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
    int *npolicies;
    struct policy** policies;
    int *nzones;
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
    struct zone** subzones;
    int nsubzones;
};

struct dbsimple_definition datadefinition;
struct dbsimple_definition policydefinition;
struct dbsimple_definition zonedefinition;
struct dbsimple_field policyfields[2] = {
    { 1, offsetof(struct policy, name), NULL, 0, 0 },
    { 3, -1,                            &datadefinition,   offsetof(struct data, npolicies), offsetof(struct data, policies) }
};
struct dbsimple_field zonefields[6] = {
    { 0, -1,                            NULL, 0, 0 },
    { 1, offsetof(struct zone, name),   NULL, 0, 0 },
    { 2, offsetof(struct zone, policy), NULL, 0, 0 },
    { 3, -1,                            &policydefinition, offsetof(struct policy, nzones), offsetof(struct policy, zones) },
    { 3, -1,                            &zonedefinition,   offsetof(struct zone, nsubzones), offsetof(struct zone, subzones) },
    { 3, -1,                            &datadefinition,   offsetof(struct data, nzones), offsetof(struct data, zones) }
};
struct dbsimple_definition datadefinition =   { sizeof(struct data),   0, 0, NULL, NULL };
struct dbsimple_definition policydefinition = { sizeof(struct policy), 0, sizeof(policyfields)/sizeof(struct dbsimple_field), policyfields, NULL };
struct dbsimple_definition zonedefinition =   { sizeof(struct zone),   1, sizeof(zonefields)/sizeof(struct dbsimple_field),   zonefields, NULL };

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
    fprintf(stderr,"so whats uso whats up\n");
    struct data* data;
    data = dbsimple_fetch(session, "default");
    for(int i=0; i<*data->npolicies; i++) {
      printf("  %s\n",data->policies[i]->name);
    }
    dbsimple_commit(session);
    dbsimple_closesession(session);
    dbsimple_closeconnection(connection);

    return 0;
}
