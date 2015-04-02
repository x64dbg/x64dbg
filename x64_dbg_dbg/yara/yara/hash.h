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

#ifndef YR_HASH_H
#define YR_HASH_H


typedef struct _YR_HASH_TABLE_ENTRY
{
  char* key;
  char* ns;
  void* value;

  struct _YR_HASH_TABLE_ENTRY* next;

} YR_HASH_TABLE_ENTRY;


typedef struct _YR_HASH_TABLE
{
  int size;

  YR_HASH_TABLE_ENTRY* buckets[1];

} YR_HASH_TABLE;


typedef int (*YR_HASH_TABLE_FREE_VALUE_FUNC)(void* value);


int yr_hash_table_create(
    int size,
    YR_HASH_TABLE** table);


void yr_hash_table_destroy(
    YR_HASH_TABLE* table,
    YR_HASH_TABLE_FREE_VALUE_FUNC free_value);


void* yr_hash_table_lookup(
    YR_HASH_TABLE* table,
    const char* key,
    const char* ns);


int yr_hash_table_add(
    YR_HASH_TABLE* table,
    const char* key,
    const char* ns,
    void* value);

#endif
