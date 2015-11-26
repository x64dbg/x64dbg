/*
Copyright (c) 2015. The YARA Authors. All Rights Reserved.

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
