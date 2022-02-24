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
#include <string.h>
#include <stddef.h>
#include <sqlite3.h>
#include <time.h>
#include "dbsimple.h"
#include "exampledb.h"

#include "sqlstmts_sqlite3.inc"
#include "sqlstmts_mysql.inc"

static dbsimple_session_type session;
static dbsimple_connection_type connection;

extern dbsimple_fetchplan_reference fetchplanQschema;
extern dbsimple_fetchplan_reference fetchplanQschema1;
extern dbsimple_fetchplan_reference fetchplanSchema;
extern dbsimple_fetchplan_reference fetchplanSchema1;
extern dbsimple_fetchplan_reference fetchplanProbe;
extern dbsimple_fetchplan_reference fetchplanDefault;

static dbsimple_fetchplan_type qschema      = NULL;
static dbsimple_fetchplan_type qschema1     = NULL;
static dbsimple_fetchplan_type schema       = NULL;
static dbsimple_fetchplan_type schema1      = NULL;
static dbsimple_fetchplan_type probe        = NULL;
static dbsimple_fetchplan_type defaultfetch = NULL;

static dbsimple_fetchplan_array fetchplans = {
    &qschema, &qschema1, &schema, &schema1, &probe, &defaultfetch
};

dbsimple_fetchplan_reference fetchplanQschema  = &fetchplans[0];
dbsimple_fetchplan_reference fetchplanQschema1 = &fetchplans[1];
dbsimple_fetchplan_reference fetchplanSchema   = &fetchplans[2];
dbsimple_fetchplan_reference fetchplanSchema1  = &fetchplans[3];
dbsimple_fetchplan_reference fetchplanProbe    = &fetchplans[4];
dbsimple_fetchplan_reference fetchplanDefault  = &fetchplans[5];

struct dbsimple_definition dbw_datadefinition;
struct dbsimple_definition dbw_policydefinition;
struct dbsimple_definition dbw_policykeydefinition;
struct dbsimple_definition dbw_hsmkeydefinition;
struct dbsimple_definition dbw_zonedefinition;
struct dbsimple_definition dbw_keydefinition;
struct dbsimple_definition dbw_keystatedefinition;
struct dbsimple_definition dbw_keydependencydefinition;
struct dbsimple_definition dbw_keystatedefinition;
struct dbsimple_definition dbw_keydependencydefinition;

struct dbsimple_field dbw_datafields[] = {
    { dbsimple_MASTERREFERENCES, &dbw_policydefinition,    offsetof(struct dbw_data, policies),   offsetof(struct dbw_data, npolicies) },
    { dbsimple_MASTERREFERENCES, &dbw_policykeydefinition, -1, -1},
    { dbsimple_MASTERREFERENCES, &dbw_hsmkeydefinition,    offsetof(struct dbw_data, hsmkeys),    offsetof(struct dbw_data, nhsmkeys) },
    { dbsimple_MASTERREFERENCES, &dbw_zonedefinition,      offsetof(struct dbw_data, zones),      offsetof(struct dbw_data, nzones) },
};

