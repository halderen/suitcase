#define _GNU_SOURCE
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <yaml.h>
#include "logging.h"
#include "settings.h"
#include "utilities.h"

static logger_cls_type logger = LOGGER_INITIALIZE(__FILE__);

static settings_handle defaulthandle = NULL;

typedef int (*namedtranslatetype)(const char*,long*resultvalue);

static yaml_node_t*
getnodebyname(yaml_document_t *document, yaml_node_t *node, const char* arg, size_t len)
{
    yaml_node_pair_t* nodepair;
    yaml_node_t* child;
    if (node && node->type == YAML_MAPPING_NODE) {
        for (nodepair = node->data.mapping.pairs.start; nodepair < node->data.mapping.pairs.top; nodepair++) {
            child = yaml_document_get_node(document, nodepair->key);
            if (child && child->type == YAML_SCALAR_NODE) {
                if (child->data.scalar.length == len && !strncmp(arg, (char*) child->data.scalar.value, child->data.scalar.length)) {
                    return yaml_document_get_node(document, nodepair->value);
                }
            }
        }
    }
    return NULL;
}

static yaml_node_t*
getnodebyindex(yaml_document_t *document, yaml_node_t *node, int index)
{
    yaml_node_item_t* nodeitem;
    yaml_node_t* child;
    if (node && node->type == YAML_SEQUENCE_NODE) {
        for (nodeitem = node->data.sequence.items.start; nodeitem < node->data.sequence.items.top; nodeitem++) {
            if(index == 0) {
            child = yaml_document_get_node(document, *nodeitem);
                return child;
            } else {
                --index;
            }
        }
    }
    return NULL;
}

static yaml_node_t*
parselocate(yaml_document_t *document, yaml_node_t *node, const char* fmt, va_list ap, const char** lastp)
{
    int len;
    char* arg;
    yaml_node_t* child;
    if (fmt == NULL) {
        do {
            arg = va_arg(ap, char*);
            if (arg != NULL) {
                child = getnodebyname(document, node, arg, strlen(arg));
                if(!child) {
                    *lastp = arg;
                    return NULL;
                } else
                    node = child;
            }
        } while (arg != NULL);
    } else {
        while (*fmt) {
            if (!strncmp(fmt, "%s", 2)) {
                arg = va_arg(ap, char*);
                child = getnodebyname(document, node, arg, strlen(arg));
                if (!child) {
                    *lastp = arg;
                    return NULL;
                } else
                    node = child;
                fmt += 2;
            } else if (!strncmp(fmt, "%d", 2)) {
                len = va_arg(ap, int);
                child = getnodebyindex(document, node, len);
                if (!child) {
                    return NULL;
                } else
                    node = child;
                fmt += 2;
            } else {
                for (len=0; fmt[len]; len++)
                    if (fmt[len] == '.')
                        break;
                child = getnodebyname(document, node, fmt, len);
                if (!child) {
                    return NULL;
                } else
                    node = child;
                fmt += len;
            }
            if(*fmt == '.')
                ++fmt;
        }
    }
    return node;
}

static int
parsefunclong(void* user, const char* str, void* resultvalue)
{
    char* end;
    long* resultlong = (long*) resultvalue;
    (void)user;
    errno = 0;
    *resultlong = strtol(str, &end, 0);
    if (errno) {
        return 1;
    } else {
        return 0;
    }
}

static int
parsefuncstring(void* user, const char* str, void* resultvalue)
{
    char** resultstring = (char**) resultvalue;
    (void)user;
    errno = 0;
    if (str) {
        *resultstring = strdup(str);
        return 0;
    } else
        return 1;
}

struct parsefuncenum {
    const char** enumstrings;
    const int* enumvalues;
};

