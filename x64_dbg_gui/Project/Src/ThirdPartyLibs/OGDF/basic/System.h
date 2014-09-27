/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Decalration of System class which provides unified
 *        access to system information.
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

#ifndef OGDF_SYSTEM_H
#define OGDF_SYSTEM_H


#include <ogdf/basic/basic.h>
#if defined(OGDF_SYSTEM_OSX)
#include <stdlib.h>
#elif defined(OGDF_SYSTEM_UNIX) || defined(__MINGW32__)
#include <malloc.h>
#endif

// detect processor architecture we're compiling for
//
// OGDF_ARCH_X86       Intel / AMD x86 32-bit processors
// OGDF_ARCH_X64       Intel / AMD x86 64-bit processors
// OGDF_ARCH_IA64      Intel Itanium
// OGDF_ARCH_PPC       PowerPC
// OGDF_ARCH_SPARC     SUN SPARC
// OGDF_ARCH_SPARC_V9  SUN SPARC V9

#if defined(_M_X64) || defined(__x86_64__)
#define OGDF_ARCH_X64
#elif defined(_M_IX86) || defined(__i386__)
#define OGDF_ARCH_X86
#elif defined(_M_IA64) || defined(__ia64__)
#define OGDF_ARCH_IA64
#elif defined(_M_MPPC) || defined(_M_PPC) || defined(__powerpc__)
#define OGDF_ARCH_PPC
#elif defined(__sparc__)
#define OGDF_ARCH_SPARC
#elif defined(__sparc_v9__)
#define OGDF_ARCH_SPARC_V9
#endif

// use SSE2 always if x64-platform or compiler option is set
#ifndef OGDF_USE_SSE2
#if defined(OGDF_ARCH_X64)
#define OGDF_USE_SSE2
#elif defined(_MSC_VER)
#if _M_IX86_FP >= 2
#define OGDF_USE_SSE2
#endif
#endif
#endif

// macros to check for using special cpu features
//
// OGDF_CHECK_SSE2   returns true if SSE2 can be used

#ifdef OGDF_USE_SSE2
#define OGDF_CHECK_SSE2 true
#elif defined(OGDF_ARCH_X86)
#define OGDF_CHECK_SSE2 ogdf::System::cpuSupports(ogdf::cpufSSE2)
#else
#define OGDF_USE_SSE2 false
#endif

// work-around for MinGW-w64
#ifdef __MINGW64__
#ifndef _aligned_free
#define _aligned_free(a) __mingw_aligned_free(a)
#endif
#ifndef _aligned_malloc
#define _aligned_malloc(a,b) __mingw_aligned_malloc(a,b)
#endif
#endif


namespace ogdf
{

//! Special features supported by a x86/x64 CPU.
/**
 * This enumeration is used to specify spcial additional features that
 * are supported by the CPU, in particular extended instruction sets
 * such as SSE.
 */
enum CPUFeature
{
    cpufMMX,    //!< Intel MMX Technology
    cpufSSE,    //!< Streaming SIMD Extensions (SSE)
    cpufSSE2,   //!< Streaming SIMD Extensions 2 (SSE2)
    cpufSSE3,   //!< Streaming SIMD Extensions 3 (SSE3)
    cpufSSSE3,  //!< Supplemental Streaming SIMD Extensions 3 (SSSE3)
    cpufSSE4_1, //!< Streaming SIMD Extensions 4.1 (SSE4.1)
    cpufSSE4_2, //!< Streaming SIMD Extensions 4.2 (SSE4.2)
    cpufVMX,    //!< Virtual Machine Extensions
    cpufSMX,    //!< Safer Mode Extensions
    cpufEST,    //!< Enhanced Intel SpeedStep Technology
    cpufMONITOR //!< Processor supports MONITOR/MWAIT instructions
};

//! Bit mask for CPU features.
enum CPUFeatureMask
{
    cpufmMMX     = 1 << cpufMMX,    //!< Intel MMX Technology
    cpufmSSE     = 1 << cpufSSE,    //!< Streaming SIMD Extensions (SSE)
    cpufmSSE2    = 1 << cpufSSE2,   //!< Streaming SIMD Extensions 2 (SSE2)
    cpufmSSE3    = 1 << cpufSSE3,   //!< Streaming SIMD Extensions 3 (SSE3)
    cpufmSSSE3   = 1 << cpufSSSE3,  //!< Supplemental Streaming SIMD Extensions 3 (SSSE3)
    cpufmSSE4_1  = 1 << cpufSSE4_1, //!< Streaming SIMD Extensions 4.1 (SSE4.1)
    cpufmSSE4_2  = 1 << cpufSSE4_2, //!< Streaming SIMD Extensions 4.2 (SSE4.2)
    cpufmVMX     = 1 << cpufVMX,    //!< Virtual Machine Extensions
    cpufmSMX     = 1 << cpufSMX,    //!< Safer Mode Extensions
    cpufmEST     = 1 << cpufEST,    //!< Enhanced Intel SpeedStep Technology
    cpufmMONITOR = 1 << cpufMONITOR //!< Processor supports MONITOR/MWAIT instructions
};


//! %System specific functionality.
/**
 * The class System encapsulates system specific functions
 * providing unified access across different operating systems.
 * The provided functionality includes:
 *   - Access to file system functionality (listing directories etc.).
 *   - Query memory usage.
 *   - Access to high-perfomance counter under Windows and Cygwin.
 *   - Query CPU specific information.
 */
class OGDF_EXPORT System
{