struct dbsimple_field dbw_policyfields[] = {
    { dbsimple_LONGINT,        &dbw_policydefinition,    offsetof(struct dbw_policy, id),                          -1 },
    { dbsimple_INT,            NULL,                     -1, -1 }, // [rev]
    { dbsimple_STRING,         &dbw_policydefinition,    offsetof(struct dbw_policy, name),                        -1 },
    { dbsimple_STRING,         &dbw_policydefinition,    offsetof(struct dbw_policy, description),                 -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, passthrough),                 -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, signatures_resign),           -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, signatures_refresh),          -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, signatures_jitter),           -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, signatures_inception_offset), -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, signatures_validity_default), -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, signatures_validity_denial),  -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, signatures_validity_keyset),  -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, signatures_max_zone_ttl),     -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, denial_type),                 -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, denial_optout),               -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, denial_ttl),                  -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, denial_resalt),               -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, denial_algorithm),            -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, denial_iterations),           -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, denial_salt_length),          -1 },
    { dbsimple_STRING,         &dbw_policydefinition,    offsetof(struct dbw_policy, denial_salt),                 -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, denial_salt_last_change),     -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, keys_ttl),                    -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, keys_retire_safety),          -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, keys_publish_safety),         -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, keys_shared),                 -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, keys_purge_after),            -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, zone_propagation_delay),      -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, zone_soa_ttl),                -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, zone_soa_minimum),            -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, zone_soa_serial),             -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, parent_registration_delay),   -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, parent_propagation_delay),    -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, parent_ds_ttl),               -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, parent_soa_ttl),              -1 },
    { dbsimple_UINT,           &dbw_policydefinition,    offsetof(struct dbw_policy, parent_soa_minimum),          -1 },
    { dbsimple_BACKREFERENCE,  &dbw_datadefinition,      offsetof(struct dbw_data, policies),    offsetof(struct dbw_data, npolicies) },
    { dbsimple_OPENREFERENCES, &dbw_policykeydefinition, offsetof(struct dbw_policy, policykey), offsetof(struct dbw_policy, policykey_count) },
    { dbsimple_OPENREFERENCES, &dbw_hsmkeydefinition,    offsetof(struct dbw_policy, hsmkey),    offsetof(struct dbw_policy, hsmkey_count) },
    { dbsimple_OPENREFERENCES, &dbw_zonedefinition,      offsetof(struct dbw_policy, zone),      offsetof(struct dbw_policy, zone_count) },
};

struct dbsimple_field dbw_policykeyfields[] = {
    { dbsimple_LONGINT,        &dbw_policykeydefinition,    offsetof(struct dbw_policykey, id),              -1 },
    { dbsimple_INT,            NULL,                     -1, -1 }, // [rev]
    { dbsimple_REFERENCE,      &dbw_policykeydefinition,    offsetof(struct dbw_policykey, policy),          -1 },
    { dbsimple_BACKREFERENCE,  &dbw_policydefinition,       offsetof(struct dbw_policy,    policykey),       offsetof(struct dbw_policy, policykey_count) },
    { dbsimple_STRING,         &dbw_policykeydefinition,    offsetof(struct dbw_policykey, repository),      -1 },
    { dbsimple_UINT,           &dbw_policykeydefinition,    offsetof(struct dbw_policykey, role),            -1 },
    { dbsimple_UINT,           &dbw_policykeydefinition,    offsetof(struct dbw_policykey, algorithm),       -1 },
    { dbsimple_UINT,           &dbw_policykeydefinition,    offsetof(struct dbw_policykey, bits),            -1 },
    { dbsimple_UINT,           &dbw_policykeydefinition,    offsetof(struct dbw_policykey, lifetime),        -1 },
    { dbsimple_UINT,           &dbw_policykeydefinition,    offsetof(struct dbw_policykey, manual_rollover), -1 },
    { dbsimple_UINT,           &dbw_policykeydefinition,    offsetof(struct dbw_policykey, rfc5011),         -1 },
    { dbsimple_UINT,           &dbw_policykeydefinition,    offsetof(struct dbw_policykey, standby),         -1 },
    { dbsimple_UINT,           &dbw_policykeydefinition,    offsetof(struct dbw_policykey, minimize),        -1 }
};

struct dbsimple_field dbw_hsmkeyfields[] = {
    { dbsimple_LONGINT,        &dbw_hsmkeydefinition,       offsetof(struct dbw_hsmkey, id),                 -1 },
    { dbsimple_INT,            NULL,                     -1, -1 }, // [rev]
    { dbsimple_STRING,         &dbw_hsmkeydefinition,       offsetof(struct dbw_hsmkey, locator),            -1 },
    { dbsimple_STRING,         &dbw_hsmkeydefinition,       offsetof(struct dbw_hsmkey, repository),         -1 },
    { dbsimple_UINT,           &dbw_hsmkeydefinition,       offsetof(struct dbw_hsmkey, state),              -1 },
    { dbsimple_UINT,           &dbw_hsmkeydefinition,       offsetof(struct dbw_hsmkey, bits),               -1 },
    { dbsimple_UINT,           &dbw_hsmkeydefinition,       offsetof(struct dbw_hsmkey, algorithm),          -1 },
    { dbsimple_UINT,           &dbw_hsmkeydefinition,       offsetof(struct dbw_hsmkey, role),               -1 },
    { dbsimple_LONGINT,        &dbw_hsmkeydefinition,       offsetof(struct dbw_hsmkey, inception),          -1 },
    { dbsimple_UINT,           &dbw_hsmkeydefinition,       offsetof(struct dbw_hsmkey, is_revoked),         -1 },
    { dbsimple_UINT,           &dbw_hsmkeydefinition,       offsetof(struct dbw_hsmkey, key_type),           -1 },
    { dbsimple_UINT,           &dbw_hsmkeydefinition,       offsetof(struct dbw_hsmkey, backup),             -1 },
    { dbsimple_BACKREFERENCE,  &dbw_policydefinition,       offsetof(struct dbw_policy, hsmkey),             offsetof(struct dbw_policy, hsmkey_count) },
    { dbsimple_BACKREFERENCE,  &dbw_datadefinition,         offsetof(struct dbw_data,   hsmkeys),            offsetof(struct dbw_data, nhsmkeys) },
    { dbsimple_OPENREFERENCES, &dbw_hsmkeydefinition,       offsetof(struct dbw_hsmkey, key),                -1 },
};

