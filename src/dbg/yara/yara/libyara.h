/*
Copyright (c) 2014. The YARA Authors. All Rights Reserved.

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

#ifndef YR_LIBYARA_H
#define YR_LIBYARA_H

#include "utils.h"

#define YR_MAJOR_VERSION   3
#define YR_MINOR_VERSION   6
#define YR_MICRO_VERSION   0

#define version_str(s) _version_str(s)
#define _version_str(s) #s

// Version as a string
#define YR_VERSION version_str(YR_MAJOR_VERSION) \
    "." version_str(YR_MINOR_VERSION) \
    "." version_str(YR_MICRO_VERSION)

// Version as a single 4-byte hex number, e.g. 0x030401 == 3.4.1.
#define YR_VERSION_HEX ((YR_MAJOR_VERSION << 16) | \
    (YR_MINOR_VERSION << 8) | \
    (YR_MICRO_VERSION << 0))


// Enumerated type listing configuration options
typedef enum _YR_CONFIG_NAME
{
    YR_CONFIG_STACK_SIZE,
    YR_CONFIG_MAX

} YR_CONFIG_NAME;


#define DEFAULT_STACK_SIZE 16384


YR_API int yr_initialize(void);


YR_API int yr_finalize(void);


YR_API void yr_finalize_thread(void);


YR_API int yr_get_tidx(void);


YR_API void yr_set_tidx(int);


YR_API int yr_set_configuration(YR_CONFIG_NAME, void*);


YR_API int yr_get_configuration(YR_CONFIG_NAME, void*);

#endif
