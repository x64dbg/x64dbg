/*
Copyright (c) 2007. Victor M. Alvarez [plusvic@gmail.com].

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

#include "re.h"

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

#define yyparse         hex_yyparse
#define yylex           hex_yylex
#define yyerror         hex_yyerror
#define yyfatal         hex_yyfatal
#define yychar          hex_yychar
#define yydebug         hex_yydebug
#define yynerrs         hex_yynerrs
#define yyget_extra     hex_yyget_extra
#define yyget_lineno    hex_yyget_lineno


#ifndef YY_TYPEDEF_YY_SCANNER_T
#define YY_TYPEDEF_YY_SCANNER_T
typedef void* yyscan_t;
#endif

#define YY_EXTRA_TYPE RE*
#define YY_USE_CONST


typedef struct _HEX_LEX_ENVIRONMENT
{
    int token_count;
    int inside_or;
    int last_error_code;
    char last_error_message[256];

} HEX_LEX_ENVIRONMENT;


#define YY_FATAL_ERROR(msg) hex_yyfatal(yyscanner, msg)

#define LEX_ENV  ((HEX_LEX_ENVIRONMENT*) lex_env)

#include <hex_grammar.h>

#define YY_DECL int hex_yylex \
    (YYSTYPE * yylval_param , yyscan_t yyscanner, HEX_LEX_ENVIRONMENT* lex_env)


YY_EXTRA_TYPE yyget_extra(
    yyscan_t yyscanner);

int yylex(
    YYSTYPE* yylval_param,
    yyscan_t yyscanner,
    HEX_LEX_ENVIRONMENT* lex_env);

int yyparse(
    void* yyscanner,
    HEX_LEX_ENVIRONMENT* lex_env);

void yyerror(
    yyscan_t yyscanner,
    HEX_LEX_ENVIRONMENT* lex_env,
    const char* error_message);

void yyfatal(
    yyscan_t yyscanner,
    const char* error_message);

int yr_parse_hex_string(
    const char* hex_string,
    int flags,
    RE** re,
    RE_ERROR* error);