struct dbsimple_field dbw_zonefields[] = {
    { dbsimple_LONGINT,        &dbw_zonedefinition,       offsetof(struct dbw_zone, id),                     -1 },
    { dbsimple_INT,            NULL,                     -1, -1 }, // [rev]
    { dbsimple_REFERENCE,      &dbw_policydefinition,     offsetof(struct dbw_zone, policy),                 -1 },
    { dbsimple_BACKREFERENCE,  &dbw_policydefinition,     offsetof(struct dbw_policy, zone),                 offsetof(struct dbw_policy, zone_count) },
    { dbsimple_OPENREFERENCES, &dbw_zonedefinition,       offsetof(struct dbw_zone, key),                    offsetof(struct dbw_zone, key_count) },
    { dbsimple_STRING,         &dbw_zonedefinition,       offsetof(struct dbw_zone, name),                   -1 },
    { dbsimple_LONGINT,        &dbw_zonedefinition,       offsetof(struct dbw_zone, next_change),            -1 },
    { dbsimple_STRING,         &dbw_zonedefinition,       offsetof(struct dbw_zone, signconf_path),          -1 },
    { dbsimple_STRING,         &dbw_zonedefinition,       offsetof(struct dbw_zone, input_adapter_uri),      -1 },
    { dbsimple_STRING,         &dbw_zonedefinition,       offsetof(struct dbw_zone, input_adapter_type),     -1 },
    { dbsimple_STRING,         &dbw_zonedefinition,       offsetof(struct dbw_zone, output_adapter_uri),     -1 },
    { dbsimple_STRING,         &dbw_zonedefinition,       offsetof(struct dbw_zone, output_adapter_type),    -1 },
    { dbsimple_LONGINT,        &dbw_zonedefinition,       offsetof(struct dbw_zone, next_ksk_roll),          -1 },
    { dbsimple_LONGINT,        &dbw_zonedefinition,       offsetof(struct dbw_zone, next_zsk_roll),          -1 },
    { dbsimple_LONGINT,        &dbw_zonedefinition,       offsetof(struct dbw_zone, next_csk_roll),          -1 },
    { dbsimple_UINT,           &dbw_zonedefinition,       offsetof(struct dbw_zone, signconf_needs_writing), -1 },
    { dbsimple_LONGINT,        &dbw_zonedefinition,       offsetof(struct dbw_zone, ttl_end_ds),             -1 },
    { dbsimple_LONGINT,        &dbw_zonedefinition,       offsetof(struct dbw_zone, ttl_end_dk),             -1 },
    { dbsimple_LONGINT,        &dbw_zonedefinition,       offsetof(struct dbw_zone, ttl_end_rs),             -1 },
    { dbsimple_UINT,           &dbw_zonedefinition,       offsetof(struct dbw_zone, roll_ksk_now),           -1 },
    { dbsimple_UINT,           &dbw_zonedefinition,       offsetof(struct dbw_zone, roll_zsk_now),           -1 },
    { dbsimple_UINT,           &dbw_zonedefinition,       offsetof(struct dbw_zone, roll_csk_now),           -1 },
    { dbsimple_BACKREFERENCE,  &dbw_datadefinition,       offsetof(struct dbw_data, zones),                  offsetof(struct dbw_data, nzones) },
};

