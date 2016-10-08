/*
Copyright (c) 2007-2015. The YARA Authors. All Rights Reserved.

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

#ifndef YR_FILEMAP_H
#define YR_FILEMAP_H

#ifdef _MSC_VER
#define off_t              int64_t
#else
#include <sys/types.h>
#endif

#if defined(_WIN32) || defined(__CYGWIN__)
#include <windows.h>
#define YR_FILE_DESCRIPTOR    HANDLE
#else
#define YR_FILE_DESCRIPTOR    int
#endif

#include <stdlib.h>

#include "integers.h"
#include "utils.h"


typedef struct _YR_MAPPED_FILE
{
    YR_FILE_DESCRIPTOR  file;
    size_t              size;
    uint8_t*            data;
#if defined(_WIN32) || defined(__CYGWIN__)
    HANDLE              mapping;
#endif

} YR_MAPPED_FILE;


YR_API int yr_filemap_map(
    const char* file_path,
    YR_MAPPED_FILE* pmapped_file);


YR_API int yr_filemap_map_fd(
    YR_FILE_DESCRIPTOR file,
    off_t offset,
    size_t size,
    YR_MAPPED_FILE* pmapped_file);


YR_API int yr_filemap_map_ex(
    const char* file_path,
    off_t offset,
    size_t size,
    YR_MAPPED_FILE* pmapped_file);


YR_API void yr_filemap_unmap(
    YR_MAPPED_FILE* pmapped_file);


YR_API void yr_filemap_unmap_fd(
    YR_MAPPED_FILE* pmapped_file);

#endif
