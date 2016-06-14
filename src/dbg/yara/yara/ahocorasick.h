/*
Copyright (c) 2013. The YARA Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef _AHOCORASICK_H
#define _AHOCORASICK_H

#include "limits.h"
#include "atoms.h"
#include "types.h"


#define YR_AC_ROOT_STATE                0
#define YR_AC_NEXT_STATE(t)             (t >> 32)
#define YR_AC_INVALID_TRANSITION(t, c)  (((t) & 0xFFFF) != c)

#define YR_AC_MAKE_TRANSITION(state, code, flags) \
  ((uint64_t)((((uint64_t) state) << 32) | ((flags) << 16) | (code)))

#define YR_AC_USED_FLAG    0x1

#define YR_AC_USED_TRANSITION_SLOT(x)   ((x) & (YR_AC_USED_FLAG << 16))
#define YR_AC_UNUSED_TRANSITION_SLOT(x) (!YR_AC_USED_TRANSITION_SLOT(x))


typedef struct _YR_AC_TABLES
{
    YR_AC_TRANSITION* transitions;
    YR_AC_MATCH_TABLE_ENTRY* matches;

} YR_AC_TABLES;


int yr_ac_automaton_create(
    YR_AC_AUTOMATON** automaton);


int yr_ac_automaton_destroy(
    YR_AC_AUTOMATON* automaton);


int yr_ac_add_string(
    YR_AC_AUTOMATON* automaton,
    YR_STRING* string,
    YR_ATOM_LIST_ITEM* atom,
    YR_ARENA* matches_arena);


int yr_ac_compile(
    YR_AC_AUTOMATON* automaton,
    YR_ARENA* arena,
    YR_AC_TABLES* tables);


void yr_ac_print_automaton(
    YR_AC_AUTOMATON* automaton);


#endif
