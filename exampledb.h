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

#ifndef EXAMPLE_H
#define EXAMPLE_H

  struct dbw_data {
    struct dbw_policy** policies;
    int npolicies;
    struct dbw_zone** zones;
    int nzones;
};

#ifndef OPENDNSSEC_CONFIG_FILE
struct dbw_policy {
    long id;
    int policykey_count;
    struct dbw_policykey **policykey;
    int hsmkey_count;
    struct dbw_hsmkey **hsmkey;
    int zone_count;
    struct dbw_zone **zone;

    char *name;
    char *description;
    char* denial_salt;
    unsigned int passthrough;
    unsigned int signatures_resign;
    unsigned int signatures_refresh;
    unsigned int signatures_jitter;
    unsigned int signatures_inception_offset;
    unsigned int signatures_validity_default;
    unsigned int signatures_validity_denial;
    unsigned int signatures_validity_keyset;
    unsigned int signatures_max_zone_ttl;
    unsigned int denial_type;
    unsigned int denial_optout;
    unsigned int denial_ttl;
    unsigned int denial_resalt;
    unsigned int denial_algorithm;
    unsigned int denial_iterations;
    unsigned int denial_salt_length;
    unsigned int denial_salt_last_change;
    unsigned int keys_ttl;
    unsigned int keys_retire_safety;
    unsigned int keys_publish_safety;
    unsigned int keys_shared;
    unsigned int keys_purge_after;
    unsigned int zone_propagation_delay;
    unsigned int zone_soa_ttl;
    unsigned int zone_soa_minimum;
    unsigned int zone_soa_serial;
    unsigned int parent_registration_delay;
    unsigned int parent_propagation_delay;
    unsigned int parent_ds_ttl;
    unsigned int parent_soa_ttl;
    unsigned int parent_soa_minimum;
};

struct dbw_policykey {
    long id;
    struct dbw_policy *policy;
    char* repository;
    unsigned int role;
    unsigned int algorithm;
    unsigned int bits;
    unsigned int lifetime;
    unsigned int standby;
    unsigned int manual_rollover;
    unsigned int rfc5011;
    unsigned int minimize;
};

struct dbw_hsmkey {
    long id;
    int key_count;
    struct dbw_key **key;

    char *locator;
    char *repository;
    unsigned int state;
    unsigned int bits;
    unsigned int algorithm;
    unsigned int role;
    time_t inception;
    unsigned int is_revoked;
    unsigned int key_type;
    unsigned int backup;
    // FIXME no policyId or backreference used
};

struct dbw_zone {
    long id;
    struct dbw_policy *policy; /** Only valid when joined */
    int key_count;
    struct dbw_key **key;

    char *name;
    time_t next_change;
    char *signconf_path;
    char *input_adapter_uri;
    char *input_adapter_type;
    char *output_adapter_uri;
    char *output_adapter_type;
    time_t next_ksk_roll;
    time_t next_zsk_roll;
    time_t next_csk_roll;
    unsigned int signconf_needs_writing;
    time_t ttl_end_ds;
    time_t ttl_end_dk;
    time_t ttl_end_rs;
    unsigned int roll_ksk_now;
    unsigned int roll_zsk_now;
    unsigned int roll_csk_now;
    int scratch;
};

struct dbw_keystate {
    long id;
    struct dbw_key *key;
    unsigned int type; // should this be the enum
    unsigned int state;
    time_t last_change;
    unsigned int minimize;
    unsigned int ttl;
};

struct dbw_keydependency {
    int id;
    struct dbw_key *fromkey; /** Only valid when joined */
    struct dbw_key *tokey; /** Only valid when joined */
    unsigned int type;
};

struct dbw_key {
    long id;
    struct dbw_zone *zone; /** Only valid when joined */
    struct dbw_hsmkey *hsmkey; /** Only valid when joined */
    int keystate_count;
    struct dbw_keystate **keystate;
    int from_keydependency_count;
    struct dbw_keydependency **from_keydependency;
    int to_keydependency_count;
    struct dbw_keydependency **to_keydependency;

    unsigned int role;
    unsigned int ds_at_parent;
    unsigned int algorithm;
    time_t inception;
    unsigned int introducing;
    unsigned int should_revoke;
    unsigned int standby;
    unsigned int active_zsk;
    unsigned int active_ksk;
    unsigned int publish;
    unsigned int keytag;
    unsigned int minimize;
};

extern int example_dbsetup(char* connectstr);
extern int example_dbtest(void);
#endif

#endif
