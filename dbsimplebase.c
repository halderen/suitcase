#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "utilities.h"
#include "tree.h"
#include "dbsimple.h"
#include "dbsimplebase.h"

struct backpatch {
    struct object* source;
    struct dbsimple_field* field;
    struct backpatch* next;
};

static int
pointermapcompare(const void* a, const void* b, __attribute__((unused)) void* user)
{
    struct object* o1 = (struct object*) a;
    struct object* o2 = (struct object*) b;
    return o1->data - o2->data;
}

static int
objectmapcompare(const void* a, const void* b, __attribute__((unused)) void* user)
{
    int result;
    struct object* o1 = (struct object*) a;
    struct object* o2 = (struct object*) b;
    result = o1->type - o2->type;
    if(result == 0) {
        if(o1->keyname)
            if(o2->keyname)
                result = strcmp(o1->keyname, o2->keyname);
            else
                result = -1;
        else
            if(o2->keyname)
                result = 1;
            else
                result = o1->keyid - o2->keyid;
    }
    return result;
}

static int
deleteobject(struct object* object, __attribute__((unused)) dbsimple_session_type session)
{
    void*** refarray;
    if(object->data) {
        for(int i=0; i<object->type->nfields; i++) {
            switch (object->type->fields[i].type) {
                case dbsimple_INTEGER:
                case dbsimple_STRING:
                case dbsimple_REFERENCE:
                case dbsimple_BACKREFERENCE:
                    break;
                case dbsimple_MASTERREFERENCES:
                case dbsimple_OPENREFERENCES:
                    refarray = (void***) &(object->data[object->type->fields[i].fieldoffset]);
                    free(*refarray);
                    break;
                default:
                    abort();
            }
        }
        free(object->data);
    }
    return 0; /* continue */
}

struct object*
dbsimple__referencebyptr(struct dbsimple_sessionbase* session, struct dbsimple_definition* def, void* ptr)
{
    struct object lookup;
    struct object* object;
    lookup.data = ptr;
    object = tree_lookup(session->pointermap, &lookup);
    if(object == NULL) {
        object = malloc(sizeof (struct object));
        object->type = def;
        object->data = ptr;
        object->state = OBJNEW;
        object->revision = 0;
        object->next = NULL;
        object->backpatches = NULL;
        tree_insert(session->pointermap, object);
    }
    return object;
}

void
dbsimple__committraverse(struct dbsimple_sessionbase* session, struct object* object)
{
    void* targetptr;
    struct object* targetobj;
    int* refarraycount;
    void*** refarray;
    if(object->next != NULL)
        return;
    switch(object->state) {
        case OBJUNKNOWN:
            abort();
        case OBJNEW:
        case OBJMODIFIED:
        case OBJREMOVED:
            object->next = (session->firstmodified ? session->firstmodified : object);
            session->firstmodified = object;
            break;
        case OBJCLEAN:
            object->next = object;
            break;
    }
    for(int i=0; i<object->type->nfields; i++) {
        switch(object->type->fields[i].type) {
            case dbsimple_INTEGER:
            case dbsimple_STRING:
                break;
            case dbsimple_REFERENCE:
                targetptr = *(void**)&(object->data[object->type->fields[i].fieldoffset]);
                if(targetptr) {
                    targetobj = dbsimple__referencebyptr(session, object->type->fields[i].def, targetptr);
                    if(targetobj->type == NULL)
                        targetobj->type = object->type->fields[i].def;
                    dbsimple__committraverse(session, targetobj);
                }
                break;
            case dbsimple_BACKREFERENCE:
                break;
            case dbsimple_MASTERREFERENCES:
                refarraycount = (int*)&(object->data[object->type->fields[i].countoffset]);
                refarray = (void***)&(object->data[object->type->fields[i].fieldoffset]);
                for(int j=0; j<*refarraycount; j++) {
                    targetptr = (*refarray)[j];
                    if(targetptr) {
                        targetobj = dbsimple__referencebyptr(session, object->type->fields[i].def, targetptr);
                        if(targetobj->type == NULL)
                            targetobj->type = object->type->fields[i].def;
                        dbsimple__committraverse(session, targetobj);
                    }
                }
                break;
            case dbsimple_OPENREFERENCES:
                break;
            default:
                abort();
        }
    }
}

struct object*
dbsimple__getobject(struct dbsimple_sessionbase* session, struct dbsimple_definition* def, int id, const char* name)
{
    tree_reference_type cursor;
    struct object lookup;
    struct object* object;
    lookup.type = def;
    lookup.keyid = id;
    lookup.keyname = name;
    object = tree_lookupref(session->objectmap, &lookup, &cursor);
    if (object == NULL) {
        object = malloc(sizeof (struct object));
        object->state = OBJUNKNOWN;
        object->type = def;
        object->data = calloc(1, def->size);
        object->keyid = id;
        object->keyname = NULL;
        object->revision = -1;
        object->next = NULL;
        object->backpatches = NULL;
        tree_insertref(session->objectmap, object, &cursor);
        tree_insert(session->pointermap, object);
    }
    return object;
}

static void
subscribereference(struct dbsimple_field* field, struct object* source, struct object* subject)
{
    struct backpatch* patch = malloc(sizeof(struct backpatch));
    patch->source = source;
    patch->field = field;
    patch->next = source->backpatches;
    subject->backpatches = patch;
}

