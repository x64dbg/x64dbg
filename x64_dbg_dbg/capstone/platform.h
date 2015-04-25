/* Capstone Disassembly Engine */
/* By Axel Souchet & Nguyen Anh Quynh, 2014 */

// handle C99 issue (for pre-2013 VisualStudio)
#ifndef CAPSTONE_PLATFORM_H
#define CAPSTONE_PLATFORM_H

#if !defined(__MINGW32__) && !defined(__MINGW64__) && (defined (WIN32) || defined (WIN64) || defined (_WIN32) || defined (_WIN64))
// MSVC

// stdbool.h
#if (_MSC_VER < 1800)
#ifndef __cplusplus
typedef unsigned char bool;
#define false 0
#define true 1
#endif

#else
// VisualStudio 2013+ -> C99 is supported
#include <stdbool.h>
#endif

#else // not MSVC -> C99 is supported
#include <stdbool.h>
#endif

#endif