static int
parsefuncenum(void* user, const char* str, void* resultvalue)
{
    int i;
    char* end;
    int* resultint = (int*) resultvalue;
    struct parsefuncenum* enums = (struct parsefuncenum*)user;
    errno = 0;
    *resultint = strtol(str, &end, 0);
    if ((errno || end == str) && enums) {
        for(i=0; enums->enumstrings[i] != NULL; i++) {
            if (!strcasecmp(enums->enumstrings[i], str)) {
                if(enums->enumvalues) {
                    *resultint = enums->enumvalues[i];
                } else {
                    *resultint = i;
                }
                return 0;
            }
        }
        return 1;
    } else
        return 0;
}

static int
parsefunccount(void* user, const char* str, void* resultvalue)
{
    char* end;
    long* resultlong = (long*) resultvalue;
    (void)user;
    errno = 0;
    if (*str == '#') {
        ++str;
        while(isspace(*str))
            ++str;
    }
    *resultlong = strtol(str, &end, 0);
    if (errno || str==end) {
        return 1;
    } else {
        return 0;
    }
}

static int
parsefuncnamed(void* user, const char* str, void* resultvalue)
{
    char* end;
    const char* last;
    long* resultlong = (long*) resultvalue;
    namedtranslatetype translate = (namedtranslatetype) functioncast(user);
    errno = 0;
    while(isspace(*str))
      ++str;
    if (*str == '#') {
        ++str;
        while(isspace(*str))
            ++str;
        *resultlong = strtol(str, &end, 0);
        if (errno || str==end) {
            return 1;
        } else {
            return 0;
        }
    }
    for (last=str; *last && !isspace(*last); ++last)
        ;
    if (*last) {
        str = strndupa(str, last-str);
    }
    return translate(str, resultvalue);
}


static int
parsescalar(yaml_document_t *document, size_t resultsize, void* resultvalue, const void* defaultvalue, int (*parsefunc)(void*,const char*,void*), void* parsedata, const char* fmt, va_list ap)
{
    size_t len;
    const char* last = "unknown";
    const char* str;
    int result;
    yaml_node_t* root;
    yaml_node_t* node;
    root = (document ? yaml_document_get_root_node(document) : NULL);
    node = parselocate(document, root, fmt, ap, &last);
    if (node) {
        if (node->type == YAML_SCALAR_NODE) {
            str = (const char*)node->data.scalar.value;
            for (len = 0; len < node->data.scalar.length && isspace(str[len]); len++)
                ;
            str = &str[len];
            len = node->data.scalar.length - len;
            while (len > 0 && isspace(str[len]))
                --len;
            str = strndup(str, len);
            if (parsefunc(parsedata,str,resultvalue)) {
                if (defaultvalue) {
                    memcpy(resultvalue,defaultvalue,resultsize);
                }
                logger_message(&logger, logger_ctx, logger_WARN, "In configuration parameter %s unable unparseable input %s\n",last,str);
                result = -1;
            } else {
                result = 0;
            }
            free((void*)str);
        } else {
            logger_message(&logger, logger_ctx, logger_WARN, "In configuration parameter %s unable to parse argument\n",last);
            if (defaultvalue) {
                memcpy(resultvalue,defaultvalue,resultsize);
            }
            result = -1;
        }
    } else {
        if (defaultvalue) {
            memcpy(resultvalue,defaultvalue,resultsize);
            result = 0;
        } else {
            logger_message(&logger, logger_ctx, logger_WARN, "In configuration parameter %s argument not found\n",last);
            result = 1;
        }
    }
    return result;
}

static int
parsecompound(yaml_document_t *document, int* resultvalue, const char* fmt, va_list ap)
{
    int count;
    const char* last = "unknown";
    yaml_node_t* root;
    yaml_node_t* node;
    yaml_node_item_t* nodeitem;
    root = (document ? yaml_document_get_root_node(document) : NULL);
    node = parselocate(document, root, fmt, ap, &last);
    if (node) {
        count = 0;
        if (node->type == YAML_SEQUENCE_NODE) {
            for(nodeitem = node->data.sequence.items.start; nodeitem<node->data.sequence.items.top; nodeitem++)
                ++count;
        }
    } else {
        count = -1;
    }
    *resultvalue = count;
    return 0;
}