void
dbsimple__assignreference(struct dbsimple_sessionbase* session, struct dbsimple_field* field, int id, const char* name, struct object* source)
{
    struct object* targetoject;
    void** destination = (void**)&(source->data[field->fieldoffset]);
    targetoject = dbsimple__getobject(session, field->def, id, name);
    if(targetoject->data) {
        *destination = targetoject->data;
    } else {
        subscribereference(field, source, targetoject);
    }
}

void
dbsimple__assignbackreference(struct dbsimple_sessionbase* session, struct dbsimple_definition* def, int id, const char* name, struct dbsimple_field* field, struct object* object)
{
    struct object* targetobject;
    char* target;
    int* refarraycount;
    void*** refarray;
    targetobject = dbsimple__getobject(session, def, id, name);
    target = targetobject->data;
    if(target) {
        refarraycount = (int*)&(target[field->countoffset]);
        refarray = (void***)&(target[field->fieldoffset]);
        *refarray = realloc(*refarray, sizeof (void*) * (1+ *refarraycount));
        (*refarray)[*refarraycount] = object->data;
        *refarraycount += 1;
    } else {
        subscribereference(field, object, targetobject);
    }
}

static int
resolvebackpatches(struct object* object, __attribute__((unused)) void* user)
{
    char* target;
    int* refarraycount;
    void*** refarray;
    while(object->backpatches) {
        struct backpatch* patch = object->backpatches;
        switch(patch->field->type) {
            case dbsimple_REFERENCE:
                target = patch->source->data;
                *(void**)&(target[patch->field->fieldoffset]) = object->data;
                break;
            case dbsimple_BACKREFERENCE:
                target = object->data;
                refarraycount = (int*)&(target[patch->field->countoffset]);
                refarray = (void***)&(target[patch->field->fieldoffset]);
                *refarray = realloc(*refarray, sizeof (void*) * (1+ *refarraycount));
                (*refarray)[*refarraycount] = object->data;
                *refarraycount += 1;
                break;
            default:
                abort();
        }
        object->backpatches = patch->next;
        free(patch);
    }
    return 0; // continue foreach items
}

static int
commitobject(struct object* object, struct dbsimple_sessionbase* session)
{
    if((object->type->flags & dbsimple_FLAG_SINGLETON) == dbsimple_FLAG_SINGLETON)
        return 0;

    if(object->next == NULL) {
        /* this object isn't reachable */
        switch(object->state) {
            case OBJMODIFIED:
                if((object->type->flags & dbsimple_FLAG_EXPLICITREMOVE) != dbsimple_FLAG_EXPLICITREMOVE)
                    object->state = OBJREMOVED;
                break;
            case OBJCLEAN:
                if(object->type->flags & dbsimple_FLAG_AUTOREMOVE)
                    object->state = OBJREMOVED;
                break;
            case OBJREMOVED:
                break;
            default:
                abort();
        }
    }
    return session->persistobject(object, (dbsimple_session_type)session);
}

void
dbsimple__commit(struct dbsimple_sessionbase* session)
{
    tree_foreach(session->pointermap, (tree_visitor_type) commitobject, (void*) session);
    tree_foreach(session->objectmap, (tree_visitor_type) deleteobject, (void*) session);
    tree_destroy(session->objectmap);
    tree_destroy(session->pointermap);
}

void
dbsimple_delete(dbsimple_session_type session, void *ptr)
{
    struct object lookup;
    struct object* object;
    assert(ptr != NULL);
    lookup.data = ptr;
    object = tree_lookup(((struct dbsimple_sessionbase*)session)->pointermap, &lookup);
    if(object != NULL) {
        object->state = OBJREMOVED;
    }
}

void
dbsimple_dirty(dbsimple_session_type session, void *ptr)
{
    tree_reference_type cursor;
    struct object lookup;
    struct object* object;
    assert(ptr != NULL);
    lookup.data = ptr;
    object = tree_lookupref(((struct dbsimple_sessionbase*)session)->pointermap, &lookup, &cursor);
    if(object == NULL) {
        object = malloc(sizeof (struct object));
        object->type = NULL;
        object->state = OBJNEW;
        object->data = ptr;
        object->keyid = -1;
        object->keyname = NULL;
        object->revision = 0;
        object->next = NULL;
        object->backpatches = NULL;
        tree_insert(((struct dbsimple_sessionbase*)session)->pointermap, object);
    } else {
        object->state = OBJMODIFIED;
    }
}

void*
dbsimple__fetch(struct dbsimple_sessionbase* session, int ndefinitions, struct dbsimple_definition** definitions)
{
    struct object* object = NULL;

    session->pointermap = tree_create(pointermapcompare, NULL);
    session->objectmap = tree_create(objectmapcompare, NULL);
    session->firstmodified = NULL;
    for(int i=0; i<ndefinitions; i++) {
        if((definitions[i]->flags & dbsimple_FLAG_SINGLETON) == dbsimple_FLAG_SINGLETON) {
            object = dbsimple__getobject(session, definitions[i], 0, NULL);
            object->state = OBJCLEAN;
        } else {
            session->fetchobject(definitions[i], (dbsimple_session_type)session);
        }
    }
    tree_foreach(session->objectmap, (tree_visitor_type) resolvebackpatches, NULL);
    return object->data;
}
