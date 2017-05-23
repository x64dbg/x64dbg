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

#ifndef YR_TYPES_H
#define YR_TYPES_H


#include "arena.h"
#include "re.h"
#include "limits.h"
#include "hash.h"
#include "utils.h"
#include "threading.h"



#ifdef PROFILING_ENABLED
#include <time.h>
#endif


#define DECLARE_REFERENCE(type, name) \
    union { type name; int64_t name##_; } YR_ALIGN(8)



#define NAMESPACE_TFLAGS_UNSATISFIED_GLOBAL      0x01


#define STRING_GFLAGS_REFERENCED        0x01
#define STRING_GFLAGS_HEXADECIMAL       0x02
#define STRING_GFLAGS_NO_CASE           0x04
#define STRING_GFLAGS_ASCII             0x08
#define STRING_GFLAGS_WIDE              0x10
#define STRING_GFLAGS_REGEXP            0x20
#define STRING_GFLAGS_FAST_REGEXP       0x40
#define STRING_GFLAGS_FULL_WORD         0x80
#define STRING_GFLAGS_ANONYMOUS         0x100
#define STRING_GFLAGS_SINGLE_MATCH      0x200
#define STRING_GFLAGS_LITERAL           0x400
#define STRING_GFLAGS_FITS_IN_ATOM      0x800
#define STRING_GFLAGS_NULL              0x1000
#define STRING_GFLAGS_CHAIN_PART        0x2000
#define STRING_GFLAGS_CHAIN_TAIL        0x4000
#define STRING_GFLAGS_FIXED_OFFSET      0x8000
#define STRING_GFLAGS_GREEDY_REGEXP     0x10000
#define STRING_GFLAGS_DOT_ALL           0x20000

#define STRING_IS_HEX(x) \
    (((x)->g_flags) & STRING_GFLAGS_HEXADECIMAL)

#define STRING_IS_NO_CASE(x) \
    (((x)->g_flags) & STRING_GFLAGS_NO_CASE)

#define STRING_IS_DOT_ALL(x) \
    (((x)->g_flags) & STRING_GFLAGS_DOT_ALL)

#define STRING_IS_ASCII(x) \
    (((x)->g_flags) & STRING_GFLAGS_ASCII)

#define STRING_IS_WIDE(x) \
    (((x)->g_flags) & STRING_GFLAGS_WIDE)

#define STRING_IS_REGEXP(x) \
    (((x)->g_flags) & STRING_GFLAGS_REGEXP)

#define STRING_IS_GREEDY_REGEXP(x) \
    (((x)->g_flags) & STRING_GFLAGS_GREEDY_REGEXP)

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

#define STRING_IS_FAST_REGEXP(x) \
    (((x)->g_flags) & STRING_GFLAGS_FAST_REGEXP)

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


#define META_TYPE_NULL      0
#define META_TYPE_INTEGER   1
#define META_TYPE_STRING    2
#define META_TYPE_BOOLEAN   3

#define META_IS_NULL(x) \
    ((x) != NULL ? (x)->type == META_TYPE_NULL : TRUE)


#define EXTERNAL_VARIABLE_TYPE_NULL           0
#define EXTERNAL_VARIABLE_TYPE_FLOAT          1
#define EXTERNAL_VARIABLE_TYPE_INTEGER        2
#define EXTERNAL_VARIABLE_TYPE_BOOLEAN        3
#define EXTERNAL_VARIABLE_TYPE_STRING         4
#define EXTERNAL_VARIABLE_TYPE_MALLOC_STRING  5

#define EXTERNAL_VARIABLE_IS_NULL(x) \
    ((x) != NULL ? (x)->type == EXTERNAL_VARIABLE_TYPE_NULL : TRUE)


#pragma pack(push)
#pragma pack(8)


typedef struct _YR_NAMESPACE
{
    int32_t t_flags[MAX_THREADS];     // Thread-specific flags
    DECLARE_REFERENCE(char*, name);

} YR_NAMESPACE;


typedef struct _YR_META
{
    int32_t type;
    YR_ALIGN(8) int64_t integer;

    DECLARE_REFERENCE(const char*, identifier);
    DECLARE_REFERENCE(char*, string);

} YR_META;


struct _YR_MATCH;


typedef struct _YR_MATCHES
{
    int32_t count;

    DECLARE_REFERENCE(struct _YR_MATCH*, head);
    DECLARE_REFERENCE(struct _YR_MATCH*, tail);

} YR_MATCHES;


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
    clock_t clock_ticks;
#endif

} YR_STRING;


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
    clock_t clock_ticks;
#endif

} YR_RULE;