struct dbsimple_field dbw_keyfields[] = {
    { dbsimple_LONGINT,        &dbw_keydefinition,           offsetof(struct dbw_key, id),                     -1 },
    { dbsimple_INT,            NULL, -1, -1 }, // [rev]
    { dbsimple_REFERENCE,      &dbw_zonedefinition,          offsetof(struct dbw_key, zone),                -1 },
    { dbsimple_BACKREFERENCE,  &dbw_zonedefinition,          offsetof(struct dbw_zone, key),                offsetof(struct dbw_zone, key_count) },
    { dbsimple_REFERENCE,      &dbw_hsmkeydefinition,        offsetof(struct dbw_key, hsmkey),              -1 },
    { dbsimple_OPENREFERENCES, &dbw_keystatedefinition,      offsetof(struct dbw_key, keystate),            offsetof(struct dbw_key, keystate_count) },
    { dbsimple_OPENREFERENCES, &dbw_keydependencydefinition, offsetof(struct dbw_key, from_keydependency),  offsetof(struct dbw_key, from_keydependency_count) },
    { dbsimple_OPENREFERENCES, &dbw_keydependencydefinition, offsetof(struct dbw_key, to_keydependency),    offsetof(struct dbw_key, to_keydependency_count) },
    { dbsimple_UINT,           &dbw_keydefinition,           offsetof(struct dbw_key, algorithm),           -1 },
    { dbsimple_LONGINT,        &dbw_keydefinition,           offsetof(struct dbw_key, inception),           -1 },
    { dbsimple_UINT,           &dbw_keydefinition,           offsetof(struct dbw_key, role),                -1 },
    { dbsimple_UINT,           &dbw_keydefinition,           offsetof(struct dbw_key, introducing),         -1 },
    { dbsimple_UINT,           &dbw_keydefinition,           offsetof(struct dbw_key, should_revoke),       -1 },
    { dbsimple_UINT,           &dbw_keydefinition,           offsetof(struct dbw_key, standby),             -1 },
    { dbsimple_UINT,           &dbw_keydefinition,           offsetof(struct dbw_key, active_zsk),          -1 },
    { dbsimple_UINT,           &dbw_keydefinition,           offsetof(struct dbw_key, publish),             -1 },
    { dbsimple_UINT,           &dbw_keydefinition,           offsetof(struct dbw_key, active_ksk),          -1 },    
    { dbsimple_UINT,           &dbw_keydefinition,           offsetof(struct dbw_key, ds_at_parent),        -1 },
    { dbsimple_UINT,           &dbw_keydefinition,           offsetof(struct dbw_key, keytag),              -1 },
    { dbsimple_UINT,           &dbw_keydefinition,           offsetof(struct dbw_key, minimize),            -1 },
};

struct dbsimple_field dbw_keystatefields[] = {
    { dbsimple_LONGINT,        &dbw_keystatedefinition, offsetof(struct dbw_keystate, id),          -1 },
    { dbsimple_INT,            NULL, -1, -1 }, // [rev]
    { dbsimple_BACKREFERENCE,  &dbw_keydefinition,      offsetof(struct dbw_key,  keystate),        offsetof(struct dbw_key, keystate_count) },
    { dbsimple_UINT,           &dbw_keystatedefinition, offsetof(struct dbw_keystate, type),        -1 },
    { dbsimple_UINT,           &dbw_keystatedefinition, offsetof(struct dbw_keystate, state),       -1 },
    { dbsimple_LONGINT,        &dbw_keystatedefinition, offsetof(struct dbw_keystate, last_change), -1 },
    { dbsimple_UINT,           &dbw_keystatedefinition, offsetof(struct dbw_keystate, minimize),    -1 },
    { dbsimple_UINT,           &dbw_keystatedefinition, offsetof(struct dbw_keystate, ttl),         -1 },
};