int
settings_access(settings_handle* handleptr, int basefd, const char* filename)
{
    int fd, returncode;
    FILE *input = NULL;
    yaml_parser_t parser;
    yaml_document_t* document;
    if(handleptr == NULL) {
        if(defaulthandle) {
            yaml_document_delete((yaml_document_t*)defaulthandle);
            free((void*)defaulthandle);
            defaulthandle = NULL;
        }
        handleptr = &defaulthandle;
    } else if(filename == NULL && *handleptr == NULL) {
        yaml_document_delete((yaml_document_t*)*handleptr);
    }
    
    yaml_parser_initialize(&parser);
    fd = openat(basefd, filename, O_RDONLY);
    if(fd >= 0)
        input = fdopen(fd, "r");
    if(fd >= 0 && input) {
        yaml_parser_set_input_file(&parser, input);
        document = malloc(sizeof(yaml_document_t));
        yaml_parser_load(&parser, document);
        yaml_parser_delete(&parser);
        fclose(input); /* this will also close the file descriptor */
        returncode = 0;
    } else {
        document = NULL;
        returncode = -1;
    }
    *handleptr = document;
    return returncode;
}

int
settings_getstring(settings_handle handle, char** resultvalue, const char* defaultvalue, const char* fmt, ...)
{
    int rc;
    va_list ap;
    yaml_document_t* document = (handle ? handle : defaulthandle);
    va_start(ap, fmt);
    rc = parsescalar(document, sizeof(char*), resultvalue, NULL, parsefuncstring, NULL, fmt, ap);
    if(*resultvalue == NULL && defaultvalue)
        *resultvalue = strdup(defaultvalue);
    va_end(ap);
    return rc;
}

int
settings_getlong(settings_handle handle, long* resultvalue, const long* defaultvalue, const char* fmt, ...)
{
    int rc;
    va_list ap;
    yaml_document_t* document = (handle ? handle : defaulthandle);
    va_start(ap, fmt);
    rc = parsescalar(document, sizeof(long), resultvalue, defaultvalue, parsefunclong, NULL, fmt, ap);
    va_end(ap);
    return rc;
}

int
settings_getenum(settings_handle handle, int* resultvalue, const int* defaultvalue, const char** enums, const char* fmt, ...)
{
    int rc;
    va_list ap;
    yaml_document_t* document = (handle ? handle : defaulthandle);
    struct parsefuncenum enummapping;
    enummapping.enumstrings = enums;
    enummapping.enumvalues = NULL;
    va_start(ap, fmt);
    rc = parsescalar(document, sizeof(long), resultvalue, defaultvalue, parsefuncenum, &enummapping, fmt, ap);
    va_end(ap);
    return rc;
}

int
settings_getenum2(settings_handle handle, int* resultvalue, const int* defaultvalue, const char** enumstrings, const int* enumvalues, const char* fmt, ...)
{
    int rc;
    va_list ap;
    yaml_document_t* document = (handle ? handle : defaulthandle);
    struct parsefuncenum enummapping;
    enummapping.enumstrings = enumstrings;
    enummapping.enumvalues = enumvalues;
    va_start(ap, fmt);
    rc = parsescalar(document, sizeof(long), resultvalue, defaultvalue, parsefuncenum, &enummapping, fmt, ap);
    va_end(ap);
    return rc;
}

int
settings_getcompound(settings_handle handle, int* resultvalue, const char* fmt, ...)
{
    int rc;
    va_list ap;
    yaml_document_t* document = (handle ? handle : defaulthandle);
    va_start(ap, fmt);
    rc = parsecompound(document, resultvalue, fmt, ap);
    va_end(ap);
    return rc;
}

