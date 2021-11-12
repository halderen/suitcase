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
const char* const** sqlstmts_default;


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
    { dbsimple_MASTERREFERENCES, &dbw_policydefinition, offsetof(struct dbw_data, policies), offsetof(struct dbw_data, npolicies) },
    //{ dbsimple_MASTERREFERENCES, &dbw_policykeydefinition, offsetof(struct dbw_data, policykeys), offsetof(struct dbw_data, npolicykeys) },
    { dbsimple_MASTERREFERENCES, &dbw_zonedefinition, offsetof(struct dbw_data, zones), offsetof(struct dbw_data, nzones) },
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
    { dbsimple_BACKREFERENCE,  &dbw_datadefinition,      offsetof(struct dbw_data, policies), offsetof(struct dbw_data, npolicies) },
    { dbsimple_OPENREFERENCES, &dbw_policykeydefinition, offsetof(struct dbw_policy, policykey),  -1 },
    { dbsimple_OPENREFERENCES, &dbw_hsmkeydefinition,    offsetof(struct dbw_policy, hsmkey),  -1 },
    { dbsimple_OPENREFERENCES, &dbw_zonedefinition,      offsetof(struct dbw_policy, zone),  offsetof(struct dbw_policy, zone_count) },
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
    { dbsimple_OPENREFERENCES, &dbw_hsmkeydefinition,       offsetof(struct dbw_hsmkey, key),                -1 },
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
};

struct dbsimple_field dbw_zonefields[] = {
    { dbsimple_LONGINT,        &dbw_zonedefinition,       offsetof(struct dbw_zone, id),                     -1 },
    { dbsimple_INT,            NULL,                     -1, -1 }, // [rev]
    { dbsimple_REFERENCE,      &dbw_policydefinition,     offsetof(struct dbw_zone, policy),                 -1 },
    { dbsimple_BACKREFERENCE,  &dbw_policydefinition,     offsetof(struct dbw_policy, zone),                 offsetof(struct dbw_policy, zone_count) },
    { dbsimple_OPENREFERENCES, &dbw_zonedefinition,       offsetof(struct dbw_zone, key),                    -1 },
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
    { dbsimple_BACKREFERENCE,  &dbw_datadefinition,       offsetof(struct dbw_data,   zones), offsetof(struct dbw_data, nzones) },
};

