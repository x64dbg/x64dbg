/*
Copyright (c) 2016. The YARA Authors. All Rights Reserved.

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
