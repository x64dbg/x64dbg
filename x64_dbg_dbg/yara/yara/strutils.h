/*
Copyright (c) 2007-2014. The YARA Authors. All Rights Reserved.

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

#ifndef YR_STRUTILS_H
#define YR_STRUTILS_H

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

#include "config.h"

#ifdef _WIN32
#define snprintf _snprintf
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif


uint64_t xtoi(
    const char* hexstr);


#if !HAVE_STRLCPY
size_t strlcpy(
    char* dst,
    const char* src,
    size_t size);
#endif


#if !HAVE_STRLCAT
size_t strlcat(
    char* dst,
    const char* src,
    size_t size);
#endif


#if !HAVE_MEMMEM
void* memmem(
    const void* haystack,
    size_t haystack_size,
    const void* needle,
    size_t needle_size);
#endif


int strlen_w(
    const char* w_str);


int strcmp_w(
    const char* w_str,
    const char* str);


size_t strlcpy_w(
    char* dst,
    const char* w_src,
    size_t n);

#endif

