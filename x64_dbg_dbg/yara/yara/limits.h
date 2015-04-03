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

#ifndef YR_LIMITS_H
#define YR_LIMITS_H


// MAX_THREADS is the number of threads that can use a YR_RULES
// object simultaneosly. This value is limited by the number of
// bits in tidx_mask.

#define MAX_THREADS 32


#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

#define MAX_COMPILER_ERROR_EXTRA_INFO   256
#define MAX_ATOM_LENGTH                 4
#define MAX_LOOP_NESTING                4
#define MAX_ARENA_PAGES                 32
#define MAX_INCLUDE_DEPTH               16
#define MAX_STRING_MATCHES              1000000
#define MAX_FUNCTION_ARGS               128
#define MAX_FAST_HEX_RE_STACK           300
#define MAX_OVERLOADED_FUNCTIONS        10
#define MAX_HEX_STRING_TOKENS           10000

#define LOOP_LOCAL_VARS                 4
#define STRING_CHAINING_THRESHOLD       200
#define LEX_BUF_SIZE                    1024


#endif
