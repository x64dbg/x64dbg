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

#ifndef YR_PARSER_H
#define YR_PARSER_H


#include "lexer.h"


int yr_parser_emit(
    yyscan_t yyscanner,
    uint8_t instruction,
    uint8_t** instruction_address);


int yr_parser_emit_with_arg(
    yyscan_t yyscanner,
    uint8_t instruction,
    int64_t argument,
    uint8_t** instruction_address,
    int64_t** argument_address);


int yr_parser_emit_with_arg_double(
    yyscan_t yyscanner,
    uint8_t instruction,
    double argument,
    uint8_t** instruction_address,
    double** argument_address);


int yr_parser_emit_with_arg_reloc(
    yyscan_t yyscanner,
    uint8_t instruction,
    void* argument,
    uint8_t** instruction_address,
    void** argument_address);


int yr_parser_check_types(
    YR_COMPILER* compiler,
    YR_OBJECT_FUNCTION* function,
    const char* actual_args_fmt);


YR_STRING* yr_parser_lookup_string(
    yyscan_t yyscanner,
    const char* identifier);


int yr_parser_lookup_loop_variable(
    yyscan_t yyscanner,
    const char* identifier);


YR_RULE* yr_parser_reduce_rule_declaration_phase_1(
    yyscan_t yyscanner,
    int32_t flags,
    const char* identifier);


int yr_parser_reduce_rule_declaration_phase_2(
    yyscan_t yyscanner,
    YR_RULE* rule);


YR_STRING* yr_parser_reduce_string_declaration(
    yyscan_t yyscanner,
    int32_t flags,
    const char* identifier,
    SIZED_STRING* str);


YR_META* yr_parser_reduce_meta_declaration(
    yyscan_t yyscanner,
    int32_t type,
    const char* identifier,
    const char* string,
    int64_t integer);


int yr_parser_reduce_string_identifier(
    yyscan_t yyscanner,
    const char* identifier,
    uint8_t instruction,
    uint64_t at_offset);


int yr_parser_emit_pushes_for_strings(
    yyscan_t yyscanner,
    const char* identifier);


int yr_parser_reduce_external(
    yyscan_t yyscanner,
    const char* identifier,
    uint8_t instruction);


int yr_parser_reduce_import(
    yyscan_t yyscanner,
    SIZED_STRING* module_name);


int yr_parser_reduce_operation(
    yyscan_t yyscanner,
    const char* operation,
    EXPRESSION left_operand,
    EXPRESSION right_operand);

#endif
