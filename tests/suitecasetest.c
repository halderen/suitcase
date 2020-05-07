#define _GNU_SOURCE

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>

#include <CUnit/Basic.h>
#include <CUnit/Automated.h>

#include "utilities.h"

char* argv0;
static char* workdir;
static janitor_threadclass_t debugthreadclass;
static janitor_thread_t debugthread;

static void
initialize(int argc, char* argv[])
{
    /* this initialization should happen only once */
    if (argv[0][0] != '/') {
        char *path = getcwd(NULL, 0);
        asprintf(&argv0, "%s/%s", path, argv[0]);
        free(path);
    } else {
        argv0 = strdup(argv[0]);
    }

    if (argc > 1) {
        workdir = argv[1];
    }

    ods_janitor_initialize(argv0);
    // logger_initialize(argv0);

    janitor_threadclass_create(&debugthreadclass, "debug");
    janitor_threadclass_setautorun(debugthreadclass);
    janitor_threadclass_setblockedsignals(debugthreadclass);
}

static void
usefile(const char* basename, const char* specific)
{
    struct timespec curtime;
    struct timespec newtime[2];
    struct stat filestat;
    int basefd = AT_FDCWD;

    unlinkat(basefd, basename, 0);
    if (specific != NULL) {
        linkat(basefd, specific, basefd, basename, 0);
        fstatat(basefd, basename, &filestat, 0);
        clock_gettime(CLOCK_REALTIME_COARSE, &curtime);
        newtime[0] = filestat.st_atim;
        newtime[1] = curtime;
        utimensat(basefd, basename, newtime, 0);
    }
}

static void
setUp(void)
{
    int linkfd, status;

    ods_log_init(argv0, 0, NULL, 3);
    // logger_initialize(argv0);

    if (workdir != NULL)
        chdir(workdir);

    unlink("suitecase.pid");

}

static void
tearDown(void)
{
    //janitor_thread_join(debugthread);

    unlink("enforcer.pid");
}

static void
finalize(void)
{
    janitor_threadclass_destroy(debugthreadclass);
    ods_log_close();
    free(argv0);
}

void
testNothing(void)
{
}

extern void testNothing(void);

struct test_struct {
    const char* suite;
    const char* name;
    const char* description;
    CU_TestFunc pTestFunc;
    CU_pSuite pSuite;
    CU_pTest pTest;
} tests[] = {
    { "enforcer", "testNothing",         "test nothing" },
    { NULL, NULL, NULL }
};

int
main(int argc, char* argv[])
{
    int i, j, status = 0;
    CU_pSuite pSuite = NULL;

    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    for(i=0; tests[i].name; i++) {
        for(j=0; j<i; j++)
                break;
        if(j<i) {
            tests[i].pSuite = tests[j].pSuite;
        } else {
            tests[i].pSuite = CU_add_suite_with_setup_and_teardown(tests[i].suite, NULL, NULL, setUp, tearDown);
        }
    }
    for(i=0; tests[i].name; i++) {
        tests[i].pTestFunc = dlsym(NULL, (tests[i].name[0]=='-' ? &tests[i].name[1] : tests[i].name));
        if(tests[i].name[0]!='-') {
            if(tests[i].pTestFunc != NULL) {
                tests[i].pTest = CU_add_test(tests[i].pSuite, tests[i].description, tests[i].pTestFunc);
                if(!tests[i].pTest) {
                    CU_cleanup_registry();
                    return CU_get_error();
                }
            } else {
                fprintf(stderr,"%s: unable to register test %s.%s\n",argv0,tests[i].suite,tests[i].name);
            }
        } else {
            tests[i].name = &(tests[i].name[1]);
            tests[i].pTest = NULL;
        }
    }

    initialize(argc, argv);
    if(argc > 1) {
        --argc;
        ++argv;
    }

    CU_list_tests_to_file();
    if (argc == 2 && !strcmp(argv[1],"-")) {
        for(i=0; tests[i].name; i++) {
            if(tests[i].name != NULL) {
                printf("TEST %s\n",tests[i].name);
                if(tests[i].pTest == NULL)
                    tests[i].pTest = CU_add_test(tests[i].pSuite, tests[i].description, tests[i].pTestFunc);
                CU_basic_run_test(tests[i].pSuite, tests[i].pTest);
            }
        }
    } else if (argc > 1) {
        for(i=1; i<argc; i++) {
            printf("TEST %s\n",argv[i]);
            for(j=0; tests[j].name; j++) {
                if(!strcmp(argv[i],tests[j].suite))
                    break;
            }
            if(tests[j].name == NULL) {
                for(j=0; tests[j].name; j++) {
                    if(!strcmp(argv[i],tests[j].name))
                        break;
                }
            }
            if(tests[j].name == NULL) {
                for(j=0; tests[j].name; j++) {
                    if(!strncmp(argv[i],tests[j].suite,strlen(tests[j].suite)) &&
                       argv[i][strlen(tests[j].suite)]=='.' &&
                       !strcmp(&argv[i][strlen(tests[j].suite)+1],tests[j].name))
                        break;
                }
            }
            if(tests[j].name != NULL) {
                if(tests[j].pTest == NULL)
                    tests[j].pTest = CU_add_test(tests[j].pSuite, tests[j].description, tests[j].pTestFunc);
                CU_basic_run_test(tests[j].pSuite, tests[j].pTest);
            } else {
                fprintf(stderr,"%s: test %s not found\n",argv0,argv[i]);
                status = 1;
            }
        }
    } else {
        CU_automated_run_tests();
    }
    if (CU_get_number_of_tests_failed() != 0)
        status = 1;
    CU_cleanup_registry();

    finalize();
    return status;
}
