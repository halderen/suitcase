#ifndef EXAMPLE_H
#define EXAMPLE_H

struct data;
struct policy;
struct zone;

struct data {
    int npolicies;
    struct policy** policies;
    int nzones;
    struct zone** zones;
};

struct policy {
    char* name;
    struct zone** zones;
    int nzones;
};

struct zone {
    char* name;
    struct policy* policy;
    struct zone* parent;
    struct zone** subzones;
    int nsubzones;
};

extern int example_dbsetup(char* connectstr);

#endif
