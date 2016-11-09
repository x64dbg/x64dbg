/*
Copyright (c) 2016. The YARA Authors. All Rights Reserved.

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

#ifndef YR_MUTEX_H
#define YR_MUTEX_H



#if defined(_WIN32) || defined(__CYGWIN__)

#include <windows.h>

typedef DWORD YR_THREAD_ID;
typedef DWORD YR_THREAD_STORAGE_KEY;
typedef HANDLE YR_MUTEX;

#else

#include <pthread.h>

typedef pthread_t YR_THREAD_ID;
typedef pthread_key_t YR_THREAD_STORAGE_KEY;
typedef pthread_mutex_t YR_MUTEX;

#endif

YR_THREAD_ID yr_current_thread_id(void);

int yr_mutex_create(YR_MUTEX*);
int yr_mutex_destroy(YR_MUTEX*);
int yr_mutex_lock(YR_MUTEX*);
int yr_mutex_unlock(YR_MUTEX*);

int yr_thread_storage_create(YR_THREAD_STORAGE_KEY*);
int yr_thread_storage_destroy(YR_THREAD_STORAGE_KEY*);
int yr_thread_storage_set_value(YR_THREAD_STORAGE_KEY*, void*);
void* yr_thread_storage_get_value(YR_THREAD_STORAGE_KEY*);

#endif
