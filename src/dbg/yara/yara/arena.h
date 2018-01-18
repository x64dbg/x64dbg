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

#ifndef YR_ARENA_H
#define YR_ARENA_H

#include <stddef.h>

#include "integers.h"
#include "stream.h"

#define ARENA_FLAGS_FIXED_SIZE   1
#define ARENA_FLAGS_COALESCED    2
#define ARENA_FILE_VERSION       ((13 << 16) | MAX_THREADS)

#define EOL ((size_t) -1)


typedef struct _YR_RELOC
{
    uint32_t offset;
    struct _YR_RELOC* next;

} YR_RELOC;


typedef struct _YR_ARENA_PAGE
{

    uint8_t* new_address;
    uint8_t* address;

    size_t size;
    size_t used;

    YR_RELOC* reloc_list_head;
    YR_RELOC* reloc_list_tail;

    struct _YR_ARENA_PAGE* next;
    struct _YR_ARENA_PAGE* prev;

} YR_ARENA_PAGE;


typedef struct _YR_ARENA
{
    int flags;

    YR_ARENA_PAGE* page_list_head;
    YR_ARENA_PAGE* current_page;

} YR_ARENA;


int yr_arena_create(
    size_t initial_size,
    int flags,
    YR_ARENA** arena);


void yr_arena_destroy(
    YR_ARENA* arena);


void* yr_arena_base_address(
    YR_ARENA* arena);


void* yr_arena_next_address(
    YR_ARENA* arena,
    void* address,
    size_t offset);


int yr_arena_coalesce(
    YR_ARENA* arena);


int yr_arena_reserve_memory(
    YR_ARENA* arena,
    size_t size);


int yr_arena_allocate_memory(
    YR_ARENA* arena,
    size_t size,
    void** allocated_memory);


int yr_arena_allocate_struct(
    YR_ARENA* arena,
    size_t size,
    void** allocated_memory,
    ...);


int yr_arena_make_relocatable(
    YR_ARENA* arena,
    void* base,
    ...);


int yr_arena_write_data(
    YR_ARENA* arena,
    void* data,
    size_t size,
    void** written_data);


int yr_arena_write_string(
    YR_ARENA* arena,
    const char* string,
    char** written_string);


int yr_arena_append(
    YR_ARENA* target_arena,
    YR_ARENA* source_arena);


int yr_arena_load_stream(
    YR_STREAM* stream,
    YR_ARENA** arena);


int yr_arena_save_stream(
    YR_ARENA* arena,
    YR_STREAM* stream);


int yr_arena_duplicate(
    YR_ARENA* arena,
    YR_ARENA** duplicated);


void yr_arena_print(
    YR_ARENA* arena);

#endif
