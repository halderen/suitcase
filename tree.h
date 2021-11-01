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
#include <stdlib.h>
#include <stdint.h>

#ifndef TREE_H
#define TREE_H

#ifndef tree_DEFAULT_MAXDEPTH
#define tree_DEFAULT_MAXDEPTH 40
#endif

struct tree;
struct tree_node;

/**
 * Tree reference structure.
 * Used to keep track of certain location in the tree.
 * Members of this structure should not be referenced or manipulated
 * directly.  The structure definition is public in order to perform
 * the static initialization of TREE_REFERNCE_INITIALIZER
 */
struct tree_reference {
    struct tree* tree;
    int depth;
    int maxdepth;
    struct tree_node* path[tree_DEFAULT_MAXDEPTH];
};

#define TREE_REFERENCE_INITIALIZER { NULL, 0, tree_DEFAULT_MAXDEPTH, { 0 } }

typedef struct tree* tree_type;
typedef struct tree_node* tree_cursor_type;
typedef struct tree_reference tree_reference_type;
typedef int (*tree_comparator_type)(const void* a, const void* b, void* user_data);
typedef int (*tree_visitor_type)(void* item, void* user);

tree_type tree_create(tree_comparator_type compare_func, void* compare_cargo);
void tree_destroy(tree_type tree);
int  tree_size(tree_type tree);
void tree_foreach(tree_type tree, tree_visitor_type func, void* user);
void  tree_traverse(tree_type tree, tree_visitor_type func, int order, void* user);
void* tree_remove(tree_type tree, const void* item);
void* tree_replace(tree_type tree, void* item);
int tree_insert(tree_type tree, void* item);
void* tree_insertref(tree_type tree, void* item, tree_reference_type* cursor);
void* tree_lookup(tree_type tree, const void* item);
void* tree_lookupref(tree_type tree, const void* item, tree_reference_type* cursor);
void* tree_lookupcursor(tree_type tree, const void* item, tree_cursor_type* node);
void* tree_lookupleftcursor(tree_type tree, const void* item, tree_cursor_type* node);
void* tree_lookuprightcursor(tree_type tree, const void* item, tree_cursor_type* node);
void* tree_lookupneighbours(struct tree* tree, const void* item, struct tree_reference* refptr, struct tree_node** left, struct tree_node** right);
void* tree_cursorremove(tree_reference_type* ref);
void* tree_refreplace(tree_reference_type* ref, void* item);
void* tree_lookupfirst(tree_type tree);
void* tree_lookupfirstcursor(tree_type tree, tree_cursor_type* node);
void* tree_cursornext(tree_cursor_type* node);

#endif
