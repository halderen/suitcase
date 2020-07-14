#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdint.h>
#include <string.h>
#include "daemonize.h"
#include "utilities.h"
#include "settings.h"

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
    int optdaemon = 0;
    int pipefd[2];
    int ch;
    (void)argc;
    (void)argv;
    settings_handle cfghandle = NULL;

    /* Get the name of the program */
    if((argv0 = strrchr(argv[0],'/')) == NULL)
        argv0 = argv[0];
    else
        ++argv0;

    settings_configure(&cfghandle, QUOTE(SYSCONFDIR), PACKAGE_NAME ".conf", -1);
    settings_access(NULL, 0, NULL);

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

    /* do stuff */
    exit(0);
}
