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


#ifndef YR_UTILS_H
#define YR_UTILS_H

#include <limits.h>

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef NULL
#define NULL 0
#endif

#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

#if defined(__GNUC__)
#define YR_API EXTERNC __attribute__((visibility("default")))
#elif defined(_MSC_VER)
#define YR_API EXTERNC __declspec(dllexport)
#else
#define YR_API EXTERNC
#endif

#if defined(__GNUC__)
#define YR_ALIGN(n) __attribute__((aligned(n)))
#elif defined(_MSC_VER)
#define YR_ALIGN(n) __declspec(align(n))
#else
#define YR_ALIGN(n)
#endif

#define yr_min(x, y) ((x < y) ? (x) : (y))
#define yr_max(x, y) ((x > y) ? (x) : (y))

#define yr_swap(x, y, T) do { T temp = x; x = y; y = temp; } while (0)

#ifdef NDEBUG

#define assertf(expr, msg, ...)  ((void)0)

#else

#include <stdlib.h>

#define assertf(expr, msg, ...) \
    if(!(expr)) { \
      fprintf(stderr, "%s:%d: " msg "\n", __FILE__, __LINE__, ##__VA_ARGS__); \
      abort(); \
    }

#endif

// Set, unset, and test bits in an array of unsigned characters by integer
// index. The underlying array must be of type char or unsigned char to
// ensure compatibility with the CHAR_BIT constant used in these definitions.

#define YR_BITARRAY_SET(uchar_array_base, bitnum) \
          (((uchar_array_base)[(bitnum)/CHAR_BIT]) = \
            ((uchar_array_base)[(bitnum)/CHAR_BIT] | (1 << ((bitnum) % CHAR_BIT))))

#define YR_BITARRAY_UNSET(uchar_array_base, bitnum) \
          (((uchar_array_base)[(bitnum)/CHAR_BIT]) = \
            ((uchar_array_base)[(bitnum)/CHAR_BIT] & (~(1 << ((bitnum) % CHAR_BIT)))))

#define YR_BITARRAY_TEST(uchar_array_base, bitnum) \
          (((uchar_array_base)[(bitnum)/CHAR_BIT] & (1 << ((bitnum) % CHAR_BIT))) != 0)

#define YR_BITARRAY_NCHARS(bitnum) \
          (((bitnum)+(CHAR_BIT-1))/CHAR_BIT)

#endif
