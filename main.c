#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include "daemonize.h"
#include "settings.h"
#include "utilities.h"

char *argv0;

void
usage(char *argv0)
{
    fprintf(stderr, "usage: %s [ options ]\n", argv0);
}

int
main(int argc, char* argv[])
{
    pid_t pid;
    (void)argc;
    (void)argv;
    settings_handle cfghandle = NULL;

    /* Get the name of the program */
    if((argv0 = strrchr(argv[0],'/')) == NULL)
        argv0 = argv[0];
    else
        ++argv0;

    settings_configure(&cfghandle, QUOTE(SYSCONFDIR), PACKAGE_NAME ".conf", -1);
    pid = checkpidfile(PACKAGE_NAME ".pid");
    settings_access(NULL, 0, NULL);
    pid = daemonize(NULL);
    if (pid<0) {
        exit(1);
    } else if(pid>0) {
        writepidfile(PACKAGE_NAME ".pid", pid);
        exit(0);
    }
    droppriviledges(NULL, NULL);
    /* do stuff */
    exit(0);
}