struct dbsimple_field dbw_keydependencyfields[] = {
    { dbsimple_INT,            &dbw_keydependencydefinition, offsetof(struct dbw_keydependency, id),      -1 },
    { dbsimple_INT,            NULL, -1, -1 }, // [rev]
    { dbsimple_REFERENCE,      &dbw_keydefinition,           offsetof(struct dbw_keydependency, fromkey), -1 },
    { dbsimple_BACKREFERENCE,  &dbw_keydefinition,           offsetof(struct dbw_key, zone),              offsetof(struct dbw_key, from_keydependency_count) },
    { dbsimple_REFERENCE,      &dbw_keydefinition,           offsetof(struct dbw_keydependency, tokey),   -1 },
    { dbsimple_BACKREFERENCE,  &dbw_keydefinition,           offsetof(struct dbw_key, zone),              offsetof(struct dbw_key, to_keydependency_count) },
    { dbsimple_UINT,           &dbw_keydependencydefinition, offsetof(struct dbw_keydependency, type),    -1 },
};

struct dbsimple_definition dbw_datadefinition =          { sizeof(struct dbw_data),          dbsimple_FLAG_SINGLETON,   sizeof(dbw_datafields)/sizeof(struct dbsimple_field),          dbw_datafields };
struct dbsimple_definition dbw_policydefinition =        { sizeof(struct dbw_policy),        dbsimple_FLAG_HASREVISION, sizeof(dbw_policyfields)/sizeof(struct dbsimple_field),        dbw_policyfields,  };
struct dbsimple_definition dbw_policykeydefinition =     { sizeof(struct dbw_policykey),     dbsimple_FLAG_HASREVISION, sizeof(dbw_policykeyfields)/sizeof(struct dbsimple_field),     dbw_policykeyfields };
struct dbsimple_definition dbw_zonedefinition =          { sizeof(struct dbw_zone),          dbsimple_FLAG_HASREVISION, sizeof(dbw_zonefields)/sizeof(struct dbsimple_field),          dbw_zonefields };
struct dbsimple_definition dbw_keydefinition =           { sizeof(struct dbw_key),           dbsimple_FLAG_HASREVISION, sizeof(dbw_keyfields)/sizeof(struct dbsimple_field),           dbw_keyfields };
struct dbsimple_definition dbw_keystatedefinition =      { sizeof(struct dbw_keystate),      dbsimple_FLAG_HASREVISION, sizeof(dbw_keystatefields)/sizeof(struct dbsimple_field),      dbw_keystatefields };
struct dbsimple_definition dbw_keydependencydefinition = { sizeof(struct dbw_keydependency), 0,                         sizeof(dbw_keydependencyfields)/sizeof(struct dbsimple_field), dbw_keydependencyfields };
struct dbsimple_definition dbw_hsmkeydefinition =        { sizeof(struct dbw_hsmkey),        dbsimple_FLAG_HASREVISION, sizeof(dbw_hsmkeyfields)/sizeof(struct dbsimple_field),        dbw_hsmkeyfields };

static struct dbsimple_definition* dbw_definitions[] = {
    &dbw_datadefinition,
    &dbw_hsmkeydefinition,
    &dbw_policydefinition,
    &dbw_policykeydefinition,
    &dbw_zonedefinition,
    &dbw_keydefinition,
    &dbw_keystatedefinition,
    &dbw_keydependencydefinition
};

