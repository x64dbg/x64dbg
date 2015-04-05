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

#ifndef YR_TYPES_H
#define YR_TYPES_H


#include "arena.h"
#include "re.h"
#include "limits.h"
#include "hash.h"

#ifdef _WIN32
#include <windows.h>
typedef HANDLE mutex_t;
#else
#include <pthread.h>
typedef pthread_mutex_t mutex_t;
#endif

typedef int32_t tidx_mask_t;


#define DECLARE_REFERENCE(type, name) \
    union { type name; int64_t name##_; }

#pragma pack(push)
#pragma pack(1)


#define NAMESPACE_TFLAGS_UNSATISFIED_GLOBAL      0x01

#define NAMESPACE_HAS_UNSATISFIED_GLOBAL(x) \
    ((x)->t_flags[yr_get_tidx()] & NAMESPACE_TFLAGS_UNSATISFIED_GLOBAL)


typedef struct _YR_NAMESPACE
{
    int32_t t_flags[MAX_THREADS];     // Thread-specific flags
    DECLARE_REFERENCE(char*, name);

} YR_NAMESPACE;


#define META_TYPE_NULL      0
#define META_TYPE_INTEGER   1
#define META_TYPE_STRING    2
#define META_TYPE_BOOLEAN   3

#define META_IS_NULL(x) \
    ((x) != NULL ? (x)->type == META_TYPE_NULL : TRUE)


typedef struct _YR_META
{
    int32_t type;
    int32_t integer;

    DECLARE_REFERENCE(const char*, identifier);
    DECLARE_REFERENCE(char*, string);

} YR_META;


typedef struct _YR_MATCH
{
    int64_t base;
    int64_t offset;
    int32_t length;

    union
    {
        uint8_t* data;           // Confirmed matches use "data",
        int32_t chain_length;    // unconfirmed ones use "chain_length"
    };

    struct _YR_MATCH*  prev;
    struct _YR_MATCH*  next;

} YR_MATCH;


typedef struct _YR_MATCHES
{
    int32_t count;

    DECLARE_REFERENCE(YR_MATCH*, head);
    DECLARE_REFERENCE(YR_MATCH*, tail);

} YR_MATCHES;


#define STRING_GFLAGS_REFERENCED        0x01
#define STRING_GFLAGS_HEXADECIMAL       0x02
#define STRING_GFLAGS_NO_CASE           0x04
#define STRING_GFLAGS_ASCII             0x08
#define STRING_GFLAGS_WIDE              0x10
#define STRING_GFLAGS_REGEXP            0x20
#define STRING_GFLAGS_FAST_HEX_REGEXP   0x40
#define STRING_GFLAGS_FULL_WORD         0x80
#define STRING_GFLAGS_ANONYMOUS         0x100
#define STRING_GFLAGS_SINGLE_MATCH      0x200
#define STRING_GFLAGS_LITERAL           0x400
#define STRING_GFLAGS_FITS_IN_ATOM      0x800
#define STRING_GFLAGS_NULL              0x1000
#define STRING_GFLAGS_CHAIN_PART        0x2000
#define STRING_GFLAGS_CHAIN_TAIL        0x4000
#define STRING_GFLAGS_FIXED_OFFSET      0x8000


#define STRING_IS_HEX(x) \
    (((x)->g_flags) & STRING_GFLAGS_HEXADECIMAL)

#define STRING_IS_NO_CASE(x) \
    (((x)->g_flags) & STRING_GFLAGS_NO_CASE)

#define STRING_IS_ASCII(x) \
    (((x)->g_flags) & STRING_GFLAGS_ASCII)

#define STRING_IS_WIDE(x) \
    (((x)->g_flags) & STRING_GFLAGS_WIDE)

#define STRING_IS_REGEXP(x) \
    (((x)->g_flags) & STRING_GFLAGS_REGEXP)

#define STRING_IS_FULL_WORD(x) \
    (((x)->g_flags) & STRING_GFLAGS_FULL_WORD)

#define STRING_IS_ANONYMOUS(x) \
    (((x)->g_flags) & STRING_GFLAGS_ANONYMOUS)

#define STRING_IS_REFERENCED(x) \
    (((x)->g_flags) & STRING_GFLAGS_REFERENCED)

#define STRING_IS_SINGLE_MATCH(x) \
    (((x)->g_flags) & STRING_GFLAGS_SINGLE_MATCH)

#define STRING_IS_FIXED_OFFSET(x) \
    (((x)->g_flags) & STRING_GFLAGS_FIXED_OFFSET)

#define STRING_IS_LITERAL(x) \
    (((x)->g_flags) & STRING_GFLAGS_LITERAL)

#define STRING_IS_FAST_HEX_REGEXP(x) \
    (((x)->g_flags) & STRING_GFLAGS_FAST_HEX_REGEXP)

#define STRING_IS_CHAIN_PART(x) \
    (((x)->g_flags) & STRING_GFLAGS_CHAIN_PART)

#define STRING_IS_CHAIN_TAIL(x) \
    (((x)->g_flags) & STRING_GFLAGS_CHAIN_TAIL)

#define STRING_IS_NULL(x) \
    ((x) == NULL || ((x)->g_flags) & STRING_GFLAGS_NULL)

#define STRING_FITS_IN_ATOM(x) \
    (((x)->g_flags) & STRING_GFLAGS_FITS_IN_ATOM)

#define STRING_FOUND(x) \
    ((x)->matches[yr_get_tidx()].tail != NULL)

#define STRING_MATCHES(x) \
    ((x)->matches[yr_get_tidx()])


typedef struct _YR_STRING
{
    int32_t g_flags;
    int32_t length;

    DECLARE_REFERENCE(char*, identifier);
    DECLARE_REFERENCE(uint8_t*, string);
    DECLARE_REFERENCE(struct _YR_STRING*, chained_to);

    int32_t chain_gap_min;
    int32_t chain_gap_max;

    int64_t fixed_offset;

    YR_MATCHES matches[MAX_THREADS];
    YR_MATCHES unconfirmed_matches[MAX_THREADS];

#ifdef PROFILING_ENABLED
    uint64_t clock_ticks;
#endif

} YR_STRING;


#define RULE_TFLAGS_MATCH                0x01

#define RULE_GFLAGS_PRIVATE              0x01
#define RULE_GFLAGS_GLOBAL               0x02
#define RULE_GFLAGS_REQUIRE_EXECUTABLE   0x04
#define RULE_GFLAGS_REQUIRE_FILE         0x08
#define RULE_GFLAGS_NULL                 0x1000

#define RULE_IS_PRIVATE(x) \
    (((x)->g_flags) & RULE_GFLAGS_PRIVATE)

#define RULE_IS_GLOBAL(x) \
    (((x)->g_flags) & RULE_GFLAGS_GLOBAL)

#define RULE_IS_NULL(x) \
    (((x)->g_flags) & RULE_GFLAGS_NULL)

#define RULE_MATCHES(x) \
    ((x)->t_flags[yr_get_tidx()] & RULE_TFLAGS_MATCH)


typedef struct _YR_RULE
{
    int32_t g_flags;               // Global flags
    int32_t t_flags[MAX_THREADS];  // Thread-specific flags

    DECLARE_REFERENCE(const char*, identifier);
    DECLARE_REFERENCE(const char*, tags);
    DECLARE_REFERENCE(YR_META*, metas);
    DECLARE_REFERENCE(YR_STRING*, strings);
    DECLARE_REFERENCE(YR_NAMESPACE*, ns);

#ifdef PROFILING_ENABLED
    uint64_t clock_ticks;
#endif

} YR_RULE;


#define EXTERNAL_VARIABLE_TYPE_NULL           0
#define EXTERNAL_VARIABLE_TYPE_FLOAT          1
#define EXTERNAL_VARIABLE_TYPE_INTEGER        2
#define EXTERNAL_VARIABLE_TYPE_BOOLEAN        3
#define EXTERNAL_VARIABLE_TYPE_STRING         4
#define EXTERNAL_VARIABLE_TYPE_MALLOC_STRING  5


#define EXTERNAL_VARIABLE_IS_NULL(x) \
    ((x) != NULL ? (x)->type == EXTERNAL_VARIABLE_TYPE_NULL : TRUE)


typedef struct _YR_EXTERNAL_VARIABLE
{
    int32_t type;

    union
    {
        int64_t i;
        double f;
        char* s;
    } value;

    DECLARE_REFERENCE(char*, identifier);

} YR_EXTERNAL_VARIABLE;


typedef struct _YR_AC_MATCH
{
    uint16_t backtrack;

    DECLARE_REFERENCE(YR_STRING*, string);
    DECLARE_REFERENCE(uint8_t*, forward_code);
    DECLARE_REFERENCE(uint8_t*, backward_code);
    DECLARE_REFERENCE(struct _YR_AC_MATCH*, next);

} YR_AC_MATCH;


typedef struct _YR_AC_STATE
{
    int8_t depth;

    DECLARE_REFERENCE(struct _YR_AC_STATE*, failure);
    DECLARE_REFERENCE(YR_AC_MATCH*, matches);

} YR_AC_STATE;


typedef struct _YR_AC_STATE_TRANSITION
{
    uint8_t input;

    DECLARE_REFERENCE(YR_AC_STATE*, state);
    DECLARE_REFERENCE(struct _YR_AC_STATE_TRANSITION*, next);

} YR_AC_STATE_TRANSITION;


typedef struct _YR_AC_TABLE_BASED_STATE
{
    int8_t depth;

    DECLARE_REFERENCE(YR_AC_STATE*, failure);
    DECLARE_REFERENCE(YR_AC_MATCH*, matches);
    DECLARE_REFERENCE(YR_AC_STATE*, state) transitions[256];

} YR_AC_TABLE_BASED_STATE;


typedef struct _YR_AC_LIST_BASED_STATE
{
    int8_t depth;

    DECLARE_REFERENCE(YR_AC_STATE*, failure);
    DECLARE_REFERENCE(YR_AC_MATCH*, matches);
    DECLARE_REFERENCE(YR_AC_STATE_TRANSITION*, transitions);

} YR_AC_LIST_BASED_STATE;


typedef struct _YR_AC_AUTOMATON
{
    DECLARE_REFERENCE(YR_AC_STATE*, root);

} YR_AC_AUTOMATON;


typedef struct _YARA_RULES_FILE_HEADER
{
    uint32_t version;

    DECLARE_REFERENCE(YR_RULE*, rules_list_head);
    DECLARE_REFERENCE(YR_EXTERNAL_VARIABLE*, externals_list_head);
    DECLARE_REFERENCE(uint8_t*, code_start);
    DECLARE_REFERENCE(YR_AC_AUTOMATON*, automaton);

} YARA_RULES_FILE_HEADER;



#pragma pack(pop)


typedef struct _YR_RULES
{

    tidx_mask_t tidx_mask;
    uint8_t* code_start;

    mutex_t mutex;

    YR_ARENA* arena;
    YR_RULE* rules_list_head;
    YR_EXTERNAL_VARIABLE* externals_list_head;
    YR_AC_AUTOMATON* automaton;

} YR_RULES;


typedef struct _YR_MEMORY_BLOCK
{
    uint8_t* data;
    size_t size;
    size_t base;

    struct _YR_MEMORY_BLOCK* next;

} YR_MEMORY_BLOCK;


typedef int (*YR_CALLBACK_FUNC)(
    int message,
    void* message_data,
    void* user_data);


typedef struct _YR_SCAN_CONTEXT
{
    uint64_t  file_size;
    uint64_t  entry_point;

    int flags;
    void* user_data;

    YR_MEMORY_BLOCK*  mem_block;
    YR_HASH_TABLE*  objects_table;
    YR_CALLBACK_FUNC  callback;

} YR_SCAN_CONTEXT;



#define OBJECT_COMMON_FIELDS \
    int8_t type; \
    const char* identifier; \
    void* data; \
    struct _YR_OBJECT* parent;


typedef struct _YR_OBJECT
{
    OBJECT_COMMON_FIELDS

} YR_OBJECT;


typedef struct _YR_OBJECT_INTEGER
{
    OBJECT_COMMON_FIELDS
    int64_t value;

} YR_OBJECT_INTEGER;


typedef struct _YR_OBJECT_DOUBLE
{
    OBJECT_COMMON_FIELDS
    double value;

} YR_OBJECT_DOUBLE;


typedef struct _YR_OBJECT_STRING
{
    OBJECT_COMMON_FIELDS
    SIZED_STRING* value;

} YR_OBJECT_STRING;


typedef struct _YR_OBJECT_REGEXP
{
    OBJECT_COMMON_FIELDS
    RE* value;

} YR_OBJECT_REGEXP;


typedef struct _YR_OBJECT_STRUCTURE
{
    OBJECT_COMMON_FIELDS
    struct _YR_STRUCTURE_MEMBER* members;

} YR_OBJECT_STRUCTURE;


typedef struct _YR_OBJECT_ARRAY
{
    OBJECT_COMMON_FIELDS
    YR_OBJECT* prototype_item;
    struct _YR_ARRAY_ITEMS* items;

} YR_OBJECT_ARRAY;


typedef struct _YR_OBJECT_DICTIONARY
{
    OBJECT_COMMON_FIELDS
    YR_OBJECT* prototype_item;
    struct _YR_DICTIONARY_ITEMS* items;

} YR_OBJECT_DICTIONARY;


struct _YR_OBJECT_FUNCTION;


typedef int (*YR_MODULE_FUNC)(
    void* args,
    YR_SCAN_CONTEXT* context,
    struct _YR_OBJECT_FUNCTION* function_obj);


typedef struct _YR_OBJECT_FUNCTION
{
    OBJECT_COMMON_FIELDS

    YR_OBJECT* return_obj;

    struct
    {
        const char* arguments_fmt;
        YR_MODULE_FUNC code;
    } prototypes[MAX_OVERLOADED_FUNCTIONS];

} YR_OBJECT_FUNCTION;


typedef struct _YR_STRUCTURE_MEMBER
{
    YR_OBJECT* object;
    struct _YR_STRUCTURE_MEMBER* next;

} YR_STRUCTURE_MEMBER;


typedef struct _YR_ARRAY_ITEMS
{
    int count;
    YR_OBJECT* objects[1];

} YR_ARRAY_ITEMS;


typedef struct _YR_DICTIONARY_ITEMS
{
    int used;
    int free;

    struct
    {

        char* key;
        YR_OBJECT* obj;

    } objects[1];

} YR_DICTIONARY_ITEMS;


#endif
