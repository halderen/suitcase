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

struct module_instance {
  char* name;
  struct module_struct* module;
  struct module_instance* next;
};

struct module_instance* instances = NULL;

struct mylibrary_struct {
    struct library_struct* next;
    char* type;
    char* name;
    struct {
        char* interface;
        void* function;
    } interfaces[1];
};
static struct mylibrary_struct library = { NULL, "basic", "suitcase", { { NULL, NULL } } };

__attribute__((constructor))
void
init(void)
{
    library_register((struct library_struct*)&library);
}

__attribute__((destructor))
void
fini(void)
{
}

int
modules(void* ptr)
{
    struct module_instance* instance;
    struct module_instance** parent;
    struct module_struct* definition;

    definition = (struct module_struct*)ptr;
    for(parent=&instances; *parent; parent=&((*parent)->next))
	if((*parent)->module == ptr)
	    break;

    if(*parent) {
	instance = *parent;
	*parent = instance->next;
	free(instance->name);
	free(instance);
    } else {
        instance = malloc(sizeof(struct module_instance));
	if(!instance)
	    return -1;
        instance->name   = strdup(definition->name);
        instance->module = definition;
        instance->next   = NULL;
	*parent = instance;
    }
    return 0;
}

char**
modules_names(void)
{
    int i, count;
    struct module_instance* instance;
    char** names;
    for(count=0,instance=instances; instance; instance=instance->next)
	++count;
    names = malloc(sizeof(char*) * (count+1));
    if(names) {
        for(i=0,instance=instances; i<count; i++) {
	    names[i] = instance->name;
	    instance = instance->next;
        }
	names[i] = NULL;
    }
    return names;
}

int
modules_lookup(char* interface, char** name, const void** ptr)
{
    static char* search = NULL;
    static struct module_instance* instance = NULL;
    int i;

    if (interface) {
        search = interface;
        instance = instances;
    }

    while(instance) {
        for(i=0; instance->module->interfaces[i].interface; i++) {
            if(!strcmp(search, instance->module->interfaces[i].interface)) {
                *name = instance->name;
                *ptr  = instance->module->interfaces[i].function;
                instance = instance->next;
                return 1;
            }
        }
        instance = instance->next;
    }
    name = NULL;
    return 0;
}

void
modules_addlibraries(struct library_struct* library)
{
    while(library) {
        modules(&(library->type));
        library = library->next;
    }
}