int
example_dbsetup(char* location)
{
    int rcode;
    int exists;

    rcode = dbsimple_openconnection(location, sizeof(fetchplans)/sizeof(dbsimple_fetchplan_reference), fetchplans,
                                    sizeof(dbw_definitions)/sizeof(struct dbsimple_definition*), dbw_definitions, &connection);
    dbsimple_fetchplan(&qschema,      connection, "sqlite3", sqlstmts_sqlite3_qschema_);
    dbsimple_fetchplan(&qschema1,     connection, "sqlite3", sqlstmts_sqlite3_qschema1_);
    dbsimple_fetchplan(&schema,       connection, "sqlite3", sqlstmts_sqlite3_schema_);
    dbsimple_fetchplan(&schema1,      connection, "sqlite3", sqlstmts_sqlite3_schema1_);
    dbsimple_fetchplan(&probe,        connection, "sqlite3", sqlstmts_sqlite3_probe_);
    dbsimple_fetchplan(&defaultfetch, connection, "sqlite3", sqlstmts_sqlite3_default_);
    dbsimple_fetchplan(&qschema,      connection, "mysql",   sqlstmts_mysql_qschema_);
    dbsimple_fetchplan(&qschema1,     connection, "mysql",   sqlstmts_mysql_qschema1_);
    dbsimple_fetchplan(&schema,       connection, "mysql",   sqlstmts_mysql_schema_);
    dbsimple_fetchplan(&schema1,      connection, "mysql",   sqlstmts_mysql_schema1_);
    dbsimple_fetchplan(&probe,        connection, "mysql",   sqlstmts_mysql_probe_);
    dbsimple_fetchplan(&defaultfetch, connection, "mysql",   sqlstmts_mysql_default_);
    if(rcode) {
        return -1;
    }
    dbsimple_opensession(connection, &session);

    exists = 0;
    dbsimple_sync(session, fetchplanQschema, &exists);
    if(exists == 0) {
        fprintf(stderr,"initializing schema\n");
        rcode = dbsimple_sync(session, fetchplanSchema, &exists);
        if(rcode)
            fprintf(stderr,"initializing schema failed\n");
        dbsimple_closesession(session);
        dbsimple_opensession(connection, &session);
    } else {
        fprintf(stderr,"checking schema\n");
        exists = 0;
        dbsimple_sync(session, fetchplanQschema1, &exists);
        if(exists == 0) {
            fprintf(stderr,"updating schema[1]\n");
            rcode = dbsimple_sync(session, fetchplanSchema1, &exists);
            if(rcode)
                fprintf(stderr,"updating schema[1] failed\n");
            dbsimple_closesession(session);
            dbsimple_opensession(connection, &session);
        }
    }
    fprintf(stderr,"schema complete\n");
    
    dbsimple_closesession(session);
    return 0;
}


static char* strtime(unsigned long seconds)
{
    time_t t = seconds;
    char* s = ctime(&t);
    s[strlen(s)-1]='\0';
    return s;
}

