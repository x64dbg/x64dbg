/*
 * $Revision: 2618 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 15:59:09 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Basic declarations, included by all source files.
 *
 * \author Carsten Gutwenger
 *
 * \par License:
 * This file is part of the Open Graph Drawing Framework (OGDF).
 *
 * \par
 * Copyright (C)<br>
 * See README.txt in the root directory of the OGDF installation for details.
 *
 * \par
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * Version 2 or 3 as published by the Free Software Foundation;
 * see the file LICENSE.txt included in the packaging of this file
 * for details.
 *
 * \par
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * \see  http://www.gnu.org/copyleft/gpl.html
 ***************************************************************/


#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_BASIC_H
#define OGDF_BASIC_H


/**
 * \mainpage The Open Graph Drawing Framework
 *
 * \section sec_intro Introduction
 * The Open Graph Drawing Framework (OGDF) is a C++ library containing
 * implementations of various graph drawing algorithms. The library is self
 * contained; optionally, additional packages like LP-solvers are required
 * for some implementations.
 *
 * Here, you find the library's code documentation. For more general information
 * on OGDF see http://www.ogdf.net. There, you can also find further explanations,
 * how-tos, and example code.
 *
 * The OGDF project is a cooperation between
 * - [Chair of Algorithm Engineering](http://ls11-www.cs.uni-dortmund.de/), Faculty of Computer Science, TU Dortmund, Germany
 * - [Juniorprofessorship of Algorithm Engineering](http://www.ae.uni-jena.de/), Faculty of Mathematics and Computer Science, Friedrich-Schiller-University Jena, Germany
 * - [Chair of Prof. J&uuml;nger](http://www.informatik.uni-koeln.de/ls_juenger/), Department of Computer Science, University of Cologne, Germany
 * - [University of Sydney](http://sydney.edu.au/engineering/it/), Australia
 * - [oreas GmbH](http://www.oreas.com/), Cologne, Germany
 */


//---------------------------------------------------------
// detection of the system
//---------------------------------------------------------

#if defined(unix) || defined(__unix__) || defined(__unix) || defined(_AIX) || defined(__APPLE__)
#define OGDF_SYSTEM_UNIX
#endif

#if defined(__WIN32__) || defined(_WIN32) || defined(__NT__)
#define OGDF_SYSTEM_WINDOWS
#endif

// Note: Apple OS X machines will be both OGDF_SYSTEM_UNIX and OGDF_SYSTEM_OSX
#if defined(__APPLE__)
#define OGDF_SYSTEM_OSX
#endif


#if defined(USE_COIN) || defined(OGDF_OWN_LPSOLVER)
#define OGDF_LP_SOLVER
#endif

#if defined(USE_COIN) && !defined(COIN_OSI_CPX) && !defined(COIN_OSI_SYM) && !defined(COIN_OSI_CLP)
#error "Compiler-flag USE_COIN requires an additional COIN_OSI_xxx-flag to choose the LP solver backend."
#endif


// define minimal MS runtime version for mingw32
#if defined(__MINGW32__) && !defined(__MINGW64__)
#ifndef __MSVCRT_VERSION__
#define __MSVCRT_VERSION__ 0x0700
#endif
#endif

// include windows.h on Windows systems
#if defined(OGDF_SYSTEM_WINDOWS) || defined(__CYGWIN__)
#define WIN32_EXTRA_LEAN
#define WIN32_LEAN_AND_MEAN
#undef NOMINMAX
#define NOMINMAX
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0500
#endif
#include <windows.h>
#endif


//---------------------------------------------------------
// assertions
//---------------------------------------------------------

#ifdef OGDF_DEBUG
#include <assert.h>
#define OGDF_ASSERT(expr) assert(expr);
#define OGDF_ASSERT_IF(minLevel,expr) \
    if (int(ogdf::debugLevel) >= int(minLevel)) { assert(expr); } else { }
#define OGDF_SET_DEBUG_LEVEL(level) ogdf::debugLevel = level;

#else
#define OGDF_ASSERT(expr)
#define OGDF_ASSERT_IF(minLevel,expr)
#define OGDF_SET_DEBUG_LEVEL(level)
#endif


//---------------------------------------------------------
// macros for optimization
//---------------------------------------------------------

// Visual C++ compiler
#ifdef _MSC_VER

