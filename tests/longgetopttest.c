#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "longgetopt.h"

struct option options[] = {
    { "version", no_argument,       NULL, 0x80|'v' },
    { "verbose", optional_argument, NULL, 'v' },
    { "config",  required_argument, NULL, 'c' },
    { "help",    no_argument,       NULL, 'h' },
    { NULL,      no_argument,       NULL, '\0' },
};

int
main(int argc, char* argv[])
{
    int value;
    int ch;
    struct longgetopt longgetoptctx;
    --argc;
    ++argv;
    for(longgetopt(argc, argv, "v::c:h", options, &value, &longgetoptctx); (ch = longgetopt(argc, argv, NULL, NULL, &value, &longgetoptctx)) >= 0; ) {
        switch(ch) {
            case 0x80|'v':
                printf("version\n");
                break;
            case 'h':
                printf("help\n");
                break;
            case 'v':
                if(longgetoptctx.optarg)
                    printf("set verbosity \"%s\"\n",longgetoptctx.optarg);
                else
                    printf("increment verbosity\n");
                break;
            case 'c':
            case ':':
                printf("bad or missing argument to option %s\n",argv[longgetoptctx.optind]);
                return 1;
            case '?':
            default:
                printf("unknown option %d:%d %s\n",longgetoptctx.optind,longgetoptctx._optpos,argv[longgetoptctx.optind]);
                return 1;
        }
    }
    printf("arguments %d %d:",longgetoptctx.optind,argc);
    for(int i=longgetoptctx.optind; i<argc; i++) {
        printf(" \"%s\"",argv[i]);
    }
    printf("\n");
    return 0;
}