int
example_dbtest(void)
{
    struct dbw_data* data;
    dbsimple_opensession(connection, &session);

    data = dbsimple_fetch(session, fetchplanDefault);
    if(data) {
        printf("%d hsmkeys\n",data->nhsmkeys);
        for(int i = 0; i<data->nhsmkeys; i++) {
            printf("hsmkey %ld\n",data->hsmkeys[i]->id);
            printf("  repository  %s\n",data->hsmkeys[i]->repository);
            printf("  locator     %s\n",data->hsmkeys[i]->locator);
            printf("  inception   %s\n",strtime(data->hsmkeys[i]->inception));
            printf("  state=%d bits=%d algo=%d role=%d revoked=%d keytype=%d backup=%d\n",
                   data->hsmkeys[i]->state,
                   data->hsmkeys[i]->bits,
                   data->hsmkeys[i]->algorithm,
                   data->hsmkeys[i]->role,
                   data->hsmkeys[i]->is_revoked,
                   data->hsmkeys[i]->key_type,
                   data->hsmkeys[i]->backup);
        }

        printf("%d policies\n",data->npolicies);
        for(int i=0; i<data->npolicies; i++) {
            printf("\npolicy %ld %s description: %s\n",data->policies[i]->id,data->policies[i]->name, data->policies[i]->description);
            printf("  signatures_resign           %u\n",data->policies[i]->signatures_resign);
            printf("  signatures_refresh          %u\n",data->policies[i]->signatures_refresh);
            printf("  signatures_jitter           %u\n",data->policies[i]->signatures_jitter);
            printf("  signatures_inception_offset %u\n",data->policies[i]->signatures_inception_offset);
            printf("  signatures_validity_default %d\n",data->policies[i]->signatures_validity_default);
            printf("  signatures_validity_denial  %d\n",data->policies[i]->signatures_validity_denial);
            printf("  signatures_validity_keyset  %d\n",data->policies[i]->signatures_validity_keyset);
            printf("  signatures_max_zone_ttl     %u\n",data->policies[i]->signatures_max_zone_ttl);
            printf("  %d hsmkeys\n",data->policies[i]->hsmkey_count);
            for(int j=0; j<data->policies[i]->hsmkey_count; j++) {
                printf("    hsmkey %ld repository=%s\n", data->policies[i]->hsmkey[j]->id, data->policies[i]->hsmkey[j]->repository);
            }
            printf("  %d policykeys\n",data->policies[i]->policykey_count);
            for(int j=0; j<data->policies[i]->policykey_count; j++) {
                printf("    policykey %ld role=%u repository=%s algo=%u bits=%u lifetime=%u standby=%u manual=%u rfc5011=%u min=%u\n",
                       data->policies[i]->policykey[j]->id,
                       data->policies[i]->policykey[j]->role, data->policies[i]->policykey[j]->repository,
                       data->policies[i]->policykey[j]->algorithm, data->policies[i]->policykey[j]->bits,
                       data->policies[i]->policykey[j]->lifetime, data->policies[i]->policykey[j]->standby,
                       data->policies[i]->policykey[j]->manual_rollover, data->policies[i]->policykey[j]->rfc5011, data->policies[i]->policykey[j]->minimize);
            }
            
            printf("  %d zones\n",data->policies[i]->zone_count);
            for(int j=0; j<data->policies[i]->zone_count; j++) {
                printf("    zone %ld %s\n", data->policies[i]->zone[j]->id, data->policies[i]->zone[j]->name);
            }
        }
        printf("%d zones\n",data->nzones);
        for(int i = 0; i<data->nzones; i++) {
            printf("zone %ld %s\n",data->zones[i]->id, data->zones[i]->name);
            printf("  policy               %s\n",(data->zones[i]->policy ? data->zones[i]->policy->name : "UNKNOWN"));
            printf("  input adapter type   %s\n",data->zones[i]->input_adapter_type);
            printf("  input adapter uri    %s\n",data->zones[i]->input_adapter_uri);
            printf("  output adapter type  %s\n",data->zones[i]->output_adapter_type);
            printf("  output adapter uri   %s\n",data->zones[i]->output_adapter_uri);
            printf("  signconf path        %s\n",data->zones[i]->signconf_path);
            printf("  signconf dirty       %u\n",data->zones[i]->signconf_needs_writing);
            printf("  next change          %s\n",strtime(data->zones[i]->next_change));
            printf("  next ksk roll        %s\n",strtime(data->zones[i]->next_ksk_roll));
            printf("  next zak roll        %s\n",strtime(data->zones[i]->next_zsk_roll));
            printf("  next csk roll        %s\n",strtime(data->zones[i]->next_csk_roll));
            printf("  ttl end ds           %s\n",strtime(data->zones[i]->ttl_end_ds));
            printf("  ttl end dk           %s\n",strtime(data->zones[i]->ttl_end_dk));
            printf("  ttl end rs           %s\n",strtime(data->zones[i]->ttl_end_rs));
            printf("  forced ksk roll      %u\n",data->zones[i]->roll_ksk_now);
            printf("  forced zak roll      %u\n",data->zones[i]->roll_zsk_now);
            printf("  forced csk roll      %u\n",data->zones[i]->roll_csk_now);
            printf("  %d keys\n",data->zones[i]->key_count);
            for(int j=0; j<data->zones[i]->key_count; j++) {
                printf("    key %ld zone %s role=%u ds_at_parent=%u algorithm=%u inception=%ld introducing=%u revoke=%u standby=%u activezsk=%u activeksk=%u"
                       " publish=%u keytag=%u minimize=%u\n",
                       data->zones[i]->key[j]->id, data->zones[i]->key[j]->zone->name,
                       data->zones[i]->key[j]->role, data->zones[i]->key[j]->ds_at_parent, data->zones[i]->key[j]->algorithm, data->zones[i]->key[j]->inception,
                       data->zones[i]->key[j]->introducing, data->zones[i]->key[j]->should_revoke, data->zones[i]->key[j]->standby,
                       data->zones[i]->key[j]->active_zsk, data->zones[i]->key[j]->active_ksk, data->zones[i]->key[j]->publish, data->zones[i]->key[j]->keytag,
                       data->zones[i]->key[j]->minimize);
                printf("      %d key states\n",data->zones[i]->key[j]->keystate_count);
                printf("      %d key from dependencies\n", data->zones[i]->key[j]->from_keydependency_count);
                printf("      %d key from dependencies\n", data->zones[i]->key[j]->to_keydependency_count);
            }
        }

        dbsimple_commit(session);
    }

    dbsimple_closesession(session);
    dbsimple_closeconnection(connection);

    return 0;
}