struct dbsimple_field dbw_keyfields[] = {
    { dbsimple_LONGINT,        &dbw_keydefinition,           offsetof(struct dbw_key, id),                     -1 },
    { dbsimple_INT,            NULL, -1, -1 }, // [rev]
    { dbsimple_REFERENCE,      &dbw_zonedefinition,          offsetof(struct dbw_key, zone),                -1 },
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

struct dbsimple_definition dbw_datadefinition =          { sizeof(struct dbw_data),          dbsimple_FLAG_SINGLETON,   sizeof(dbw_datafields)/sizeof(struct dbsimple_field),          dbw_datafields,   { 0 } };
struct dbsimple_definition dbw_policydefinition =        { sizeof(struct dbw_policy),        dbsimple_FLAG_HASREVISION, sizeof(dbw_policyfields)/sizeof(struct dbsimple_field),        dbw_policyfields, { 0 } };
struct dbsimple_definition dbw_policykeydefinition =     { sizeof(struct dbw_policykey),     dbsimple_FLAG_HASREVISION, sizeof(dbw_policykeyfields)/sizeof(struct dbsimple_field),     dbw_policykeyfields, { 0 } };
struct dbsimple_definition dbw_hsmkeydefinition =        { sizeof(struct dbw_hsmkey),        dbsimple_FLAG_HASREVISION, sizeof(dbw_hsmkeyfields)/sizeof(struct dbsimple_field),        dbw_hsmkeyfields, { 0 } };
struct dbsimple_definition dbw_zonedefinition =          { sizeof(struct dbw_zone),          dbsimple_FLAG_HASREVISION, sizeof(dbw_zonefields)/sizeof(struct dbsimple_field),          dbw_zonefields,     { 0 } };
struct dbsimple_definition dbw_keydefinition =           { sizeof(struct dbw_key),           dbsimple_FLAG_HASREVISION, sizeof(dbw_keyfields)/sizeof(struct dbsimple_field),           dbw_keyfields,     { 0 } };
struct dbsimple_definition dbw_keystatedefinition =      { sizeof(struct dbw_keystate),      dbsimple_FLAG_HASREVISION, sizeof(dbw_keystatefields)/sizeof(struct dbsimple_field),      dbw_keystatefields,     { 0 } };
struct dbsimple_definition dbw_keydependencydefinition = { sizeof(struct dbw_keydependency), 0,                         sizeof(dbw_keydependencyfields)/sizeof(struct dbsimple_field), dbw_keydependencyfields,     { 0 } };

static struct dbsimple_definition* dbw_definitions[] = {
    &dbw_datadefinition, &dbw_policydefinition, &dbw_policykeydefinition, &dbw_hsmkeydefinition, &dbw_zonedefinition, &dbw_keydefinition, &dbw_keystatedefinition, &dbw_keydependencydefinition
};

int
example_dbsetup(char* location)
{
    int rcode;
    int exists;
    
    if(!strncasecmp(location,"sqlite3:",strlen("sqlite3:")) || !strcasecmp(location,"sqlite3")) {
        rcode = dbsimple_openconnection(location,
                                   sizeof(sqlstmts_sqlite3_array)/sizeof(const char*) - 1 , sqlstmts_sqlite3_array,
                                   sizeof(dbw_definitions)/sizeof(struct dbsimple_definition*), dbw_definitions, &connection);
        if(rcode) {
            return -1;
        }
        sqlstmts_qschema  = sqlstmts_sqlite3_qschema;
        sqlstmts_schema   = sqlstmts_sqlite3_schema;
        sqlstmts_qschema1 = sqlstmts_sqlite3_qschema1;
        sqlstmts_schema1  = sqlstmts_sqlite3_schema1;
        sqlstmts_default  = sqlstmts_sqlite3_default;
    } else if(!strncasecmp(location,"mysql:",strlen("mysql:"))) {
        rcode = dbsimple_openconnection(location,
                                   sizeof(sqlstmts_mysql_array)/sizeof(const char*) - 1 , sqlstmts_mysql_array,
                                   sizeof(dbw_definitions)/sizeof(struct dbsimple_definition*), dbw_definitions, &connection);
        if(rcode) {
            return -1;
        }
        sqlstmts_qschema  = sqlstmts_mysql_qschema;
        sqlstmts_schema   = sqlstmts_mysql_schema;
        sqlstmts_qschema1 = sqlstmts_mysql_qschema1;
        sqlstmts_schema1  = sqlstmts_mysql_schema1;
        sqlstmts_default  = sqlstmts_mysql_default;
    }

    dbsimple_opensession(connection, &session);

    exists = 0;
    dbsimple_sync(session, sqlstmts_qschema, &exists);
    if(exists == 0) {
        fprintf(stderr,"initializing schema\n");
        rcode = dbsimple_sync(session, sqlstmts_schema, &exists);
        if(rcode)
            fprintf(stderr,"initializing schema failed\n");
        dbsimple_closesession(session);
        dbsimple_opensession(connection, &session);
    } else {
        fprintf(stderr,"checking schema\n");
        exists = 0;
        dbsimple_sync(session, sqlstmts_qschema1, &exists);
        if(exists == 0) {
            fprintf(stderr,"updating schema[1]\n");
            rcode = dbsimple_sync(session, sqlstmts_schema1, &exists);
            if(rcode)
                fprintf(stderr,"updating schema[1] failed\n");
            dbsimple_closesession(session);
            dbsimple_opensession(connection, &session);
        }
    }
    fprintf(stderr,"schema complete\n");
    return 0;
}


int
example_dbtest(void)
{
    struct dbw_data* data;
    dbsimple_opensession(connection, &session);

    data = dbsimple_fetch(session, sqlstmts_default);
    if(data) {
        printf("%d policies\n",data->npolicies);
        for(int i=0; i<data->npolicies; i++) {
            printf("policy %ld %s description: %s\n",data->policies[i]->id,data->policies[i]->name, data->policies[i]->description);
            printf("  signatures_resign           %d\n",data->policies[i]->signatures_resign);
            printf("  signatures_refresh          %d\n",data->policies[i]->signatures_refresh);
            printf("  signatures_jitter           %d\n",data->policies[i]->signatures_jitter);
            printf("  signatures_inception_offset %d\n",data->policies[i]->signatures_inception_offset);
            printf("  signatures_validity_default %d\n",data->policies[i]->signatures_validity_default);
            printf("  signatures_validity_denial  %d\n",data->policies[i]->signatures_validity_denial);
            printf("  signatures_validity_keyset  %d\n",data->policies[i]->signatures_validity_keyset);
            printf("  signatures_max_zone_ttl     %d\n",data->policies[i]->signatures_max_zone_ttl);
            printf("  %d hsmkeys\n",data->policies[i]->hsmkey_count);
            for(int j=0; j<data->policies[i]->hsmkey_count; j++) {
                printf("    hsmkey %ld repository=%s\n", data->policies[i]->hsmkey[j]->id, data->policies[i]->hsmkey[j]->repository);
            }
            printf("  %d policykeys\n",data->policies[i]->policykey_count);
            for(int j=0; j<data->policies[i]->policykey_count; j++) {
                printf("    policykey %ld role=%d repository=%s algo=%u bits=%u lifetime=%u standby=%u manual=%u rfc5011=%u min=%u\n",
                       data->policies[i]->policykey[j]->id,
                       data->policies[i]->policykey[j]->role, data->policies[i]->policykey[j]->repository,
                       data->policies[i]->policykey[j]->algorithm, data->policies[i]->policykey[j]->bits,
                       data->policies[i]->policykey[j]->lifetime, data->policies[i]->policykey[j]->standby,
                       data->policies[i]->policykey[j]->manual_rollover, data->policies[i]->policykey[j]->rfc5011, data->policies[i]->policykey[j]->minimize);
            }
            printf("  policykeys %d:\n",data->policies[i]->policykey_count);
            
            printf("  %d zones\n",data->policies[i]->zone_count);
            for(int j=0; j<data->policies[i]->zone_count; j++) {
                printf("    zone %ld %s\n", data->policies[i]->zone[j]->id, data->policies[i]->zone[j]->name);
            }
        }
        printf("%d zones\n",data->nzones);
        for(int i = 0; i<data->nzones; i++) {
            printf("zone %ld %s\n",data->zones[i]->id, data->zones[i]->name);
        }
        dbsimple_commit(session);
    }

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
