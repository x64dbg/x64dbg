/*
Copyright (c) 2013. The YARA Authors. All Rights Reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its contributors
may be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef YR_ATOMS_H
#define YR_ATOMS_H

#include "limits.h"
#include "re.h"

#define ATOM_TREE_LEAF  1
#define ATOM_TREE_AND   2
#define ATOM_TREE_OR    3


typedef struct _ATOM_TREE_NODE
{
    uint8_t type;
    uint8_t atom_length;
    uint8_t atom[MAX_ATOM_LENGTH];

    uint8_t* forward_code;
    uint8_t* backward_code;

    RE_NODE* recent_nodes[MAX_ATOM_LENGTH];

    struct _ATOM_TREE_NODE* children_head;
    struct _ATOM_TREE_NODE* children_tail;
    struct _ATOM_TREE_NODE* next_sibling;

} ATOM_TREE_NODE;


typedef struct _ATOM_TREE
{
    ATOM_TREE_NODE* current_leaf;
    ATOM_TREE_NODE* root_node;

} ATOM_TREE;


typedef struct _YR_ATOM_LIST_ITEM
{
    uint8_t atom_length;
    uint8_t atom[MAX_ATOM_LENGTH];

    uint16_t backtrack;

    uint8_t* forward_code;
    uint8_t* backward_code;

    struct _YR_ATOM_LIST_ITEM* next;

} YR_ATOM_LIST_ITEM;


int yr_atoms_extract_from_re(
    RE_AST* re_ast,
    int flags,
    YR_ATOM_LIST_ITEM** atoms);


int yr_atoms_extract_from_string(
    uint8_t* string,
    int string_length,
    int flags,
    YR_ATOM_LIST_ITEM** atoms);


int yr_atoms_min_quality(
    YR_ATOM_LIST_ITEM* atom_list);


void yr_atoms_list_destroy(
    YR_ATOM_LIST_ITEM* list_head);

#endif
