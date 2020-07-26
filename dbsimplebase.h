#ifndef DBSIMPLEBASE_H
#define DBSIMPLEBASE_H

#include <stdarg.h>

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
};

struct dbsimple_sessionbase {
    tree_type pointermap;
    tree_type objectmap;
    struct object* firstmodified;
    int (*closesession)(dbsimple_session_type session);
    int (*syncdata)(dbsimple_session_type, const char* const** query,  void* data);
    void* (*fetchdata)(dbsimple_session_type, const char* const** query, va_list);
    int (*commitdata)(dbsimple_session_type);
};

enum objectstate { OBJUNKNOWN=0, OBJNEW, OBJCLEAN, OBJREMOVED, OBJMODIFIED };

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

struct dbsimple_module {
    const char* identifier;
    int (*openconnection)(char* location, int, const char* const**, dbsimple_connection_type* connectionPtr);
    int (*closeconnection)(dbsimple_connection_type);
    int (*opensession)(dbsimple_connection_type, dbsimple_session_type*);
    int (*closesession)(dbsimple_session_type);
    int (*syncdata)(dbsimple_session_type, const char* const**,  void*);
    void* (*fetchdata)(dbsimple_session_type, const char* const**, va_list);
    int (*commitdata)(dbsimple_session_type);
};

int dbsimple_registermodule(const struct dbsimple_module* bindings);

int dbsimple__openconnection(char* location,
                             int nqueries, const char* const ** queries,
                             int ndefinitions, struct dbsimple_definition** definitions,
                             dbsimple_connection_type* connectionPtr);
int dbsimple__closeconnection(dbsimple_connection_type);
int dbsimple__opensession(dbsimple_connection_type connection, dbsimple_session_type* sessionPtr);
int dbsimple__closesession(dbsimple_session_type);
int dbsimple__sync(dbsimple_session_type, const char* const** query, void* data);
//void* dbsimple_fetch(dbsimple_session_type, const char* const** model, ...);


void* dbsimple__fetch(struct dbsimple_sessionbase* session, int ndefinitions, struct dbsimple_definition** definitions);
void dbsimple__assignreference(struct dbsimple_sessionbase* session, struct dbsimple_field* field, int id, const char* name, struct object* source);
void dbsimple__assignbackreference(struct dbsimple_sessionbase* session, struct dbsimple_definition* def, int id, const char* name, struct dbsimple_field* field, struct object* object);
void dbsimple__commit(struct dbsimple_sessionbase* session);

void dbsimple__committraverse(struct dbsimple_sessionbase* session, struct object* object);
struct object* dbsimple__referencebyptr(struct dbsimple_sessionbase* session, struct dbsimple_definition* def, void* ptr);
struct object* dbsimple__getobject(struct dbsimple_sessionbase* session, struct dbsimple_definition* def, int id, const char* name);

int dbsimple__persistobject(struct object* object, dbsimple_session_type session);
void dbsimple__fetchobject(struct dbsimple_definition* def, dbsimple_session_type session);

#endif
