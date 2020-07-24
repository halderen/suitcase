#ifndef DBSIMPLE_H
#define DBSIMPLE_H

struct dbsimple_implementation;
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
    struct dbsimple_implementation* implementation;
};

int dbsimple_initialize(char* hint);
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
