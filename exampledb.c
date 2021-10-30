#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sqlite3.h>
#include "dbsimple.h"
#include "exampledb.h"

#include "sqlstmts_sqlite3.inc"
#include "sqlstmts_mysql.inc"

static dbsimple_session_type session;
static dbsimple_connection_type connection;

const char* const** sqlstmts_qschema;
const char* const** sqlstmts_schema;
const char* const** sqlstmts_qschema1;
const char* const** sqlstmts_schema1;

struct dbsimple_definition datadefinition;
struct dbsimple_definition policydefinition;
struct dbsimple_definition zonedefinition;
struct dbsimple_field datafields[] = {
    { dbsimple_MASTERREFERENCES, &policydefinition, offsetof(struct data, policies), offsetof(struct data, npolicies) },
    { dbsimple_MASTERREFERENCES, &zonedefinition,   offsetof(struct data, zones),    offsetof(struct data, nzones) }
};
struct dbsimple_field policyfields[] = {
    { dbsimple_INTEGER,        NULL,                 -1,                              -1 },
    { dbsimple_STRING,         NULL,                 offsetof(struct policy, name),   -1 },
    { dbsimple_OPENREFERENCES, &zonedefinition,      offsetof(struct policy, zones),  -1 },
    { dbsimple_BACKREFERENCE,  &datadefinition,      offsetof(struct data, policies), offsetof(struct data, npolicies) }
};
struct dbsimple_field zonefields[] = {
    { dbsimple_INTEGER,        NULL,                 -1,                              -1 },
    { dbsimple_INTEGER,        NULL,                 -1,                              -1 },
    { dbsimple_STRING,         NULL,                 offsetof(struct zone, name),     -1 },
    { dbsimple_REFERENCE,      &policydefinition,    offsetof(struct zone, policy),   -1 },
    { dbsimple_BACKREFERENCE,  &policydefinition,    offsetof(struct policy, zones),  offsetof(struct policy, nzones) },
    { dbsimple_REFERENCE,      &zonedefinition,      offsetof(struct zone, parent),   -1 },
    { dbsimple_BACKREFERENCE,  &zonedefinition,      offsetof(struct zone, subzones), offsetof(struct zone, nsubzones) },
    { dbsimple_OPENREFERENCES, &zonedefinition,      offsetof(struct zone, subzones), offsetof(struct zone, nsubzones) },
    { dbsimple_BACKREFERENCE,  &datadefinition,      offsetof(struct data, zones),    offsetof(struct data, nzones) }
};
struct dbsimple_definition datadefinition =   { sizeof(struct data),   dbsimple_FLAG_SINGLETON,   sizeof(datafields)/sizeof(struct dbsimple_field),   datafields,   { 0 } };
struct dbsimple_definition policydefinition = { sizeof(struct policy), 0,                         sizeof(policyfields)/sizeof(struct dbsimple_field), policyfields, { 0 } };
struct dbsimple_definition zonedefinition =   { sizeof(struct zone),   dbsimple_FLAG_HASREVISION, sizeof(zonefields)/sizeof(struct dbsimple_field),   zonefields,   { 0 } };

static struct dbsimple_definition* mydefinitions[] = {
    &datadefinition, &policydefinition, &zonedefinition
};


