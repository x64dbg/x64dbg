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


#ifndef YR_UTILS_H
#define YR_UTILS_H

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#ifdef __cplusplus
#define YR_API extern "C" __declspec(dllimport)
#else
#define YR_API
#endif

#ifndef min
#define min(x, y) ((x < y) ? (x) : (y))
#endif

#ifndef max
#define max(x, y) ((x > y) ? (x) : (y))
#endif


#define PTR_TO_UINT64(x)  ((uint64_t) (size_t) x)


#ifdef NDEBUG

#define assertf(expr, msg)  ((void)0)

#else

#include <stdlib.h>

#define assertf(expr, msg, ...) \
    if(!(expr)) { \
      fprintf(stderr, "%s:%d: " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
      abort(); \
    }

#endif

#endif
