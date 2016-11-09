/*
Copyright (c) 2015. The YARA Authors. All Rights Reserved.

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

#ifndef YR_STREAM_H
#define YR_STREAM_H

#include <stddef.h>

typedef size_t (*YR_STREAM_READ_FUNC)(
    void* ptr,
    size_t size,
    size_t count,
    void* user_data);


typedef size_t (*YR_STREAM_WRITE_FUNC)(
    const void* ptr,
    size_t size,
    size_t count,
    void* user_data);


typedef struct _YR_STREAM
{
    void* user_data;

    YR_STREAM_READ_FUNC read;
    YR_STREAM_WRITE_FUNC write;

} YR_STREAM;


size_t yr_stream_read(
    void* ptr,
    size_t size,
    size_t count,
    YR_STREAM* stream);


size_t yr_stream_write(
    const void* ptr,
    size_t size,
    size_t count,
    YR_STREAM* stream);

#endif
