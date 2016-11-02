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

#ifndef YR_HASH_H
#define YR_HASH_H

#include <stddef.h>

#include "utils.h"

typedef struct _YR_HASH_TABLE_ENTRY
{
    void* key;
    size_t key_length;
    char* ns;
    void* value;

    struct _YR_HASH_TABLE_ENTRY* next;

} YR_HASH_TABLE_ENTRY;


typedef struct _YR_HASH_TABLE
{
    int size;

    YR_HASH_TABLE_ENTRY* buckets[1];

} YR_HASH_TABLE;


typedef int (*YR_HASH_TABLE_FREE_VALUE_FUNC)(void* value);


YR_API int yr_hash_table_create(
    int size,
    YR_HASH_TABLE** table);


YR_API void yr_hash_table_clean(
    YR_HASH_TABLE* table,
    YR_HASH_TABLE_FREE_VALUE_FUNC free_value);


YR_API void yr_hash_table_destroy(
    YR_HASH_TABLE* table,
    YR_HASH_TABLE_FREE_VALUE_FUNC free_value);


YR_API void* yr_hash_table_lookup(
    YR_HASH_TABLE* table,
    const char* key,
    const char* ns);


YR_API int yr_hash_table_add(
    YR_HASH_TABLE* table,
    const char* key,
    const char* ns,
    void* value);


YR_API void* yr_hash_table_lookup_raw_key(
    YR_HASH_TABLE* table,
    const void* key,
    size_t key_length,
    const char* ns);


YR_API int yr_hash_table_add_raw_key(
    YR_HASH_TABLE* table,
    const void* key,
    size_t key_length,
    const char* ns,
    void* value);

#endif
