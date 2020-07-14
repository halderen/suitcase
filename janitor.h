#ifndef JANITOR_H
#define JANITOR_H

#include <pthread.h>

struct janitor_thread_struct;
typedef struct janitor_thread_struct* janitor_thread_t;

typedef void (*janitor_runfn_t)(void *);

typedef void (*janitor_alertfn_t)(const char *format, ...)
#ifdef HAVE___ATTRIBUTE__
     __attribute__ ((format (printf, 1, 2)))
#endif
     ;

extern void janitor_initialize(janitor_alertfn_t fatalalertfn, janitor_alertfn_t problemalertfn);

struct janitor_threadclass_struct;
typedef struct janitor_threadclass_struct* janitor_threadclass_t;
#define janitor_threadclass_DEFAULT (NULL)

extern int janitor_threadclass_create(janitor_threadclass_t* threadclassptr, const char* name);
extern char* janitor_threadclass_name(janitor_threadclass_t threadclass);
extern void janitor_threadclass_destroy(janitor_threadclass_t threadclass);
extern void janitor_threadclass_setdetached(janitor_threadclass_t threadclass);
extern void janitor_threadclass_setautorun(janitor_threadclass_t threadclass);
extern void janitor_threadclass_setblockedsignals(janitor_threadclass_t threadclass);
extern void janitor_threadclass_setminstacksize(janitor_threadclass_t threadclass, size_t minstacksize);

extern int janitor_thread_create(janitor_thread_t* thread, janitor_threadclass_t threadclass, janitor_runfn_t func, void*data);
extern void janitor_thread_start(janitor_thread_t thread);
extern int janitor_thread_join(janitor_thread_t thread);
extern int janitor_thread_tryjoinall(janitor_threadclass_t threadclass);
extern void janitor_thread_joinall(janitor_threadclass_t threadclass);

extern int janitor_disablecoredump(void);
extern int janitor_trapsignals(char* argv0);

extern void janitor_backtrace(void);
extern char* janitor_backtrace_string(void);
extern void janitor_backtrace_all(void);

extern void janitor_thread_signal(janitor_thread_t thread);

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
