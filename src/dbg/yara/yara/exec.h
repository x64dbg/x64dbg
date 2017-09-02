/*
Copyright (c) 2013-2014. The YARA Authors. All Rights Reserved.

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

#ifndef YR_EXEC_H
#define YR_EXEC_H

#include "hash.h"
#include "scan.h"
#include "types.h"
#include "rules.h"


#define UNDEFINED           0xFFFABADAFABADAFFLL
#define IS_UNDEFINED(x)     ((size_t)(x) == (size_t) UNDEFINED)

#define OP_ERROR          0
#define OP_HALT           255
#define OP_NOP            254

#define OP_AND            1
#define OP_OR             2
#define OP_NOT            3
#define OP_BITWISE_NOT    4
#define OP_BITWISE_AND    5
#define OP_BITWISE_OR     6
#define OP_BITWISE_XOR    7
#define OP_SHL            8
#define OP_SHR            9
#define OP_MOD            10
#define OP_INT_TO_DBL     11
#define OP_STR_TO_BOOL    12
#define OP_PUSH           13
#define OP_POP            14
#define OP_CALL           15
#define OP_OBJ_LOAD       16
#define OP_OBJ_VALUE      17
#define OP_OBJ_FIELD      18
#define OP_INDEX_ARRAY    19
#define OP_COUNT          20
#define OP_LENGTH         21
#define OP_FOUND          22
#define OP_FOUND_AT       23
#define OP_FOUND_IN       24
#define OP_OFFSET         25
#define OP_OF             26
#define OP_PUSH_RULE      27
#define OP_INIT_RULE      28
#define OP_MATCH_RULE     29
#define OP_INCR_M         30
#define OP_CLEAR_M        31
#define OP_ADD_M          32
#define OP_POP_M          33
#define OP_PUSH_M         34
#define OP_SWAPUNDEF      35
#define OP_JNUNDEF        36
#define OP_JLE            37
#define OP_FILESIZE       38
#define OP_ENTRYPOINT     39
#define OP_CONTAINS       40
#define OP_MATCHES        41
#define OP_IMPORT         42
#define OP_LOOKUP_DICT    43
#define OP_JFALSE         44
#define OP_JTRUE          45


#define _OP_EQ            0
#define _OP_NEQ           1
#define _OP_LT            2
#define _OP_GT            3
#define _OP_LE            4
#define _OP_GE            5
#define _OP_ADD           6
#define _OP_SUB           7
#define _OP_MUL           8
#define _OP_DIV           9
#define _OP_MINUS         10


#define OP_INT_BEGIN      100
#define OP_INT_EQ         (OP_INT_BEGIN + _OP_EQ)
#define OP_INT_NEQ        (OP_INT_BEGIN + _OP_NEQ)
#define OP_INT_LT         (OP_INT_BEGIN + _OP_LT)
#define OP_INT_GT         (OP_INT_BEGIN + _OP_GT)
#define OP_INT_LE         (OP_INT_BEGIN + _OP_LE)
#define OP_INT_GE         (OP_INT_BEGIN + _OP_GE)
#define OP_INT_ADD        (OP_INT_BEGIN + _OP_ADD)
#define OP_INT_SUB        (OP_INT_BEGIN + _OP_SUB)
#define OP_INT_MUL        (OP_INT_BEGIN + _OP_MUL)
#define OP_INT_DIV        (OP_INT_BEGIN + _OP_DIV)
#define OP_INT_MINUS      (OP_INT_BEGIN + _OP_MINUS)
#define OP_INT_END        OP_INT_MINUS

#define OP_DBL_BEGIN      120
#define OP_DBL_EQ         (OP_DBL_BEGIN + _OP_EQ)
#define OP_DBL_NEQ        (OP_DBL_BEGIN + _OP_NEQ)
#define OP_DBL_LT         (OP_DBL_BEGIN + _OP_LT)
#define OP_DBL_GT         (OP_DBL_BEGIN + _OP_GT)
#define OP_DBL_LE         (OP_DBL_BEGIN + _OP_LE)
#define OP_DBL_GE         (OP_DBL_BEGIN + _OP_GE)
#define OP_DBL_ADD        (OP_DBL_BEGIN + _OP_ADD)
#define OP_DBL_SUB        (OP_DBL_BEGIN + _OP_SUB)
#define OP_DBL_MUL        (OP_DBL_BEGIN + _OP_MUL)
#define OP_DBL_DIV        (OP_DBL_BEGIN + _OP_DIV)
#define OP_DBL_MINUS      (OP_DBL_BEGIN + _OP_MINUS)
#define OP_DBL_END        OP_DBL_MINUS

#define OP_STR_BEGIN      140
#define OP_STR_EQ         (OP_STR_BEGIN + _OP_EQ)
#define OP_STR_NEQ        (OP_STR_BEGIN + _OP_NEQ)
#define OP_STR_LT         (OP_STR_BEGIN + _OP_LT)
#define OP_STR_GT         (OP_STR_BEGIN + _OP_GT)
#define OP_STR_LE         (OP_STR_BEGIN + _OP_LE)
#define OP_STR_GE         (OP_STR_BEGIN + _OP_GE)
#define OP_STR_END        OP_STR_GE

#define IS_INT_OP(x)      ((x) >= OP_INT_BEGIN && (x) <= OP_INT_END)
#define IS_DBL_OP(x)      ((x) >= OP_DBL_BEGIN && (x) <= OP_DBL_END)
#define IS_STR_OP(x)      ((x) >= OP_STR_BEGIN && (x) <= OP_STR_END)

#define OP_READ_INT       240
#define OP_INT8           (OP_READ_INT + 0)
#define OP_INT16          (OP_READ_INT + 1)
#define OP_INT32          (OP_READ_INT + 2)
#define OP_UINT8          (OP_READ_INT + 3)
#define OP_UINT16         (OP_READ_INT + 4)
#define OP_UINT32         (OP_READ_INT + 5)
#define OP_INT8BE         (OP_READ_INT + 6)
#define OP_INT16BE        (OP_READ_INT + 7)
#define OP_INT32BE        (OP_READ_INT + 8)
#define OP_UINT8BE        (OP_READ_INT + 9)
#define OP_UINT16BE       (OP_READ_INT + 10)
#define OP_UINT32BE       (OP_READ_INT + 11)


#define OPERATION(operator, op1, op2) \
    (IS_UNDEFINED(op1) || IS_UNDEFINED(op2)) ? (UNDEFINED) : (op1 operator op2)


#define COMPARISON(operator, op1, op2) \
    (IS_UNDEFINED(op1) || IS_UNDEFINED(op2)) ? (0) : (op1 operator op2)


int yr_execute_code(
    YR_RULES* rules,
    YR_SCAN_CONTEXT* context,
    int timeout,
    time_t start_time);

#endif
