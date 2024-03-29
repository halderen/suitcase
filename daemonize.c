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
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <syslog.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

extern void *argv0;

pid_t
daemonize(char *directory)
{
    int status = 0;
    int fd, i;
    pid_t pid;
    pid = fork();
    if (pid < 0) {
        return -1;
    }
    if (pid > 0) {
        return pid;
    }
    if(setsid() < 0) {
        /* log error */
        status = -1;
    }
    if(directory) {
        if(chdir(directory)) {
            /* log error */
            status = -1;
        }
    }
    for (i = getdtablesize(); i >= 0; --i)
        close(i);
    fd = open("/dev/null", O_RDWR, 0);
    if (fd != -1) {
        dup2(fd, STDIN_FILENO);
        dup2(fd, STDOUT_FILENO);
        dup2(fd, STDERR_FILENO);
        if (fd > 2) {
            close(fd);
        }
    }
    umask(027);
    return status;
}

int
droppriviledges(char* username, char* groupname)
{
    int status = 0;
    struct passwd* userinfo;
    struct group* groupinfo;
    if (username != NULL) {
        userinfo = getpwnam(username);
        if (userinfo) {
            if (setuid(userinfo->pw_uid)) {
                status = -1;
                /* log error */
            }
            if(chdir(userinfo->pw_dir)) {
                /* log warning */
                if(chdir("/")) {
                status = -1;
                 /* log error */
                }
            }
        } else {
                status = -1;
            /* log error */
        }
    }
    if (groupname != NULL) {
        groupinfo = getgrnam(groupname);
        if (groupinfo) {
            if (setgid(groupinfo->gr_gid)) {
                status = -1;
                /* log error */
            }
        } else {
                status = -1;
            /* log error */
        }
    }
    return status;
}
    
pid_t
checkpidfile(const char* pidfile)
{
    int fd;
    int len;
    pid_t pid;
    struct stat statbuf;
    char pidbuf[32];
    char* end;
    if(stat(pidfile, &statbuf)!=0) {
        if(errno != ENOENT) {
            /* unable to stat pidfile */
            return -1;
        } else {
            return 0;
        }
    }
    if (S_ISREG(statbuf.st_mode)) {
        if((fd = open(pidfile, O_RDONLY))>=0) {
            len = read(fd, pidbuf, sizeof (pidbuf));
            close(fd);
            if (len < 0) {
                return -2;
            } else if(len == 0) {
                /* empty pidfile equivalent to no pidfile */
                return 0;
            } else {
                pid = (pid_t)strtol(pidbuf, &end, 10);
                if(*end && *end!='\n') {
                    return -1;
                }
            }
        } else {
            return -2;
        }
    } else {
        return 0;
    }
    if (kill(pid, 0) == 0 || errno == EPERM) {
        // A process with pid is already running.
        return pid;
    } else {
        // pidfile already exists but no process is running pidfile stale
        return 0;
    }
}

int
writepidfile(const char* pidfile, pid_t pid)
{
    int fd;
    int len;
    char pidbuf[32];

    snprintf(pidbuf, sizeof(pidbuf), "%lu\n", (unsigned long) pid);

    if((fd = open(pidfile, O_WRONLY)) >= 0) {
        len = write(fd, pidfile, strlen(pidbuf));
        close(fd);
    } else {
        len = -1;
    }
    if(len != (int)strlen(pidbuf)) {
        return -1;
    } else {
        return 0;
    }
}
