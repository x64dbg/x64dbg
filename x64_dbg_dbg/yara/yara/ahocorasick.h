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


int yr_ac_create_automaton(
    YR_ARENA* arena,
    YR_AC_AUTOMATON** automaton);


int yr_ac_add_string(
    YR_ARENA* arena,
    YR_AC_AUTOMATON* automaton,
    YR_STRING* string,
    YR_ATOM_LIST_ITEM* atom);


YR_AC_STATE* yr_ac_next_state(
    YR_AC_STATE* state,
    uint8_t input);


int yr_ac_create_failure_links(
    YR_ARENA* arena,
    YR_AC_AUTOMATON* automaton);


void yr_ac_print_automaton(
    YR_AC_AUTOMATON* automaton);

#endif
