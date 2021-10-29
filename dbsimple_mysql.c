#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>
#ifdef HAVE_MYSQL_H
#include <mysql/mysql.h>
#endif

#include "utilities.h"
#include "tree.h"
#include "dbsimple.h"
#include "dbsimplebase.h"

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
    //MYSQL_STMT* beginStmt;
    //MYSQL_STMT* commitStmt;
    //MYSQL_STMT* rollbackStmt;
    MYSQL_STMT** currentStmt;
    MYSQL_STMT*** stmts;
    int nstmts;
};

static inline int
preparestmt(MYSQL* handle, const char* query, MYSQL_STMT** stmtPtr)
{
    MYSQL_STMT* stmt;
    stmt = mysql_stmt_init(handle);
    if(mysql_stmt_prepare(stmt, query, -1)) {
        stmt = NULL;
        mysql_stmt_close(stmt);
        fprintf(stderr,"MySQL: Unable to prepare statement %s (%u)\n%s\n",mysql_error(handle),mysql_errno(handle),query);
        return 1;
    }
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
    for(int i = 0; i<object->type->nfields; i++) {
        int valuetype = MYSQL_TYPE_NULL;
        void* valueptr;
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
            default:
                valuetype = MYSQL_TYPE_NULL;
                break;
        }
        switch(valuetype) {
            case MYSQL_TYPE_LONG:
                assert(column < nparam);
                bind[column].buffer = valueptr;
                bind[column].buffer_type = MYSQL_TYPE_LONG;
                if(!valueptr)
                    bind[column].is_null = &isnull;
                ++column;
                break;
            case MYSQL_TYPE_STRING:
                assert(column < nparam);
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
    if(mysql_stmt_bind_param(stmt, bind)) {
        fprintf(stderr,"MySQL: Unable to bind variables to statement: %s (%u)\n",mysql_error(session->handle),mysql_errno(session->handle));
    }
    if(mysql_stmt_execute(stmt)) {
        fprintf(stderr,"MySQL: Unable to execute statement: %s (%u)\n",mysql_error(session->handle),mysql_errno(session->handle));
    }
    return 0;
}

static void
invalidatesession(dbsimple_session_type session)
{
    MYSQL_STMT** stmts;
    for(int i=0; i<session->nstmts; i++) {
        for(stmts=session->stmts[i]; stmts && *stmts; stmts++) {
            mysql_stmt_close(*stmts);
        }
    }
    free(session->stmts);
    //mysql_stmt_close(session->beginStmt);
    //mysql_stmt_close(session->commitStmt);
    //mysql_stmt_close(session->rollbackStmt);
    session->stmts = NULL;
    //session->beginStmt = session->commitStmt = session->rollbackStmt = NULL;
}

static int
validatesession(dbsimple_session_type session)
{
    int returnCode = 0;
    const char* const* queries;
    int i, j, count;
    //const char* beginQuery = "START TRANSACTION";
    //const char* commitQuery = "COMMIT";
    //const char* rollbackQuery = "ROLLBACK";
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
#ifdef NOTDEFINED
    if(!mysql_real_connect(session->handle, session->connection->hostname, session->connection->username, session->connection->password,
                           session->connection->basename, session->connection->port, session->connection->socketname, CLIENT_MULTI_STATEMENTS)) {
        fprintf(stderr,"MySQL: Unable to connect: %s (%u)\n",mysql_error(session->handle),mysql_errno(session->handle));
    }
#else
    if(!mysql_real_connect(session->handle, "localhost", "suitcase", "8xsMw6Qa", "suitcase", -1, NULL, CLIENT_MULTI_STATEMENTS|CLIENT_MULTI_RESULTS)) {
        fprintf(stderr,"MySQL: Unable to connect: %s (%u)\n",mysql_error(session->handle),mysql_errno(session->handle));
    }
#endif
    mysql_autocommit(session->handle, 1);
    //preparestmt(session->handle, beginQuery, &session->beginStmt);
    //preparestmt(session->handle, commitQuery, &session->commitStmt);
    //preparestmt(session->handle, rollbackQuery, &session->rollbackStmt);
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
                stmts = session->stmts[i] = NULL;
                break;
            }
        }
        if(stmts)
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
    //(*sessionPtr)->beginStmt = NULL;
    //(*sessionPtr)->commitStmt = NULL;
    //(*sessionPtr)->rollbackStmt = NULL;
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
    MYSQL_ROW row;
    fprintf(stderr,"BERRY#A#syncdata\n");
    stmtindex = query - session->connection->baseconnection.queries;
    if(stmtindex>=0 && stmtindex<session->nstmts) {
                fprintf(stderr, "BERRY#B\n");
        stmts = session->stmts[stmtindex];
        if(stmts && *stmts) {
        for(int i = 0; stmts && stmts[i]; i++) {
                fprintf(stderr, "BERRY#C\n");
                if(mysql_stmt_execute(stmts[i])) {
                    fprintf(stderr, "MySQL: Error executing query %s (%u)\n%s\n", mysql_error(session->handle), mysql_errno(session->handle), session->connection->baseconnection.queries[stmtindex][i]);
                }
                do {
                    fprintf(stderr, "BERRY#D\n");
                    result = mysql_store_result(session->handle);
                    while((row = mysql_fetch_row(result))&&row[0]) {
                        *(int*)data = atoi(row[0]);
                    }
                    mysql_free_result(result);
                } while(!mysql_stmt_next_result(stmts[i]));
            }
        } else {
    fprintf(stderr,"BERRY#a\n");
            const char* const* queries = *query;
            while(*queries) {
    fprintf(stderr,"BERRY#b %s\n",*queries);
                if(mysql_query(session->handle, *queries)) {
                    fprintf(stderr, "MySQL: Error executing query %s (%u)\n%s\n", mysql_error(session->handle), mysql_errno(session->handle), *queries);
                }
                do {
                result = mysql_store_result(session->handle);
    fprintf(stderr,"BERRY#c\n");
                    while((row = mysql_fetch_row(result))&&row[0]) {
    fprintf(stderr,"BERRY#d\n");
                        *(int*)data = atoi(row[0]);
                    }
                } while(!mysql_next_result(session->handle));
                mysql_free_result(result);
                ++queries;
            }
        }
    } else {
        mysql_query(session->handle, **query);
        result = mysql_store_result(session->handle);
        while((row = mysql_fetch_row(result)) && row[0]) {
            *(int*)data = atoi(row[0]);
        }
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

    MYSQL_STMT* stmt = *(session->currentStmt);
    ++(session->currentStmt);
    if(!stmt) {
        return;
    }
    if(mysql_stmt_execute(stmt)) {
        fprintf(stderr, "MySQL: Error executing query %s (%u)\n", mysql_error(session->handle), mysql_errno(session->handle));
    }

    MYSQL_BIND *bind;
    long* intvalues;
    char** cstrvalues;
    my_bool* nullvalues;
    bind = malloc(sizeof(MYSQL_BIND) * def->nfields);
    intvalues = malloc(sizeof(long) * def->nfields);
    cstrvalues = malloc(sizeof(char*) * def->nfields);
    nullvalues = malloc(sizeof(my_bool) * def->nfields);
    memset(bind, 0, sizeof(MYSQL_BIND) * def->nfields);
    memset(cstrvalues, 0, sizeof(char*) * def->nfields);
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
                cstrvalues[column] = malloc(10240);
                bind[column].buffer = cstrvalues[column];
                bind[column].buffer_type = MYSQL_TYPE_STRING;
                bind[column].buffer_length = 10240;
                bind[column].length = (unsigned long*)&intvalues[column];
                column += 1;
                break;
            case dbsimple_REFERENCE:
            case dbsimple_BACKREFERENCE:
                if((def->fields[i].def->flags & dbsimple_FLAG_SINGLETON) != dbsimple_FLAG_SINGLETON) {
                    switch(def->fields[i].def->fields[0].type) {
                        case dbsimple_INTEGER:
                            bind[column].buffer = &intvalues[column];
                            bind[column].buffer_type = MYSQL_TYPE_LONG;
                            break;
                        case dbsimple_STRING:
                            cstrvalues[column] = malloc(10240);
                            bind[column].buffer = cstrvalues[column];
                            bind[column].buffer_type = MYSQL_TYPE_STRING;
                            bind[column].buffer_length = 10240;
                            bind[column].length = (unsigned long*)&intvalues[column];
                            assert(cstrvalues[column]);
                            break;
                        default:
                            abort();
                    }
                    column += 1;
                }
                break;
            case dbsimple_MASTERREFERENCES:
            case dbsimple_OPENREFERENCES:
                break;
        }
    }
    mysql_stmt_bind_result(stmt, bind);

    while(!mysql_stmt_fetch(stmt)) {
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

    for(int i=0; i<def->nfields; i++)
        if(cstrvalues[i])
            free(cstrvalues[i]);
    free(cstrvalues);
    free(intvalues);
    free(nullvalues);
    free(bind);
}