#define OGDF_LIKELY(x)    (x)
#define OGDF_UNLIKELY(x)  (x)

#ifdef OGDF_DEBUG
#define OGDF_NODEFAULT    default: assert(0);
#else
#define OGDF_NODEFAULT    default: __assume(0);
#endif

#define OGDF_DECL_ALIGN(b) __declspec(align(b))
#define OGDF_DECL_THREAD __declspec(thread)


// GNU gcc compiler (also Intel compiler)
#elif defined(__GNUC__)
//// make sure that SIZE_MAX gets defined
//#define __STDC_LIMIT_MACROS
//#include <stdint.h>

#define OGDF_LIKELY(x)    __builtin_expect((x),1)
#define OGDF_UNLIKELY(x)  __builtin_expect((x),0)
#define OGDF_NODEFAULT    default: ;

#define OGDF_DECL_ALIGN(b) __attribute__ ((aligned(b)))
#define OGDF_DECL_THREAD __thread


// other compiler
#else
#define OGDF_LIKELY(x)    (x)
#define OGDF_UNLIKELY(x)  (x)
#define OGDF_NODEFAULT

#define OGDF_DECL_ALIGN(b)
#endif

#ifndef __SIZEOF_POINTER__
#ifdef _M_X64
#define __SIZEOF_POINTER__ 8
#else
#define __SIZEOF_POINTER__ 4
#endif
#endif


#if defined(__CYGWIN__) || defined(__APPLE__) || defined(__sparc__)
#define OGDF_NO_COMPILER_TLS
#elif defined(__GNUC__)
#if __GNUC__ < 4
#define OGDF_NO_COMPILER_TLS
#endif
#endif


//---------------------------------------------------------
// macros for compiling OGDF as DLL
//---------------------------------------------------------

#ifdef OGDF_SYSTEM_WINDOWS
#ifdef OGDF_DLL

#ifdef OGDF_INSTALL
#define OGDF_EXPORT __declspec(dllexport)
#else
#define OGDF_EXPORT __declspec(dllimport)
#endif

#else
#define OGDF_EXPORT
#endif

#else
#define OGDF_EXPORT
#endif


//---------------------------------------------------------
// define data types with known size
//---------------------------------------------------------

#if defined(_MSC_VER)

typedef unsigned __int8  __uint8;
typedef unsigned __int16 __uint16;
typedef unsigned __int32 __uint32;
typedef unsigned __int64 __uint64;

#else

#undef __int8
#undef __int16
#undef __int32
#undef __int64

typedef signed char        __int8;
typedef short              __int16;
typedef int                __int32;
typedef long long          __int64;
typedef unsigned char      __uint8;
typedef unsigned short     __uint16;
typedef unsigned int       __uint32;
typedef unsigned long long __uint64;
#endif


//---------------------------------------------------------
// common includes
//---------------------------------------------------------

#include <iostream>
#include <fstream>
#include <algorithm>

using std::ios;
using std::istream;
using std::ifstream;
using std::ostream;
using std::ofstream;
using std::cin;
using std::cout;
using std::cerr;
using std::endl;
using std::flush;
using std::swap;
using std::min;
using std::max;

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <math.h>

#ifdef OGDF_SYSTEM_UNIX
#include <stdint.h>
#endif
// make sure that SIZE_MAX gets defined
#ifndef SIZE_MAX
#define SIZE_MAX ((size_t)-1)
#endif


#include <ogdf/basic/exceptions.h>
#include <ogdf/basic/memory.h>
#include <ogdf/basic/comparer.h>



//---------------------------------------------------------
// compiler adaptions
//---------------------------------------------------------

#ifdef _MSC_VER

// disable useless warnings

// missing dll-interface
#pragma warning(disable:4251)
#pragma warning(disable:4275)

#endif


