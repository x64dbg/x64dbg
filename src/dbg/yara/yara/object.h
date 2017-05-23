/*
Copyright (c) 2014. The YARA Authors. All Rights Reserved.

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

#ifndef YR_OBJECT_H
#define YR_OBJECT_H

#ifdef _MSC_VER

#include <float.h>
#ifndef isnan
#define isnan _isnan
#endif

#ifndef INFINITY
#define INFINITY (DBL_MAX + DBL_MAX)
#endif

#ifndef NAN
#define NAN (INFINITY-INFINITY)
#endif

#endif

#include "types.h"


#define OBJECT_CREATE           1

#define OBJECT_TYPE_INTEGER     1
#define OBJECT_TYPE_STRING      2
#define OBJECT_TYPE_STRUCTURE   3
#define OBJECT_TYPE_ARRAY       4
#define OBJECT_TYPE_FUNCTION    5
#define OBJECT_TYPE_DICTIONARY  6
#define OBJECT_TYPE_FLOAT       7


int yr_object_create(
    int8_t type,
    const char* identifier,
    YR_OBJECT* parent,
    YR_OBJECT** object);


int yr_object_function_create(
    const char* identifier,
    const char* arguments_fmt,
    const char* return_fmt,
    YR_MODULE_FUNC func,
    YR_OBJECT* parent,
    YR_OBJECT** function);


int yr_object_from_external_variable(
    YR_EXTERNAL_VARIABLE* external,
    YR_OBJECT** object);


void yr_object_destroy(
    YR_OBJECT* object);


int yr_object_copy(
    YR_OBJECT* object,
    YR_OBJECT** object_copy);


YR_OBJECT* yr_object_lookup_field(
    YR_OBJECT* object,
    const char* field_name);


YR_OBJECT* yr_object_lookup(
    YR_OBJECT* root,
    int flags,
    const char* pattern,
    ...);


int yr_object_has_undefined_value(
    YR_OBJECT* object,
    const char* field,
    ...);

int64_t yr_object_get_integer(
    YR_OBJECT* object,
    const char* field,
    ...);


SIZED_STRING* yr_object_get_string(
    YR_OBJECT* object,
    const char* field,
    ...);


int yr_object_set_integer(
    int64_t value,
    YR_OBJECT* object,
    const char* field,
    ...);


int yr_object_set_float(
    double value,
    YR_OBJECT* object,
    const char* field,
    ...);


int yr_object_set_string(
    const char* value,
    size_t len,
    YR_OBJECT* object,
    const char* field,
    ...);


YR_OBJECT* yr_object_array_get_item(
    YR_OBJECT* object,
    int flags,
    int index);


int yr_object_array_set_item(
    YR_OBJECT* object,
    YR_OBJECT* item,
    int index);


YR_OBJECT* yr_object_dict_get_item(
    YR_OBJECT* object,
    int flags,
    const char* key);


int yr_object_dict_set_item(
    YR_OBJECT* object,
    YR_OBJECT* item,
    const char* key);


int yr_object_structure_set_member(
    YR_OBJECT* object,
    YR_OBJECT* member);


YR_OBJECT* yr_object_get_root(
    YR_OBJECT* object);


YR_API void yr_object_print_data(
    YR_OBJECT* object,
    int indent,
    int print_identifier);


#endif