static int
persistobject(struct object* object, dbsimple_session_type session)
{
    int affected = -1;
    MYSQL_STMT* insertStmt = ((MYSQL_STMT**)(object->type->implementation.p))[0];
    MYSQL_STMT* deleteStmt = ((MYSQL_STMT**)(object->type->implementation.p))[1];  
    fprintf(stderr,"BERRY#persistobject\n");
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
    int position = 0;
    if(stmtindex >=0 && stmtindex < session->nstmts && session->stmts[stmtindex] && session->stmts[stmtindex][0]) {
        session->currentStmt = stmts = session->stmts[stmtindex];
        fprintf(stderr,"BERRY#fetchdata\n");
        for(int i=0; i<session->connection->baseconnection.ndefinitions; i++) {
            if((session->connection->baseconnection.definitions[i]->flags & dbsimple_FLAG_SINGLETON) != dbsimple_FLAG_SINGLETON) {
                ++position;
            }
        }
        for(int i=0; i<session->connection->baseconnection.ndefinitions; i++) {
            if((session->connection->baseconnection.definitions[i]->flags & dbsimple_FLAG_SINGLETON) != dbsimple_FLAG_SINGLETON) {
                session->connection->baseconnection.definitions[i]->implementation.p = &stmts[position];
                assert(((MYSQL_STMT**)(session->connection->baseconnection.definitions[i]->implementation.p))[0]);
                assert(((MYSQL_STMT**)(session->connection->baseconnection.definitions[i]->implementation.p))[1]);
                position += 2;
            }
        }
        return dbsimple__fetch(&session->basesession, session->connection->baseconnection.ndefinitions, session->connection->baseconnection.definitions);
    } else {
        return NULL;
    }
}

static int
commitdata(dbsimple_session_type session)
{
    struct object* object;
    //mysql_stmt_execute(session->beginStmt);

    for(int i=0; i<session->connection->baseconnection.ndefinitions; i++) {
        if((session->connection->baseconnection.definitions[i]->flags & dbsimple_FLAG_SINGLETON) == dbsimple_FLAG_SINGLETON) {
            object = dbsimple__getobject(&session->basesession, session->connection->baseconnection.definitions[i], 0, NULL);
            dbsimple__committraverse(&session->basesession, object);
        }
    }
    dbsimple__commit(&session->basesession);

    //mysql_stmt_execute(session->commitStmt);

    return 0;
}

static const struct dbsimple_module module = {
    "mysql", openconnection, closeconnection, opensession, closesession, syncdata, fetchdata, commitdata, fetchobject, persistobject
};

int
dbsimple_mysql_initialize(void)
{
    dbsimple_registermodule(&module);
    return 0;
}

int
dbsimple_mysql_finalize(void)
{
    return 0;
}