typedef struct _YR_EXTERNAL_VARIABLE
{
    int32_t type;

    YR_ALIGN(8) union
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


typedef struct _YR_AC_MATCH_TABLE_ENTRY
{
    DECLARE_REFERENCE(YR_AC_MATCH*, match);

} YR_AC_MATCH_TABLE_ENTRY;


typedef uint64_t                  YR_AC_TRANSITION;
typedef YR_AC_TRANSITION*         YR_AC_TRANSITION_TABLE;
typedef YR_AC_MATCH_TABLE_ENTRY*  YR_AC_MATCH_TABLE;


typedef struct _YARA_RULES_FILE_HEADER
{
    DECLARE_REFERENCE(YR_RULE*, rules_list_head);
    DECLARE_REFERENCE(YR_EXTERNAL_VARIABLE*, externals_list_head);
    DECLARE_REFERENCE(uint8_t*, code_start);
    DECLARE_REFERENCE(YR_AC_MATCH_TABLE, match_table);
    DECLARE_REFERENCE(YR_AC_TRANSITION_TABLE, transition_table);

} YARA_RULES_FILE_HEADER;

#pragma pack(pop)


//
// Structs defined below are never stored in the compiled rules file
//

typedef struct _YR_MATCH
{
    int64_t base;              // Base address for the match
    int64_t offset;            // Offset relative to base for the match
    int32_t match_length;      // Match length
    int32_t data_length;

    // Pointer to a buffer containing a portion of the matched data. The size of
    // the buffer is data_length. data_length is always <= length and is limited
    // to MAX_MATCH_DATA bytes.

    uint8_t* data;

    // If the match belongs to a chained string chain_length contains the
    // length of the chain. This field is used only in unconfirmed matches.

    int32_t chain_length;

    struct _YR_MATCH* prev;
    struct _YR_MATCH* next;

} YR_MATCH;


struct _YR_AC_STATE;


typedef struct _YR_AC_STATE
{
    uint8_t depth;
    uint8_t input;

    uint32_t t_table_slot;

    struct _YR_AC_STATE* failure;
    struct _YR_AC_STATE* first_child;
    struct _YR_AC_STATE* siblings;

    YR_AC_MATCH* matches;

} YR_AC_STATE;


typedef struct _YR_AC_AUTOMATON
{
    // Both m_table and t_table have the same number of elements, which is
    // stored in tables_size.

    uint32_t tables_size;
    uint32_t t_table_unused_candidate;

    YR_AC_TRANSITION_TABLE t_table;
    YR_AC_MATCH_TABLE m_table;

    YR_AC_STATE* root;

} YR_AC_AUTOMATON;


typedef struct _YR_RULES
{

    unsigned char tidx_mask[YR_BITARRAY_NCHARS(MAX_THREADS)];
    uint8_t* code_start;

    YR_MUTEX mutex;
    YR_ARENA* arena;
    YR_RULE* rules_list_head;
    YR_EXTERNAL_VARIABLE* externals_list_head;
    YR_AC_TRANSITION_TABLE transition_table;
    YR_AC_MATCH_TABLE match_table;

} YR_RULES;


struct _YR_MEMORY_BLOCK;
struct _YR_MEMORY_BLOCK_ITERATOR;


typedef uint8_t* (*YR_MEMORY_BLOCK_FETCH_DATA_FUNC)(
    struct _YR_MEMORY_BLOCK* self);


typedef struct _YR_MEMORY_BLOCK* (*YR_MEMORY_BLOCK_ITERATOR_FUNC)(
    struct _YR_MEMORY_BLOCK_ITERATOR* self);


typedef struct _YR_MEMORY_BLOCK
{
    size_t size;
    size_t base;

    void* context;

    YR_MEMORY_BLOCK_FETCH_DATA_FUNC fetch_data;

} YR_MEMORY_BLOCK;


typedef struct _YR_MEMORY_BLOCK_ITERATOR
{
    void* context;

    YR_MEMORY_BLOCK_ITERATOR_FUNC  first;
    YR_MEMORY_BLOCK_ITERATOR_FUNC  next;

} YR_MEMORY_BLOCK_ITERATOR;


typedef int (*YR_CALLBACK_FUNC)(
    int message,
    void* message_data,
    void* user_data);


typedef struct _YR_SCAN_CONTEXT
{
    uint64_t  file_size;
    uint64_t  entry_point;

    int flags;
    int tidx;

    void* user_data;

    YR_MEMORY_BLOCK_ITERATOR*  iterator;
    YR_HASH_TABLE*  objects_table;
    YR_CALLBACK_FUNC  callback;

    YR_ARENA* matches_arena;
    YR_ARENA* matching_strings_arena;

} YR_SCAN_CONTEXT;


struct _YR_OBJECT;


typedef union _YR_VALUE
{
    int64_t i;
    double d;
    void* p;
    struct _YR_OBJECT* o;
    YR_STRING* s;
    SIZED_STRING* ss;
    RE* re;

} YR_VALUE;


#define OBJECT_COMMON_FIELDS \
    int8_t type; \
    const char* identifier; \
    struct _YR_OBJECT* parent; \
    void* data;


typedef struct _YR_OBJECT
{
    OBJECT_COMMON_FIELDS
    YR_VALUE value;

} YR_OBJECT;


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
    YR_VALUE* args,
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


#define object_as_structure(obj)  ((YR_OBJECT_STRUCTURE*) (obj))
#define object_as_array(obj)      ((YR_OBJECT_ARRAY*) (obj))
#define object_as_dictionary(obj) ((YR_OBJECT_DICTIONARY*) (obj))
#define object_as_function(obj)   ((YR_OBJECT_FUNCTION*) (obj))


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
