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

#ifndef YR_PARSER_H
#define YR_PARSER_H


#include "lexer.h"


int yr_parser_emit(
    yyscan_t yyscanner,
    int8_t instruction,
    int8_t** instruction_address);


int yr_parser_emit_with_arg(
    yyscan_t yyscanner,
    int8_t instruction,
    int64_t argument,
    int8_t** instruction_address);


int yr_parser_emit_with_arg_double(
    yyscan_t yyscanner,
    int8_t instruction,
    double argument,
    int8_t** instruction_address);


int yr_parser_emit_with_arg_reloc(
    yyscan_t yyscanner,
    int8_t instruction,
    int64_t argument,
    int8_t** instruction_address);


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


int yr_parser_reduce_rule_declaration(
    yyscan_t yyscanner,
    int flags,
    const char* identifier,
    char* tags,
    YR_STRING* strings,
    YR_META* metas);


YR_STRING* yr_parser_reduce_string_declaration(
    yyscan_t yyscanner,
    int flags,
    const char* identifier,
    SIZED_STRING* str);


YR_META* yr_parser_reduce_meta_declaration(
    yyscan_t yyscanner,
    int32_t type,
    const char* identifier,
    const char* string,
    int32_t integer);


int yr_parser_reduce_string_identifier(
    yyscan_t yyscanner,
    const char* identifier,
    int8_t instruction,
    uint64_t at_offset);


int yr_parser_emit_pushes_for_strings(
    yyscan_t yyscanner,
    const char* identifier);


int yr_parser_reduce_external(
    yyscan_t yyscanner,
    const char* identifier,
    int8_t intruction);


int yr_parser_reduce_import(
    yyscan_t yyscanner,
    SIZED_STRING* module_name);


int yr_parser_reduce_operation(
    yyscan_t yyscanner,
    const char* operation,
    EXPRESSION left_operand,
    EXPRESSION right_operand);

#endif