int
settings_getcount(settings_handle handle, long* resultvalue, const long* defaultvalue, const char* fmt, ...)
{
    int rc;
    va_list ap;
    yaml_document_t* document = (handle ? handle : defaulthandle);
    va_start(ap, fmt);
    rc = parsescalar(document, sizeof(long), resultvalue, defaultvalue, parsefunccount, NULL, fmt, ap);
    va_end(ap);
    return rc;
}

int
settings_getnamed(settings_handle handle, long* resultvalue, const long* defaultvalue, int (*translate)(const char*,long*resultvalue), const char* fmt, ...)
{
    int rc;
    va_list ap;
    yaml_document_t* document = (handle ? handle : defaulthandle);
    va_start(ap, fmt);
    rc = parsescalar(document, sizeof(long), resultvalue, defaultvalue, parsefuncnamed, translate, fmt, ap);
    va_end(ap);
    return rc;
}

static const char* loggerstrings[] = { "fatal", "error", "alert", "warn", "warning", "info", "informational", "notice", "debug", "verbose", "diag", "diagnostic", "trace", "tracing", NULL };
static const int loggervalues[] = { logger_FATAL, logger_ERROR, logger_ERROR, logger_WARN, logger_WARN, logger_INFO, logger_INFO, logger_INFO, logger_DEBUG, logger_DEBUG, logger_DIAG, logger_DIAG, logger_DIAG, logger_DIAG };
static const char* loggertargets[] = { "default", "stdout", "stderr", "syslog", NULL };

int
settings_configure(settings_handle* cfghandleptr, char* sysconfdir, char* sysconffile, int cmdline_verbosity)
{
    int verbosity, count;
    char* name;
    int defaultverbosity = 1;
    int target = -1;
    int defaulttarget = -1;
    logger_procedure targetproc = logger_log_stderr;
    settings_handle cfghandle;

    int basefd = open(sysconfdir, O_DIRECTORY);
    if(basefd < 0)
        basefd = AT_FDCWD;
    
    if (!settings_access(cfghandleptr, basefd, sysconffile)) {

        /* parse generic processing */
        cfghandle = (cfghandleptr ? *cfghandleptr : NULL);
        verbosity = -1;
        if(cmdline_verbosity <= 0) {
            if(!settings_getenum2(cfghandle, &verbosity, NULL, loggerstrings, loggervalues, NULL, "logging", "verbosity", NULL)) {
                logger_configurecls(NULL, verbosity, targetproc);
            } else {
                logger_configurecls(NULL, logger_ERROR, targetproc);
            }
        } else {
            switch(cmdline_verbosity) {
                case 1:
                    logger_configurecls(NULL, logger_WARN, targetproc);
                    break;
                case 2:
                    logger_configurecls(NULL, logger_INFO, targetproc);
                    break;
                case 3:
                    logger_configurecls(NULL, logger_DEBUG, targetproc);
                    break;
                case 4:
                default:
                    logger_configurecls(NULL, logger_DIAG, targetproc);
                    break;
            }
        }

        /* parse per class logging */
        settings_getcompound(cfghandle, &count, "logging.classes");
        for(int i=0; i<count; i++) {
            settings_getstring(cfghandle, &name, NULL, "logging.classes.%d.name", i);
            settings_getenum2(cfghandle, &verbosity, &defaultverbosity, loggerstrings, loggervalues, "logging.classes.%d.verbosity", i);
            settings_getenum(cfghandle, &target, &defaulttarget, loggertargets, "logging.classes.%d.target", i);
            switch (target) {
                case 1:
                    targetproc = logger_log_stdout;
                    break;
                case 2:
                    targetproc = logger_log_stderr;
                    break;
                case 3:
                    targetproc = logger_log_syslog;
                    break;
                case 0:
                default:
                    targetproc = logger_log_syslog;
                    break;
            }
            logger_configurecls(name, verbosity, targetproc);
        }
    }
    close(basefd);
    return 0;
}
