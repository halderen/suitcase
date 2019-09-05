#ifndef DAEMONIZE_H
#define DAEMONIZE_H

#include <sys/types.h>
#include <unistd.h>

extern char *argv0;

pid_t daemonize(char *directory);

int droppriviledges(char *username, char* groupname);

pid_t checkpidfile(const char* pidfile);

int writepidfile(const char* pidfile, pid_t pid);

#endif