int
example_dbsetup(char* location)
{
    int rcode;
    int exists;
    
    if(!strncasecmp(location,"sqlite3:",strlen("sqlite3:")) || !strcasecmp(location,"sqlite3")) {
        rcode = dbsimple_openconnection(location,
                                   sizeof(sqlstmts_sqlite3_array)/sizeof(const char*) - 1 , sqlstmts_sqlite3_array,
                                   sizeof(mydefinitions)/sizeof(struct dbsimple_definition*), mydefinitions, &connection);
        if(rcode) {
            return -1;
        }
        sqlstmts_qschema  = sqlstmts_sqlite3_qschema;
        sqlstmts_schema   = sqlstmts_sqlite3_schema;
        sqlstmts_qschema1 = sqlstmts_sqlite3_qschema1;
        sqlstmts_schema1  = sqlstmts_sqlite3_schema1;
    } else if(!strncasecmp(location,"mysql:",strlen("mysql:"))) {
        rcode = dbsimple_openconnection(location,
                                   sizeof(sqlstmts_mysql_array)/sizeof(const char*) - 1 , sqlstmts_mysql_array,
                                   sizeof(mydefinitions)/sizeof(struct dbsimple_definition*), mydefinitions, &connection);
        if(rcode) {
            return -1;
        }
        sqlstmts_qschema  = sqlstmts_mysql_qschema;
        sqlstmts_schema   = sqlstmts_mysql_schema;
        sqlstmts_qschema1 = sqlstmts_mysql_qschema1;
        sqlstmts_schema1  = sqlstmts_mysql_schema1;
    }

    dbsimple_opensession(connection, &session);

    exists = 0;
    dbsimple_sync(session, sqlstmts_qschema, &exists);
    if(exists == 0) {
        fprintf(stderr,"initializing schema\n");
        dbsimple_sync(session, sqlstmts_schema, &exists);
        dbsimple_closesession(session);
        dbsimple_opensession(connection, &session);
    } else {
        fprintf(stderr,"checking schema\n");
        exists = 0;
        dbsimple_sync(session, sqlstmts_qschema1, &exists);
        if(exists == 0) {
            fprintf(stderr,"updating schema[1]\n");
            dbsimple_sync(session, sqlstmts_schema1, &exists);
            dbsimple_closesession(session);
            dbsimple_opensession(connection, &session);
        }
    }
    fprintf(stderr,"schema complete\n");
    return 0;
}


int
example_test(void)
{
    struct data* data;
    dbsimple_opensession(connection, &session);
    dbsimple_closesession(session);

    data = dbsimple_fetch(session, sqlstmts_mysql_default);
    if(data) {
        free(data->zones[0]->name);
        data->zones[0]->name = strdup("example.net");
        data->zones[0]->policy = data->policies[0];
        dbsimple_dirty(session, data->zones[0]);
    }
    dbsimple_commit(session);

#ifdef BERRY
    data = dbsimple_fetch(session, sqlstmts_sqlite_default);
    if(data) {
        struct zone* zone = malloc(sizeof (struct zone));
        zone->name = strdup("example.nl");
        zone->policy = data->policies[0];
        zone->nsubzones = 0;
        zone->subzones = NULL;
        zone->parent = NULL;
        data->nzones += 1;
        data->zones = realloc(data->zones, sizeof (struct zone*) * data->nzones);
        data->zones[data->nzones - 1] = zone;
        dbsimple_dirty(session, zone);
    }
    dbsimple_commit(session);

    data = dbsimple_fetch(session, sqlstmts_sqlite_default);
    if(data) {
        dbsimple_delete(session, data->zones[3]);
        //free(data->zones[3]);
        data->zones[3] = NULL;
    }
    dbsimple_commit(session);

    data = dbsimple_fetch(session, sqlstmts_sqlite_default);
    if(data) {
        dbsimple_dirty(session, data->zones[3]);
        free(data->zones[3]);
        data->zones[3] = NULL;
    }
    dbsimple_commit(session);

    data = dbsimple_fetch(session, sqlstmts_mysql_default);
    printf("policies:\n");
    for(int i=0; data && data->policies && i<data->npolicies; i++) {
      printf("  %s %d\n",data->policies[i]->name, data->policies[i]->nzones);
      for(int j=0; j<data->policies[i]->nzones; j++)
          printf("    %s\n",data->policies[i]->zones[j]->name);
    }
    printf("zones:\n");
    for(int i=0; data && data->zones && i<data->nzones; i++) {
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
#endif

    return 0;
}
