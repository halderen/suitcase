#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include <dlfcn.h>
#include "utilities.h"
#include "modules.h"

static void* modulesymbol;
static void* modulessymbol;

static functioncast_t
functioncastx(void*generic) {
    functioncast_t* function = (functioncast_t*)&generic;
    return *function;
}

__attribute__((constructor))
void
init(void)
{    
    int (*modules)(void*);
    modulesymbol = dlsym(NULL, "module");
    modulessymbol = dlsym(NULL, "modules");
    modules = (int (*)(void*))functioncastx(modulessymbol);

    modules(&module);
}

__attribute__((destructor))
void
fini(void)
{
    int (*modules)(void*);
    modules = (int (*)(void*)) functioncastx(modulessymbol);
    modules(&module);
}
