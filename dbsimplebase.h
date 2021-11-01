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

#ifndef DBSIMPLEBASE_H
#define DBSIMPLEBASE_H

#include <stdarg.h>
#include "tree.h"
#include "dbsimple.h"

enum objectstate { OBJUNKNOWN=0, OBJNEW, OBJCLEAN, OBJREMOVED, OBJMODIFIED };

struct backpatch;

struct object {
    struct dbsimple_definition* type;
    char* data;
    int keyid;
    const char* keyname;
    int revision;
    enum objectstate state;
    struct object* next;
    struct backpatch* backpatches;
};

struct dbsimple_connectionbase {
    char* location;
    int nqueries;
    const char* const** queries;
    int ndefinitions;
    struct dbsimple_definition** definitions;
    int (*closeconnection)(dbsimple_connection_type);
    int (*opensession)(dbsimple_connection_type, dbsimple_session_type*);
    int (*closesession)(dbsimple_session_type session);
    int (*syncdata)(dbsimple_session_type, const char* const** query,  void* data);
    void* (*fetchdata)(dbsimple_session_type, const char* const** query, va_list);
    int (*commitdata)(dbsimple_session_type);
    void (*fetchobject)(struct dbsimple_definition*, dbsimple_session_type);
    int (*persistobject)(struct object*, dbsimple_session_type);
};

struct dbsimple_sessionbase {
    tree_type pointermap;
    tree_type objectmap;
    struct object* firstmodified;
    int (*closesession)(dbsimple_session_type session);
    int (*syncdata)(dbsimple_session_type, const char* const** query,  void* data);
    void* (*fetchdata)(dbsimple_session_type, const char* const** query, va_list);
    int (*commitdata)(dbsimple_session_type);
    void (*fetchobject)(struct dbsimple_definition*, dbsimple_session_type);
    int (*persistobject)(struct object*, dbsimple_session_type);
};

struct dbsimple_module {
    const char* identifier;
    int (*openconnection)(char* location, int, const char* const**, dbsimple_connection_type* connectionPtr);
    int (*closeconnection)(dbsimple_connection_type);
    int (*opensession)(dbsimple_connection_type, dbsimple_session_type*);
    int (*closesession)(dbsimple_session_type);
    int (*syncdata)(dbsimple_session_type, const char* const**,  void*);
    void* (*fetchdata)(dbsimple_session_type, const char* const**, va_list);
    int (*commitdata)(dbsimple_session_type);
    void (*fetchobject)(struct dbsimple_definition*, dbsimple_session_type);
    int (*persistobject)(struct object*, dbsimple_session_type);
};

int dbsimple_registermodule(const struct dbsimple_module* bindings);

void* dbsimple__fetch(struct dbsimple_sessionbase* session, int ndefinitions, struct dbsimple_definition** definitions);
void dbsimple__assignreference(struct dbsimple_sessionbase* session, struct dbsimple_field* field, int id, const char* name, struct object* source);
void dbsimple__assignbackreference(struct dbsimple_sessionbase* session, struct dbsimple_definition* def, int id, const char* name, struct dbsimple_field* field, struct object* object);
void dbsimple__commit(struct dbsimple_sessionbase* session);
void dbsimple__committraverse(struct dbsimple_sessionbase* session, struct object* object);
struct object* dbsimple__referencebyptr(struct dbsimple_sessionbase* session, struct dbsimple_definition* def, void* ptr);
struct object* dbsimple__getobject(struct dbsimple_sessionbase* session, struct dbsimple_definition* def, int id, const char* name);

#endif
