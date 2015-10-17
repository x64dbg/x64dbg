/*
Copyright (c) 2014. The YARA Authors. All Rights Reserved.

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

#ifndef YR_OBJECT_H
#define YR_OBJECT_H

#ifdef _MSC_VER
#include <float.h>
#define isnan _isnan
//#define INFINITY (DBL_MAX + DBL_MAX)
//#define NAN (INFINITY-INFINITY)
#endif

#include "types.h"


#define OBJECT_CREATE           1

#define OBJECT_TYPE_INTEGER     1
#define OBJECT_TYPE_STRING      2
#define OBJECT_TYPE_STRUCTURE   3
#define OBJECT_TYPE_ARRAY       4
#define OBJECT_TYPE_FUNCTION    5
#define OBJECT_TYPE_REGEXP      6
#define OBJECT_TYPE_DICTIONARY  7
#define OBJECT_TYPE_FLOAT       8


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


void yr_object_print_data(
    YR_OBJECT* object,
    int indent);


#endif
