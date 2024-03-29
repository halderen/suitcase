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

#ifndef JANITOR_H
#define JANITOR_H

#include <pthread.h>

struct janitor_thread_struct;
typedef struct janitor_thread_struct* janitor_thread_type;

typedef void (*janitor_runfn_type)(void *);

typedef void (*janitor_alertfn_type)(const char *format, ...)
#ifdef HAVE___ATTRIBUTE__
     __attribute__ ((format (printf, 1, 2)))
#endif
     ;

extern void janitor_initialize(janitor_alertfn_type fatalalertfn, janitor_alertfn_type problemalertfn);

struct janitor_threadclass_struct;
typedef struct janitor_threadclass_struct* janitor_threadclass_type;
#define janitor_threadclass_DEFAULT (NULL)

extern int janitor_threadclass_create(janitor_threadclass_type* threadclassptr, const char* name);
extern char* janitor_threadclass_name(janitor_threadclass_type threadclass);
extern void janitor_threadclass_destroy(janitor_threadclass_type threadclass);
extern void janitor_threadclass_setdetached(janitor_threadclass_type threadclass);
extern void janitor_threadclass_setautorun(janitor_threadclass_type threadclass);
extern void janitor_threadclass_setblockedsignals(janitor_threadclass_type threadclass);
extern void janitor_threadclass_setminstacksize(janitor_threadclass_type threadclass, size_t minstacksize);

extern int janitor_thread_create(janitor_thread_type* thread, janitor_threadclass_type threadclass, janitor_runfn_type func, void*data);
extern void janitor_thread_start(janitor_thread_type thread);
extern int janitor_thread_join(janitor_thread_type thread);
extern int janitor_thread_tryjoinall(janitor_threadclass_type threadclass);
extern void janitor_thread_joinall(janitor_threadclass_type threadclass);

extern int janitor_disablecoredump(void);
extern int janitor_trapsignals(char* argv0);

extern void janitor_backtrace(void);
extern char* janitor_backtrace_string(void);
extern void janitor_backtrace_all(void);

extern void janitor_thread_signal(janitor_thread_type thread);

/* in case of missing pthread barrier calls */
extern int janitor_pthread_barrier_init(pthread_barrier_t* barrier, const pthread_barrierattr_t* attr, unsigned int count);
extern int janitor_pthread_barrier_wait(pthread_barrier_t* barrier);
extern int janitor_pthread_barrier_destroy(pthread_barrier_t* barrier);
#ifndef HAVE_PTHREAD_BARRIER
# ifdef pthread_barrier_init
#  undef pthread_barrier_init
# endif
# define pthread_barrier_init janitor_pthread_barrier_init
# ifdef pthread_barrier_destroy
#  undef pthread_barrier_destroy
# endif
#  define pthread_barrier_destroy janitor_pthread_barrier_destroy
# ifdef pthread_barrier_wait
#  undef pthread_barrier_wait
# endif
# define pthread_barrier_wait janitor_pthread_barrier_wait
# ifndef PTHREAD_BARRIER_SERIAL_THREAD
#  define PTHREAD_BARRIER_SERIAL_THREAD 1
# endif
#endif

#endif
