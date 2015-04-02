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

#undef yyparse
#undef yylex
#undef yyerror
#undef yyfatal
#undef yychar
#undef yydebug
#undef yynerrs
#undef yyget_extra
#undef yyget_lineno

#undef YY_FATAL_ERROR
#undef YY_DECL
#undef LEX_ENV


#define yyparse         re_yyparse
#define yylex           re_yylex
#define yyerror         re_yyerror
#define yyfatal         re_yyfatal
#define yychar          re_yychar
#define yydebug         re_yydebug
#define yynerrs         re_yynerrs
#define yyget_extra     re_yyget_extra
#define yyget_lineno    re_yyget_lineno


#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

#define YY_EXTRA_TYPE RE*
#define YY_USE_CONST


typedef struct _RE_LEX_ENVIRONMENT
{
    int negated_class;
    uint8_t class_vector[32];
    int last_error_code;
    char last_error_message[256];

} RE_LEX_ENVIRONMENT;


#define LEX_ENV  ((RE_LEX_ENVIRONMENT*) lex_env)

#define YY_FATAL_ERROR(msg) re_yyfatal(yyscanner, msg)

#include <re_grammar.h>

#define YY_DECL int re_yylex \
    (YYSTYPE * yylval_param , yyscan_t yyscanner, RE_LEX_ENVIRONMENT* lex_env)


YY_EXTRA_TYPE yyget_extra(
    yyscan_t yyscanner);

int yylex(
    YYSTYPE* yylval_param,
    yyscan_t yyscanner,
    RE_LEX_ENVIRONMENT* lex_env);

int yyparse(
    void* yyscanner,
    RE_LEX_ENVIRONMENT* lex_env);

void yyerror(
    yyscan_t yyscanner,
    RE_LEX_ENVIRONMENT* lex_env,
    const char* error_message);

void yyfatal(
    yyscan_t yyscanner,
    const char* error_message);

int yr_parse_re_string(
    const char* re_string,
    int flags,
    RE** re,
    RE_ERROR* error);
