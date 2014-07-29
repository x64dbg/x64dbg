/**
 * @file   export.h
 * @author igor.gutnik@gmail.com
 * @date   Mon Sep 22 09:28:54 2008
 *
 * @brief  This file sets things up for C dynamic library function definitions and
 *         static inlined functions
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

#ifndef __BEA_EXPORT_H__
#define __BEA_EXPORT_H__


/*  Set up for C function definitions, even when using C++ */

#ifdef __cplusplus
#define CPP_VISIBLE_BEGIN extern "C" {
#define CPP_VISIBLE_END }
#else
#define CPP_VISIBLE_BEGIN
#define CPP_VISIBLE_END
#endif

#if defined(_MSC_VER)
#pragma warning( disable: 4251 )
#endif

/* Some compilers use a special export keyword */
#ifndef bea__api_export__
# if defined(__BEOS__)
#  if defined(__GNUC__)
#   define bea__api_export__        __declspec(dllexport)
#  else
#   define bea__api_export__        __declspec(export)
#  endif
# elif defined(_WIN32) || defined(_WIN64)
#  ifdef __BORLANDC__
#    define bea__api_export__   __declspec(dllexport)
#    define bea__api_import__     __declspec(dllimport)
#  elif defined(__WATCOMC__)
#    define bea__api_export__    __declspec(dllexport)
#    define bea__api_import__
#  else
#   define bea__api_export__        __declspec(dllexport)
#   define bea__api_import__        __declspec(dllimport)
#  endif
# elif defined(__OS2__)
#  ifdef __WATCOMC__
#    define bea__api_export__    __declspec(dllexport)
#    define bea__api_import__
#  else
#   define bea__api_export__
#   define bea__api_import__
#  endif
# else
#  if defined(_WIN32) && defined(__GNUC__) && __GNUC__ >= 4
#   define bea__api_export__        __attribubea__ ((visibility("default")))
#   define bea__api_import__        __attribubea__ ((visibility("default")))
#  else
#   define bea__api_export__
#   define bea__api_import__
#  endif
# endif
#endif

/* Use C calling convention by default*/

#ifndef __bea_callspec__
#if defined(BEA_USE_STDCALL)
#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#if defined(__BORLANDC__) || defined(__WATCOMC__) || defined(_MSC_VER) || defined(__MINGW32__) || defined(__POCC__)
#define __bea_callspec__     __stdcall
#else
#define __bea_callspec__
#endif
#else
#ifdef __OS2__
#define __bea_callspec__ _System
#else
#define __bea_callspec__
#endif
#endif
#else
#define __bea_callspec__
#endif
#endif

#ifdef __SYMBIAN32__
#    ifndef EKA2
#        undef bea__api_export__
#        undef bea__api_import__
#        define bea__api_export__
#        define bea__api_import__
#    elif !defined(__WINS__)
#        undef bea__api_export__
#        undef bea__api_import__
#        define bea__api_export__ __declspec(dllexport)
#        define bea__api_import__ __declspec(dllexport)
#    endif /* !EKA2 */
#endif /* __SYMBIAN32__ */


#if defined(__GNUC__) && (__GNUC__ > 2)
#define BEA_EXPECT_CONDITIONAL(c)    (__builtin_expect((c), 1))
#define BEA_UNEXPECT_CONDITIONAL(c)  (__builtin_expect((c), 0))
#else
#define BEA_EXPECT_CONDITIONAL(c)    (c)
#define BEA_UNEXPECT_CONDITIONAL(c)  (c)
#endif


/* Set up compiler-specific options for inlining functions */
#ifndef BEA_HAS_INLINE
#if defined(__GNUC__) || defined(__POCC__) || defined(__WATCOMC__) || defined(__SUNPRO_C)
#define BEA_HAS_INLINE
#else
/* Add any special compiler-specific cases here */
#if defined(_MSC_VER) || defined(__BORLANDC__) ||    \
  defined(__DMC__) || defined(__SC__) ||        \
  defined(__WATCOMC__) || defined(__LCC__) ||        \
  defined(__DECC) || defined(__EABI__)
#ifndef __inline__
#define __inline__    __inline
#endif
#define BEA_HAS_INLINE
#else
#if !defined(__MRC__) && !defined(_SGI_SOURCE)
#ifndef __inline__
#define __inline__ inline
#endif
#define BEA_HAS_INLINE
#endif /* Not a funky compiler */
#endif /* Visual C++ */
#endif /* GNU C */
#endif /* CACHE_HAS_INLINE */

/* If inlining isn't supported, remove "__inline__", turning static
   inlined functions into static functions (resulting in code bloat
   in all files which include the offending header files)
*/
#ifndef BEA_HAS_INLINE
#define __inline__
#endif

/* fix a bug with gcc under windows */

#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32) || defined(_WIN64)
#if defined(__MINGW32__)
#define const__
#else
#define const__ const
#endif
#else
#define const__ const
#endif



#endif
