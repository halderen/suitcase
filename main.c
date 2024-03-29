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
#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <dlfcn.h>
#include <getopt.h>
#include "daemonize.h"
#include "utilities.h"
#include "settings.h"
#include "modules.h"
#include "logging.h"
#include "commandline.h"
#include "dbsimple.h"
#include "exampledb.h"
#include "executelua.h"
#include "executetcl.h"

struct library_struct* libraries = NULL;

extern int dbsimple_sqlite3_initialize(void);
extern int dbsimple_dummy_initialize(void);

void
usage(char *argv0)
{
    fprintf(stderr, "usage: %s [ options ]\n", argv0);
}

struct option longopts[] = {
    { "conf", required_argument, NULL, 'c' },
    { NULL,   0, NULL, 0 }
};

int
main(int argc, char* argv[])
{
    pid_t pid;
    int optdaemon = 0;
    int pipefd[2];
    int ch;
    (void)argc;
    (void)argv;
    settings_handle cfghandle = NULL;
    char* cfgfile = PACKAGE_NAME ".conf";

    /* Get the name of the program */
    if((argv0 = strrchr(argv[0],'/')) == NULL)
        argv0 = argv[0];
    else
        ++argv0;
    programsetup(argv[0]);

    /* Enable logging */
    logger_initialize(argv0);

    /* Parse command line options */
    while((ch = getopt_long(argc, argv, "c:", longopts, NULL)) > 0) {
        switch(ch) {
            case 'c':
                cfgfile = optarg;
                break;
            case '?':
            default:
                fprintf(stderr,"%s: Unknown arguments\n",argv0);
                exit(1);
        }
    }

    /* Read configuration file */
    if(cfgfile) {
        if(cfgfile[0]=='/') {
            printf("loading configuration %s\n",cfgfile);
        } else {
            printf("loading configuration %s from %s\n",cfgfile,QUOTE(SYSCONFDIR));
        }
        settings_configure(&cfghandle, QUOTE(SYSCONFDIR), cfgfile, -1);
    }

    /* Steps to act as a daemon process*/
    if (optdaemon) {
        pid = checkpidfile(PACKAGE_NAME ".pid");
        if (pid) {
            fprintf(stderr,"%s already running as process %d\n",argv0,pid);
            exit(1);
        }
        pipe(pipefd);
        pid = daemonize(NULL);
        if (pid<0) {
            exit(1);
        } else if(pid>0) {
            close(pipefd[1]);
            writepidfile(PACKAGE_NAME ".pid", pid);
            read(pipefd[0], &ch, 1);
            if (ch != 0) {
                while(ch>1 && ch<255) {
                    write(2, &ch, 1);
                    read(pipefd[0], &ch, 1);
                }
                close(pipefd[0]);
                exit(1);
            }
            close(pipefd[0]);
            exit(0);
        }
        close(pipefd[0]);
        if (droppriviledges(NULL, NULL)) {
            write(pipefd[1], "\001", 1);
        }
        write(pipefd[1], "", 1);
        close(pipefd[1]);
    }

    /* Load modules */
    int count;
    char* path;
    settings_getcompound(cfghandle, &count, "modules.libraries");
    for(int i=0; i<count; i++) {
        settings_getstringdefault(cfghandle, &path, "", "modules.libraries.%d", i);
        if(dlopen(path, RTLD_LAZY|RTLD_LOCAL|RTLD_DEEPBIND)) {
	    fprintf(stderr, "loaded library %s\n",path);
        } else {
	    fprintf(stderr, "failed to load %s\n",path);
        }
        free(path);
    }

    /* Initialize simple modules */
    char* name;
    int found;
    const void* table;
    for(found=modules_lookup("initialize",&name,&table); found; found=modules_lookup(NULL,&name,&table)) {
        int (*func)(void);
        fprintf(stderr,"initialize %s\n",name);
        func = (int(*)()) functioncast(*(void**)table);
        func();
    }

    /* Non-modules registered libraries */
    for(struct library_struct* library=libraries; library; library=library->next) {
        fprintf(stderr,"library\n");
        fprintf(stderr,"library \"%s\"\n",library->name);
    }
    modules_addlibraries(libraries);

    /* setup database layer */
    dbsimple_initialize();
    dbsimple_dummy_initialize();
#if defined(DO_DYNAMIC_SQLITE3) || defined(HAVE_SQLITE3)
    dbsimple_sqlite3_initialize();
#endif
    char* connectstr;
    settings_getstring(cfghandle, &connectstr, NULL, "storage.datasource");
    if(connectstr && strcmp(connectstr,"")) {
        example_dbsetup(connectstr);
        example_dbtest();
    }
    free(connectstr);
   
    settings_free(cfghandle);
    return 0;
}
