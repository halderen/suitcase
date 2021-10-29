#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <unistd.h>
#include "commandline.h"

char* argv0;
char* argv0path;
char* argv0full;

int
programsetup(char* arg)
{
    int pos;
    argv0path = argv0full = NULL;
    for(int i=1; argv0path==NULL; i++) {
        argv0full = realloc(argv0path, i*PATH_MAX);
        if(!argv0full) {
            free(argv0path);
            goto fail;
        }
        argv0path = getcwd(argv0full, i*PATH_MAX);
        if(argv0path == NULL && errno != ERANGE) {
            free(argv0full);
            goto fail;
        }
    }
    if(arg[0] != '/') {
        int arglen = strlen(argv0path)+strlen(arg)+2;
        argv0full = realloc(argv0path, arglen);
        if(!argv0full) {
            free(argv0path);
            goto fail;
        }
        strncat(argv0full,"/",arglen);
        strncat(argv0full,arg,arglen);
    } else {
        int arglen = strlen(arg)+1;
        argv0full = malloc(arglen);
        strncpy(argv0full, arg, arglen);
    }
    pos = (strrchr(argv0full,'/') ? strrchr(argv0full,'/')-argv0full : 0);
    argv0path = malloc(pos + 1);
    if (!argv0path) {
        free(argv0full);
        goto fail;
    }
    strncpy(argv0path, argv0full, pos);
    argv0path[pos] = '\0';
    argv0 = &arg[pos+1];
    return 0;
  fail:
    argv0 = argv0path = argv0full = arg;
    return -1;
}

int
programteardown(void)
{
    if(argv0 == argv0path && argv0 == argv0full)
        return 0;
    free(argv0path);
    free(argv0full);
    return 0;
}
