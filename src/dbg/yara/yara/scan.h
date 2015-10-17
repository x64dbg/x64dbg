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

#ifndef YR_SCAN_H
#define YR_SCAN_H

#include "types.h"

#define SCAN_FLAGS_FAST_MODE         1
#define SCAN_FLAGS_PROCESS_MEMORY    2


int yr_scan_verify_match(
    YR_AC_MATCH* ac_match,
    uint8_t* data,
    size_t data_size,
    size_t data_base,
    size_t offset,
    YR_ARENA* matches_arena,
    int flags);

#endif
