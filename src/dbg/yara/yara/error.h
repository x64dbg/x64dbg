/*
Copyright (c) 2014. The YARA Authors. All Rights Reserved.

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

#ifndef YR_ERROR_H
#define YR_ERROR_H

#include <string.h>

#ifndef ERROR_SUCCESS
#define ERROR_SUCCESS                           0
#endif

#define ERROR_INSUFICIENT_MEMORY                1
#define ERROR_COULD_NOT_ATTACH_TO_PROCESS       2
#define ERROR_COULD_NOT_OPEN_FILE               3
#define ERROR_COULD_NOT_MAP_FILE                4
#define ERROR_INVALID_FILE                      6
#define ERROR_CORRUPT_FILE                      7
#define ERROR_UNSUPPORTED_FILE_VERSION          8
#define ERROR_INVALID_REGULAR_EXPRESSION        9
#define ERROR_INVALID_HEX_STRING                10
#define ERROR_SYNTAX_ERROR                      11
#define ERROR_LOOP_NESTING_LIMIT_EXCEEDED       12
#define ERROR_DUPLICATED_LOOP_IDENTIFIER        13
#define ERROR_DUPLICATED_IDENTIFIER             14
#define ERROR_DUPLICATED_TAG_IDENTIFIER         15
#define ERROR_DUPLICATED_META_IDENTIFIER        16
#define ERROR_DUPLICATED_STRING_IDENTIFIER      17
#define ERROR_UNREFERENCED_STRING               18
#define ERROR_UNDEFINED_STRING                  19
#define ERROR_UNDEFINED_IDENTIFIER              20
#define ERROR_MISPLACED_ANONYMOUS_STRING        21
#define ERROR_INCLUDES_CIRCULAR_REFERENCE       22
#define ERROR_INCLUDE_DEPTH_EXCEEDED            23
#define ERROR_WRONG_TYPE                        24
#define ERROR_EXEC_STACK_OVERFLOW               25
#define ERROR_SCAN_TIMEOUT                      26
#define ERROR_TOO_MANY_SCAN_THREADS             27
#define ERROR_CALLBACK_ERROR                    28
#define ERROR_INVALID_ARGUMENT                  29
#define ERROR_TOO_MANY_MATCHES                  30
#define ERROR_INTERNAL_FATAL_ERROR              31
#define ERROR_NESTED_FOR_OF_LOOP                32
#define ERROR_INVALID_FIELD_NAME                33
#define ERROR_UNKNOWN_MODULE                    34
#define ERROR_NOT_A_STRUCTURE                   35
#define ERROR_NOT_INDEXABLE                     36
#define ERROR_NOT_A_FUNCTION                    37
#define ERROR_INVALID_FORMAT                    38
#define ERROR_TOO_MANY_ARGUMENTS                39
#define ERROR_WRONG_ARGUMENTS                   40
#define ERROR_WRONG_RETURN_TYPE                 41
#define ERROR_DUPLICATED_STRUCTURE_MEMBER       42


#define FAIL_ON_ERROR(x) { \
  int result = (x); \
  if (result != ERROR_SUCCESS) \
    return result; \
}

#define FAIL_ON_ERROR_WITH_CLEANUP(x, cleanup) { \
  int result = (x); \
  if (result != ERROR_SUCCESS) { \
    cleanup; \
    return result; \
  } \
}

#define FAIL_ON_COMPILER_ERROR(x) { \
  compiler->last_result = (x); \
  if (compiler->last_result != ERROR_SUCCESS) \
    return compiler->last_result; \
}


#ifdef NDEBUG
#define assertf(expr, msg)  ((void)0)
#else
#define assertf(expr, msg, ...) \
    if(!(expr)) { \
      fprintf(stderr, "%s:%d: " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
      abort(); \
    }
#endif

#endif
