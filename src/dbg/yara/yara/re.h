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

#ifndef YR_RE_H
#define YR_RE_H

#include <ctype.h>

#include "arena.h"
#include "sizedstr.h"

#define RE_NODE_LITERAL             1
#define RE_NODE_MASKED_LITERAL      2
#define RE_NODE_ANY                 3
#define RE_NODE_CONCAT              4
#define RE_NODE_ALT                 5
#define RE_NODE_RANGE               6
#define RE_NODE_STAR                7
#define RE_NODE_PLUS                8
#define RE_NODE_CLASS               9
#define RE_NODE_WORD_CHAR           10
#define RE_NODE_NON_WORD_CHAR       11
#define RE_NODE_SPACE               12
#define RE_NODE_NON_SPACE           13
#define RE_NODE_DIGIT               14
#define RE_NODE_NON_DIGIT           15
#define RE_NODE_EMPTY               16
#define RE_NODE_ANCHOR_START        17
#define RE_NODE_ANCHOR_END          18
#define RE_NODE_WORD_BOUNDARY       19
#define RE_NODE_NON_WORD_BOUNDARY   20


#define RE_OPCODE_ANY                   0xA0
#define RE_OPCODE_ANY_EXCEPT_NEW_LINE   0xA1
#define RE_OPCODE_LITERAL               0xA2
#define RE_OPCODE_LITERAL_NO_CASE       0xA3
#define RE_OPCODE_MASKED_LITERAL        0xA4
#define RE_OPCODE_CLASS                 0xA5
#define RE_OPCODE_CLASS_NO_CASE         0xA6
#define RE_OPCODE_WORD_CHAR             0xA7
#define RE_OPCODE_NON_WORD_CHAR         0xA8
#define RE_OPCODE_SPACE                 0xA9
#define RE_OPCODE_NON_SPACE             0xAA
#define RE_OPCODE_DIGIT                 0xAB
#define RE_OPCODE_NON_DIGIT             0xAC
#define RE_OPCODE_MATCH                 0xAD

#define RE_OPCODE_MATCH_AT_END          0xB0
#define RE_OPCODE_MATCH_AT_START        0xB1
#define RE_OPCODE_WORD_BOUNDARY         0xB2
#define RE_OPCODE_NON_WORD_BOUNDARY     0xB3

#define RE_OPCODE_SPLIT_A               0xC0
#define RE_OPCODE_SPLIT_B               0xC1
#define RE_OPCODE_PUSH                  0xC2
#define RE_OPCODE_POP                   0xC3
#define RE_OPCODE_JNZ                   0xC4
#define RE_OPCODE_JUMP                  0xC5


#define RE_FLAGS_FAST_HEX_REGEXP          0x02
#define RE_FLAGS_BACKWARDS                0x04
#define RE_FLAGS_EXHAUSTIVE               0x08
#define RE_FLAGS_WIDE                     0x10
#define RE_FLAGS_NO_CASE                  0x20
#define RE_FLAGS_SCAN                     0x40
#define RE_FLAGS_DOT_ALL                  0x80
#define RE_FLAGS_NOT_AT_START            0x100


typedef struct RE RE;
typedef struct RE_NODE RE_NODE;
typedef struct RE_ERROR RE_ERROR;

typedef uint8_t* RE_CODE;

#define CHAR_IN_CLASS(chr, cls)  \
    ((cls)[(chr) / 8] & 1 << ((chr) % 8))


#define IS_WORD_CHAR(chr) \
    (isalnum(chr) || (chr) == '_')


struct RE_NODE
{
    int type;

    union
    {
        int value;
        int count;
        int start;
    };

    union
    {
        int mask;
        int end;
    };

    int greedy;

    uint8_t* class_vector;

    RE_NODE* left;
    RE_NODE* right;

    RE_CODE forward_code;
    RE_CODE backward_code;
};


struct RE
{

    uint32_t flags;
    RE_NODE* root_node;
    YR_ARENA* code_arena;
    RE_CODE code;
};


struct RE_ERROR
{

    char message[512];

};


typedef int RE_MATCH_CALLBACK_FUNC(
    uint8_t* match,
    int match_length,
    int flags,
    void* args);


int yr_re_create(
    RE** re);


int yr_re_parse(
    const char* re_string,
    int flags,
    RE** re,
    RE_ERROR* error);


int yr_re_parse_hex(
    const char* hex_string,
    int flags,
    RE** re,
    RE_ERROR* error);


int yr_re_compile(
    const char* re_string,
    int flags,
    YR_ARENA* code_arena,
    RE** re,
    RE_ERROR* error);


void yr_re_destroy(
    RE* re);


void yr_re_print(
    RE* re);


RE_NODE* yr_re_node_create(
    int type,
    RE_NODE* left,
    RE_NODE* right);


void yr_re_node_destroy(
    RE_NODE* node);


SIZED_STRING* yr_re_extract_literal(
    RE* re);


int yr_re_contains_dot_star(
    RE* re);


int yr_re_split_at_chaining_point(
    RE* re,
    RE** result_re,
    RE** remainder_re,
    int32_t* min_gap,
    int32_t* max_gap);


int yr_re_emit_code(
    RE* re,
    YR_ARENA* arena);


int yr_re_exec(
    RE_CODE re_code,
    uint8_t* input,
    size_t input_size,
    int flags,
    RE_MATCH_CALLBACK_FUNC callback,
    void* callback_args);


int yr_re_match(
    RE_CODE re_code,
    const char* target);


int yr_re_initialize(void);


int yr_re_finalize(void);


int yr_re_finalize_thread(void);

#endif
