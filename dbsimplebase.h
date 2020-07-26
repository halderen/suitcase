enum objectstate { OBJUNKNOWN=0, OBJNEW, OBJCLEAN, OBJREMOVED, OBJMODIFIED };

struct dbsimple_sessionbase {
    tree_type pointermap;
    tree_type objectmap;
    struct object* firstmodified;
};

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

void* dbsimple__fetch(struct dbsimple_sessionbase* session, int ndefinitions, struct dbsimple_definition** definitions);
void dbsimple__assignreference(struct dbsimple_sessionbase* session, struct dbsimple_field* field, int id, const char* name, struct object* source);
void dbsimple__assignbackreference(struct dbsimple_sessionbase* session, struct dbsimple_definition* def, int id, const char* name, struct dbsimple_field* field, struct object* object);
void dbsimple__commitprocedure(struct dbsimple_sessionbase* session);

void dbsimple__committraverse(struct dbsimple_sessionbase* session, struct object* object);
struct object* dbsimple__referencebyptr(struct dbsimple_sessionbase* session, struct dbsimple_definition* def, void* ptr);
struct object* dbsimple__getobject(struct dbsimple_sessionbase* session, struct dbsimple_definition* def, int id, const char* name);

int dbsimple__persistobject(struct object* object, dbsimple_session_type session);
void dbsimple__fetchobject(struct dbsimple_definition* def, dbsimple_session_type session);

