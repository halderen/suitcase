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
#include "utilities.h"
#define settings__INTERNAL
typedef yaml_document_t* document_type;
typedef yaml_node_t* node_type;
#include "settings.h"

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

node_type
settings__parselocate_yaml(document_type document, node_type node, const char* fmt, va_list ap, const char** lastp)
{
    int len;
    char* arg;
    yaml_node_t* child;
    if(node == NULL) {
        node = yaml_document_get_root_node(document);
    }
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
                if(fmt[0] == '@')
                    ++fmt;
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

char*
settings__getscalar_yaml(node_type node)
{
    const char* str;
    size_t len;
    if(node->type == YAML_SCALAR_NODE) {
        str = (const char*)node->data.scalar.value;
        for(len = 0; len < node->data.scalar.length && isspace(str[len]); len++)
            ;
        str = &str[len];
        len = node->data.scalar.length-len;
        while(len>0&&isspace(str[len]))
            --len;
        return strndup(str, len);
    } else {
        return NULL;
    }
}

int
settings__nodecount_yaml(node_type node)
{
    yaml_node_item_t* nodeitem;
    yaml_node_pair_t* nodepair;
    int count = 0;
    if(node->type == YAML_SEQUENCE_NODE) {
        for(nodeitem = node->data.sequence.items.start; nodeitem<node->data.sequence.items.top; nodeitem++)
            ++count;
    } else if(node->type == YAML_MAPPING_NODE) {
        for(nodepair = node->data.mapping.pairs.start; nodepair<node->data.mapping.pairs.top; nodepair++)
            ++count;
    }
    return count;
}

document_type
settings__access_yaml(document_type olddocument, int fd)
{
    yaml_parser_t parser;
    yaml_document_t* document;
    FILE* file;
    if(olddocument) {
        yaml_document_delete(olddocument);
        free((void*)olddocument);
    }
    file = fdopen(dup(fd), "r");
    if (file == NULL) {
	document = NULL;
	return NULL;
    }
    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, file);
    document = malloc(sizeof (yaml_document_t));
    if(!yaml_parser_load(&parser, document)) {
        yaml_document_delete(document);
        document = NULL;
    }
    yaml_parser_delete(&parser);
    fclose(file);
    return document;
}
