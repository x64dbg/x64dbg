/**
 * @file   basic_types.h
 * @author  <igor.gutnik@gmail.com>
 * @date   Thu Dec 24 19:31:22 2009
 *
 * @brief  Definitions of fixed-size integer types for various platforms
 *
 * This file is part of BeaEngine.
 *
 *    BeaEngine is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU Lesser General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    BeaEngine is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public License
 *    along with BeaEngine.  If not, see <http://www.gnu.org/licenses/>. */

#ifndef __BEA_BASIC_TYPES_HPP__
#define __BEA_BASIC_TYPES_HPP__

#include <stddef.h>

#if defined(__GNUC__) || defined (__INTEL_COMPILER) || defined(__LCC__) || defined(__POCC__)
#include <stdint.h>
#endif

#if defined(_MSC_VER) && !defined(__BORLANDC__)
/*
* Windows/Visual C++
*/
typedef signed char            Int8;
typedef unsigned char          UInt8;
typedef signed short           Int16;
typedef unsigned short         UInt16;
typedef signed int             Int32;
typedef unsigned int           UInt32;
typedef signed __int64         Int64;
typedef unsigned __int64       UInt64;
#if defined(_WIN64)
#define BEA_PTR_IS_64_BIT 1
typedef signed __int64     IntPtr;
typedef unsigned __int64   UIntPtr;
#else
typedef signed long        IntPtr;
typedef size_t             UIntPtr;
#endif
#define BEA_HAVE_INT64 1
#elif defined(__POCC__)
/*
* PellesC
*/
typedef signed char            Int8;
typedef unsigned char          UInt8;
typedef signed short           Int16;
typedef unsigned short         UInt16;
typedef signed int             Int32;
typedef unsigned int           UInt32;
typedef signed long long       Int64;
typedef unsigned long long     UInt64;
#if defined(_WIN64)
#define BEA_PTR_IS_64_BIT 1
typedef signed long long   IntPtr;
typedef unsigned long long UIntPtr;
#else
typedef signed long        IntPtr;
typedef size_t             UIntPtr;
#endif
#define BEA_HAVE_INT64 1
#elif defined(__GNUC__) || defined(__LCC__)
/*
* Unix/GCC
*/
typedef signed char            Int8;
typedef unsigned char          UInt8;
typedef signed short           Int16;
typedef unsigned short         UInt16;
typedef signed int             Int32;
typedef unsigned int           UInt32;
typedef intptr_t               IntPtr;
typedef uintptr_t              UIntPtr;
#if defined(__LP64__)
#define BEA_PTR_IS_64_BIT 1
#define BEA_LONG_IS_64_BIT 1
typedef signed long        Int64;
typedef unsigned long      UInt64;
#else
#if defined (__INTEL_COMPILER) || defined (__ICC) || defined (_ICC)
typedef __int64           Int64;
typedef unsigned __int64  UInt64;
#else
typedef signed long long   Int64;
typedef unsigned long long UInt64;
#endif
#endif
#define BEA_HAVE_INT64 1
#elif defined(__DECCXX)
/*
* Compaq C++
*/
typedef signed char            Int8;
typedef unsigned char          UInt8;
typedef signed short           Int16;
typedef unsigned short         UInt16;
typedef signed int             Int32;
typedef unsigned int           UInt32;
typedef signed __int64         Int64;
typedef unsigned __int64       UInt64;
#if defined(__VMS)
#if defined(__32BITS)
typedef signed long    IntPtr;
typedef unsigned long  UIntPtr;
#else
typedef Int64          IntPtr;
typedef UInt64         UIntPtr;
#define BEA_PTR_IS_64_BIT 1
#endif
#else
typedef signed long        IntPtr;
typedef unsigned long      UIntPtr;
#define BEA_PTR_IS_64_BIT 1
#define BEA_LONG_IS_64_BIT 1
#endif
#define BEA_HAVE_INT64 1
#elif defined(__HP_aCC)
/*
* HP Ansi C++
*/
typedef signed char            Int8;
typedef unsigned char          UInt8;
typedef signed short           Int16;
typedef unsigned short         UInt16;
typedef signed int             Int32;
typedef unsigned int           UInt32;
typedef signed long            IntPtr;
typedef unsigned long          UIntPtr;
#if defined(__LP64__)
#define BEA_PTR_IS_64_BIT 1
#define BEA_LONG_IS_64_BIT 1
typedef signed long        Int64;
typedef unsigned long      UInt64;
#else
typedef signed long long   Int64;
typedef unsigned long long UInt64;
#endif
#define BEA_HAVE_INT64 1
#elif defined(__SUNPRO_CC) || defined(__SUNPRO_C)
/*
* SUN Forte C++
*/
typedef signed char            Int8;
typedef unsigned char          UInt8;
typedef signed short           Int16;
typedef unsigned short         UInt16;
typedef signed int             Int32;
typedef unsigned int           UInt32;
typedef signed long            IntPtr;
typedef unsigned long          UIntPtr;
#if defined(__sparcv9)
#define BEA_PTR_IS_64_BIT 1
#define BEA_LONG_IS_64_BIT 1
typedef signed long        Int64;
typedef unsigned long      UInt64;
#else
typedef signed long long   Int64;
typedef unsigned long long UInt64;
#endif
#define BEA_HAVE_INT64 1
#elif defined(__IBMCPP__)
/*
* IBM XL C++
*/
typedef signed char            Int8;
typedef unsigned char          UInt8;
typedef signed short           Int16;
typedef unsigned short         UInt16;
typedef signed int             Int32;
typedef unsigned int           UInt32;
typedef signed long            IntPtr;
typedef unsigned long          UIntPtr;
#if defined(__64BIT__)
#define BEA_PTR_IS_64_BIT 1
#define BEA_LONG_IS_64_BIT 1
typedef signed long        Int64;
typedef unsigned long      UInt64;
#else
typedef signed long long   Int64;
typedef unsigned long long UInt64;
#endif
#define BEA_HAVE_INT64 1
#elif defined(__BORLANDC__)
/*
* Borland C/C++
*/
typedef signed char            Int8;
typedef unsigned char          UInt8;
typedef signed short           Int16;
typedef unsigned short         UInt16;
typedef signed int             Int32;
typedef unsigned int           UInt32;
typedef unsigned __int64       Int64;
typedef signed __int64         UInt64;
typedef signed long            IntPtr;
typedef unsigned long          UIntPtr;
#define BEA_HAVE_INT64 1
#elif defined(__WATCOMC__)
/*
* Watcom C/C++
*/
typedef signed char            Int8;
typedef unsigned char          UInt8;
typedef signed short           Int16;
typedef unsigned short         UInt16;
typedef signed int             Int32;
typedef unsigned int           UInt32;
typedef unsigned __int64       Int64;
typedef signed __int64         UInt64;
#define BEA_HAVE_INT64 1
typedef size_t                 UIntPtr;
#elif defined(__sgi)
/*
* MIPSpro C++
*/
typedef signed char            Int8;
typedef unsigned char          UInt8;
typedef signed short           Int16;
typedef unsigned short         UInt16;
typedef signed int             Int32;
typedef unsigned int           UInt32;
typedef signed long            IntPtr;
typedef unsigned long          UIntPtr;
#if _MIPS_SZLONG == 64
#define BEA_PTR_IS_64_BIT 1
#define BEA_LONG_IS_64_BIT 1
typedef signed long        Int64;
typedef unsigned long      UInt64;
#else
typedef signed long long   Int64;
typedef unsigned long long UInt64;
#endif
#define BEA_HAVE_INT64 1
#endif

#if defined(_MSC_VER) || defined(__BORLANDC__)
#define W64LIT(x) x##ui64
#else
#define W64LIT(x) x##ULL
#endif


#ifndef C_STATIC_ASSERT
#define C_STATIC_ASSERT(tag_name, x)            \
       typedef int cache_static_assert_ ## tag_name[(x) * 2-1]
#endif

C_STATIC_ASSERT(sizeof_Int8 , (sizeof(Int8)  == 1));
C_STATIC_ASSERT(sizeof_UInt8, (sizeof(UInt8) == 1));

C_STATIC_ASSERT(sizeof_Int16 , (sizeof(Int16)  == 2));
C_STATIC_ASSERT(sizeof_UInt16, (sizeof(UInt16) == 2));

C_STATIC_ASSERT(sizeof_Int32 , (sizeof(Int32)  == 4));
C_STATIC_ASSERT(sizeof_UInt32, (sizeof(UInt32) == 4));

C_STATIC_ASSERT(sizeof_Int64 , (sizeof(Int64)  == 8));
C_STATIC_ASSERT(sizeof_UInt64, (sizeof(UInt64) == 8));

#endif
