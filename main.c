#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include "daemonize.h"
#include "settings.h"
#include "utilities.h"

int
main(int argc, char* argv[])
{
    pid_t pid;
    (void)argc;
    (void)argv;
    settings_handle cfghandle = NULL;

    settings_configure(&cfghandle, QUOTE(SYSCONFDIR), PACKAGE_NAME ".conf", -1);
    pid = checkpidfile(PACKAGE_NAME ".pid");
    settings_access(NULL, 0, NULL);
    pid = daemonize(NULL);
    if(pid<0) {
    } else if(pid>0) {
        writepidfile(PACKAGE_NAME ".pid", pid);
        exit(0);
    }
    droppriviledges(NULL, NULL);
    /* do stuff */
    exit(0);
}