//! The namespace for all OGDF objects.
namespace ogdf
{

#ifndef OGDF_DLL

/**
 *  The class Initialization is used for initializing global variables.
 *  You should never create instances of it!
*/
class Initialization
{
    static int s_count;

public:
    Initialization();
    ~Initialization();
};

static Initialization s_ogdfInitializer;

#endif


//---------------------------------------------------------
// global basic functions
//---------------------------------------------------------

// forward declarations
template<class E> class List;
class OGDF_EXPORT String;


enum Direction { before, after };

//! Returns random integer between low and high (including).
inline int randomNumber(int low, int high)
{
#if RAND_MAX == 32767
    // We get only 15 random bits on some systems (Windows, Solaris)!
    int r1 = (rand() & ((1 << 16) - 1));
    int r2 = (rand() & ((1 << 16) - 1));
    int r = (r1 << 15) | r2;
#else
    int r = rand();
#endif
    return low + (r % (high - low + 1));
}

//! Returns random double value between low and high.
inline double randomDouble(double low, double high)
{
    double val = low + (rand() * (high - low)) / RAND_MAX;
    OGDF_ASSERT(val >= low && val <= high);
    return val;
}

//! Returns a random double value from the normal distribution
//! with mean m and standard deviation sd
inline double randomDoubleNormal(double m, double sd)
{
    double x1, x2, y1, w, rndVal;

    do
    {
        rndVal = randomDouble(0, 1);
        x1 = 2.0 * rndVal - 1.0;
        rndVal = randomDouble(0, 1);
        x2 = 2.0 * rndVal - 1.0;
        w = x1 * x1 + x2 * x2;
    }
    while(w >= 1.0);

    w = sqrt((-2.0 * log(w)) / w) ;
    y1 = x1 * w;

    return(m + y1 * sd);
}



//! Returns used CPU time from T to current time and assigns
//! current time to T.
OGDF_EXPORT double usedTime(double & T);

//! \a doDestruction() returns false if a data type does not require to
//! call its destructor (e.g. build-in data types).
template<class E>inline bool doDestruction(const E*)
{
    return true;
}

// specializations
template<>inline bool doDestruction(const char*)
{
    return false;
}
template<>inline bool doDestruction<int>(const int*)
{
    return false;
}
template<>inline bool doDestruction<double>(const double*)
{
    return false;
}


//---------------------------------------------------------
// handling files and directories
//---------------------------------------------------------

//! The type of an entry in a directory.
enum FileType
{
    ftEntry,     /**< file or directory */
    ftFile,      /**< file */
    ftDirectory  /**< directory */
};

//! Returns true iff \a fileName is a regular file (not a directory).
OGDF_EXPORT bool isFile(const char* fileName);

//! Returns true iff \a fileName is a directory.
OGDF_EXPORT bool isDirectory(const char* fileName);

//! Changes current directory to \a dirName; returns true if successful.
OGDF_EXPORT bool changeDir(const char* dirName);

//! Returns in \a files the list of files in directory \a dirName.
/** The optional argument \a pattern can be used to filter files.
 *
 *  \pre \a dirName is a directory
 */
OGDF_EXPORT void getFiles(const char* dirName,
                          List<String> & files,
                          const char* pattern = "*");

//! Appends to \a files the list of files in directory \a dirName.
/** The optional argument \a pattern can be used to filter files.
 *
 *  \pre \a dirName is a directory
 */
OGDF_EXPORT void getFilesAppend(const char* dirName,
                                List<String> & files,
                                const char* pattern = "*");


//! Returns in \a subdirs the list of directories contained in directory \a dirName.
/** The optional argument \a pattern can be used to filter files.
 *
 *  \pre \a dirName is a directory
 */
OGDF_EXPORT void getSubdirs(const char* dirName,
                            List<String> & subdirs,
                            const char* pattern = "*");

//! Appends to \a subdirs the list of directories contained in directory \a dirName.
/** The optional argument \a pattern can be used to filter files.
 *
 *  \pre \a dirName is a directory
 */
OGDF_EXPORT void getSubdirsAppend(const char* dirName,
                                  List<String> & subdirs,
                                  const char* pattern = "*");


//! Returns in \a entries the list of all entries contained in directory \a dirName.
/** Entries may be files or directories. The optional argument \a pattern
 *  can be used to filter files.
 *
 *  \pre \a dirName is a directory
 */
OGDF_EXPORT void getEntries(const char* dirName,
                            List<String> & entries,
                            const char* pattern = "*");

//! Appends to \a entries the list of all entries contained in directory \a dirName.
/** Entries may be files or directories. The optional argument \a pattern
 *  can be used to filter files.
 *
 *  \pre \a dirName is a directory
 */
OGDF_EXPORT void getEntriesAppend(const char* dirName,
                                  List<String> & entries,
                                  const char* pattern = "*");


//! Returns in \a entries the list of all entries of type \a t contained in directory \a dirName.
/** The optional argument \a pattern can be used to filter files.
 *
 *  \pre \a dirName is a directory
 */
OGDF_EXPORT void getEntries(const char* dirName,
                            FileType t,
                            List<String> & entries,
                            const char* pattern = "*");

//! Appends to \a entries the list of all entries of type \a t contained in directory \a dirName.
/** The optional argument \a pattern can be used to filter files.
 *
 *  \pre \a dirName is a directory
 */
OGDF_EXPORT void getEntriesAppend(const char* dirName,
                                  FileType t,
                                  List<String> & entries,
                                  const char* pattern = "*");

//---------------------------------------------------------
// handling markup formatting
//---------------------------------------------------------

#ifdef OGDF_DEBUG
/** We maintain a debug level in debug versions indicating how many
 *  internal checks (usually assertions) are done.
 *  Usage: Set the variable ogdf::debugLevel using the macro
 *   OGDF_SET_DEBUG_LEVEL(level) to the desired level
 *   in the calling code (e.g. main()). The debugLevel can be set
 *   to a higher level for critical parts (e.g., where you assume a bug)
 *   ensuring that other parts are not too slow.
 */
enum DebugLevel
{
    dlMinimal, dlExtendedChecking, dlConsistencyChecks, dlHeavyChecks
};
extern DebugLevel debugLevel;
#endif


//! Abstract base class for bucket functions.
/**
 * The parameterized class \a BucketFunc<E> is an abstract base class
 * for bucket functions. Derived classes have to implement \a getBucket().
 * Bucket functions are used by bucket sort functions for container types.
 */
template<class E> class BucketFunc
{
public:
    virtual ~BucketFunc() { }

    //! Returns the bucket of \a x.
    virtual int getBucket(const E & x) = 0;
};



#if _MSC_VER >= 1400

inline int sprintf(char* buffer, size_t sizeOfBuffer, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    return vsprintf_s(buffer, sizeOfBuffer, format, args);
}

inline int vsprintf(char* buffer, size_t sizeInBytes, const char* format, va_list argptr)
{
    return vsprintf_s(buffer, sizeInBytes, format, argptr);
}

inline int strcat(char* strDest, size_t sizeOfDest, const char* strSource)
{
    return (int)strcat_s(strDest, sizeOfDest, strSource);
}

inline int strcpy(char* strDest, size_t sizeOfDest, const char* strSource)
{
    return (int)strcpy_s(strDest, sizeOfDest, strSource);
}

inline int strncpy(char* strDest, size_t sizeOfDest, const char* strSource, size_t count)
{
    return (int)strncpy_s(strDest, sizeOfDest, strSource, count);
}

inline char* strtok(char* strToken, const char* strDelimit)
{
    //provide a persistent context pointer for strtok_s
    static char* context;
    return strtok_s(strToken, strDelimit, &context);
}

#define scanf scanf_s
#define fscanf fscanf_s
#define sscanf sscanf_s

inline FILE* fopen(const char* filename, const char* mode)
{
    FILE* stream;
    if(fopen_s(&stream, filename, mode)) stream = 0;
    return stream;
}

inline int localtime(struct tm* ptm, const time_t* timer)
{
    return (int)localtime_s(ptm, timer);
}

#else ///////////////////////////////////////////////////////////////////////////////

inline int sprintf(char* buffer, size_t, const char* format, ...)
{
    va_list args;
    va_start(args, format);

    return ::vsprintf(buffer, format, args);
}


inline int vsprintf(char* buffer, size_t, const char* format, va_list argptr)
{
    return ::vsprintf(buffer, format, argptr);
}


inline int strcat(char* strDest, size_t, const char* strSource)
{
    ::strcat(strDest, strSource);
    return 0;
}

inline int strcpy(char* strDest, size_t, const char* strSource)
{
    ::strcpy(strDest, strSource);
    return 0;
}

inline int strncpy(char* strDest, size_t, const char* strSource, size_t count)
{
    ::strncpy(strDest, strSource, count);
    return 0;
}

inline int localtime(struct tm* ptm, const time_t* timer)
{
    struct tm* newtime = ::localtime(timer);
    if(newtime)
    {
        *ptm = *newtime;
        return 0;
    }
    return 1; // indicates error
}

#endif

} // end namespace ogdf


#endif