    //friend class ::OgdfInitialization;

public:
    /**
     * @name Memory usage
     * These methods allow to query the amount of physical memory, as well as the
     * current memory usage by both the process and OGDF's internal memory manager.
     */
    //@{

    static void* alignedMemoryAlloc16(size_t size)
    {
        size_t alignment = 16;
#ifdef OGDF_SYSTEM_WINDOWS
        return _aligned_malloc(size, alignment);
#elif defined(OGDF_SYSTEM_OSX)
        // malloc returns 16 byte aligned memory on OS X.
        return malloc(size);
#else
        return memalign(alignment, size);
#endif
    }

    static void alignedMemoryFree(void* p)
    {
#ifdef OGDF_SYSTEM_WINDOWS
        _aligned_free(p);
#else
        free(p);
#endif
    }

    //! Returns the page size of virtual memory (in bytes).
    static int pageSize()
    {
        return s_pageSize;
    }

    //! Returns the total size of physical memory (in bytes).
    static long long physicalMemory();

    //! Returns the size of available (free) physical memory (in bytes).
    static long long availablePhysicalMemory();

    //! Returns the amount of memory (in bytes) allocated by the process.
    static size_t memoryUsedByProcess();

#if defined(OGDF_SYSTEM_WINDOWS) || defined(__CYGWIN__)
    //! Returns the maximal amount of memory (in bytes) used by the process (Windows/Cygwin only).
    static size_t peakMemoryUsedByProcess();
#endif

    //! Returns the amount of memory (in bytes) allocated by OGDF's memory manager.
    /**
     * The memory manager allocates blocks of a fixed size from the system (via malloc())
     * and makes it available in its free lists (for allocating small pieces of memory.
     * The returned value is the total amount of memory allocated from the system;
     * the amount of memory currently allocated from the user is
     * memoryAllocatedByMemoryManager() - memoryInFreelistOfMemoryManager().
     *
     * Keep in mind that the memory manager never releases memory to the system before
     * its destruction.
     */
    static size_t memoryAllocatedByMemoryManager();

    //! Returns the amount of memory (in bytes) contained in the global free list of OGDF's memory manager.
    static size_t memoryInGlobalFreeListOfMemoryManager();

    //! Returns the amount of memory (in bytes) contained in the thread's free list of OGDF's memory manager.
    static size_t memoryInThreadFreeListOfMemoryManager();

    //! Returns the amount of memory (in bytes) allocated on the heap (e.g., with malloc).
    /**
     * This refers to dynamically allocated memory, e.g., memory allocated with malloc()
     * or new.
     */
    static size_t memoryAllocatedByMalloc();

    //! Returns the amount of memory (in bytes) contained in free chunks on the heap.
    /**
     * This refers to memory that has been deallocated with free() or delete, but has not
     * yet been returned to the operating system.
     */
    static size_t memoryInFreelistOfMalloc();

#if defined(OGDF_SYSTEM_WINDOWS) || defined(__CYGWIN__)
    //@}
    /**
     * @name Measuring time
     * These methods provide various ways to measure time. The high-performance
     * counter (Windows and Cygwin only) can be used to measure real time
     * periods with a better resolution than the standard system time function.
     */
    //@{

    //! Returns the current value of the high-performance counter in \a counter.
    static void getHPCounter(LARGE_INTEGER & counter);

    //! Returns the elapsed time (in seconds) between \a startCounter and \a endCounter.
    static double elapsedSeconds(
        const LARGE_INTEGER & startCounter,
        const LARGE_INTEGER & endCounter);
#endif

    //! Returns the elapsed time (in milliseconds) between \a t and now.
    /**
     * The functions sets \a t to to the current time. Usually, you first call
     * usedRealTime(t) to query the start time \a t, and determine the elapsed time
     * after performing some computation by calling usedRealTime(t) again; this time
     * the return value gives you the elapsed time in milliseconds.
     */
    static __int64 usedRealTime(__int64 & t);


    //@}
    /**
     * @name Processor information
     * These methods allow to query information about the current processor such as
     * supported instruction sets (e.g., SSE extensions), cache size, and number of
     * installed processors.
     */
    //@{

    //! Returns the bit vector describing the CPU features supported on current system.
    static int cpuFeatures()
    {
        return s_cpuFeatures;
    }

    //! Returns true if the CPU supports \a feature.
    static bool cpuSupports(CPUFeature feature)
    {
        return (s_cpuFeatures & (1 << feature)) != 0;
    }

    //! Returns the L2-cache size (in KBytes).
    static int cacheSizeKBytes()
    {
        return s_cacheSize;
    }

    //! Returns the number of bytes in a cache line.
    static int cacheLineBytes()
    {
        return s_cacheLine;
    }

    //! Returns the number of processors (cores) available on the current system.
    static int numberOfProcessors()
    {
        return s_numberOfProcessors;
    }

    //@}

private:
    static unsigned int s_cpuFeatures; //!< Supported CPU features.
    static int          s_cacheSize;   //!< Cache size in KBytes.
    static int          s_cacheLine;   //!< Bytes in a cache line.
    static int          s_numberOfProcessors; //!< Number of processors (cores) available.
    static int          s_pageSize;    //!< The page size of virtual memory.

#if defined(OGDF_SYSTEM_WINDOWS) || defined(__CYGWIN__)
    static LARGE_INTEGER s_HPCounterFrequency; //!< Frequency of high-performance counter.
#endif

public:
    //! Static initilization routine (automatically called).
    static void init();
};


} // end namespace ogdf


#endif
