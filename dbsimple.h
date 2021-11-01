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

#ifndef DBSIMPLE_H
#define DBSIMPLE_H

struct dbsimple_connection_struct;
typedef struct dbsimple_connection_struct* dbsimple_connection_type;
struct dbsimple_session_struct;
typedef struct dbsimple_session_struct* dbsimple_session_type;

#define dbsimple_INTEGER          0
#define dbsimple_STRING           1
#define dbsimple_REFERENCE        2
#define dbsimple_BACKREFERENCE    3
#define dbsimple_MASTERREFERENCES 4
#define dbsimple_OPENREFERENCES   5

struct dbsimple_field {
    int type;
    struct dbsimple_definition* def;
    off_t fieldoffset;
    off_t countoffset;
};

#define dbsimple_FLAG_HASREVISION    0x0001
#define dbsimple_FLAG_SINGLETON      0x0002
#define dbsimple_FLAG_EXPLICITREMOVE 0x0004
#define dbsimple_FLAG_AUTOREMOVE     0x0008

struct dbsimple_definition {
    size_t size;
    int flags;
    int nfields;
    struct dbsimple_field* fields;
    union {
        int i;
        void* p;
    } implementation;
};

int dbsimple_initialize(void);
int dbsimple_finalize(void);
int dbsimple_openconnection(char* location,
                            int nqueries, const char* const ** queries,
                            int ndefinitions, struct dbsimple_definition** definitions,
                            dbsimple_connection_type* connectionPtr);
int dbsimple_closeconnection(dbsimple_connection_type);
int dbsimple_opensession(dbsimple_connection_type connection, dbsimple_session_type* sessionPtr);
int dbsimple_closesession(dbsimple_session_type);
int dbsimple_sync(dbsimple_session_type, const char* const** query, void* data);
void* dbsimple_fetch(dbsimple_session_type, const char* const** model, ...);

int dbsimple_commit(dbsimple_session_type);
void dbsimple_dirty(dbsimple_session_type, void *ptr);
void dbsimple_delete(dbsimple_session_type, void *ptr);

#endif
