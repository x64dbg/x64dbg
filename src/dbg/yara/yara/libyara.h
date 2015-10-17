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

#ifndef YR_LIBYARA_H
#define YR_LIBYARA_H

#include "utils.h"

#define YR_MAJOR_VERSION   3
#define YR_MINOR_VERSION   3
#define YR_MICRO_VERSION   0

// Version as a string
#define YR_VERSION         "3.3.0"

// Version as a single 4-byte hex number, e.g. 0x030401 == 3.4.1.
#define YR_VERSION_HEX ((YR_MAJOR_VERSION << 16) | \
    (YR_MINOR_VERSION << 8) | \
    (YR_MICRO_VERSION << 0)


YR_API int yr_initialize(void);


YR_API int yr_finalize(void);


YR_API void yr_finalize_thread(void);


YR_API int yr_get_tidx(void);


YR_API void yr_set_tidx(int);

#endif
