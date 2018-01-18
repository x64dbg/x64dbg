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

#ifndef YR_ENDIAN_H
#define YR_ENDIAN_H

#include <yara/integers.h>


#if defined(__has_builtin)
#  if __has_builtin(__builtin_bswap16)
#    define yr_bswap16(x) __builtin_bswap16(x)
#  endif
#endif

#if !defined(yr_bswap16) && defined(_MSC_VER)
#  define yr_bswap16(x) _byteswap_ushort(x)
#endif

#if !defined(yr_bswap16)
uint16_t _yr_bswap16(uint16_t x);
# define yr_bswap16(x) _yr_bswap16(x)
#endif


#if defined(__has_builtin)
#  if __has_builtin(__builtin_bswap32)
#    define yr_bswap32(x) __builtin_bswap32(x)
#  endif
#endif

#if !defined(yr_bswap32) && defined(_MSC_VER)
#  define yr_bswap32(x) _byteswap_ulong(x)
#endif

#if !defined(yr_bswap32)
uint32_t _yr_bswap32(uint32_t x);
#define yr_bswap32(x) _yr_bswap32(x)
#endif


#if defined(__has_builtin)
#  if __has_builtin(__builtin_bswap64)
#    define yr_bswap64(x) __builtin_bswap64(x)
#  endif
#endif

#if !defined(yr_bswap64) && defined(_MSC_VER)
#  define yr_bswap64(x) _byteswap_uint64(x)
#endif

#if !defined(yr_bswap64)
uint64_t _yr_bswap64(uint64_t x);
#define yr_bswap64(x) _yr_bswap64(x)
#endif


#if defined(WORDS_BIGENDIAN)
#define yr_le16toh(x) yr_bswap16(x)
#define yr_le32toh(x) yr_bswap32(x)
#define yr_le64toh(x) yr_bswap64(x)
#define yr_be16toh(x) (x)
#define yr_be32toh(x) (x)
#define yr_be64toh(x) (x)
#else
#define yr_le16toh(x) (x)
#define yr_le32toh(x) (x)
#define yr_le64toh(x) (x)
#define yr_be16toh(x) yr_bswap16(x)
#define yr_be32toh(x) yr_bswap32(x)
#define yr_be64toh(x) yr_bswap64(x)
#endif

#endif
