/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of memory manager for allocating small
 *        pieces of memory
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

#ifndef OGDF_MALLOC_MEMORY_ALLOCATOR_H
#define OGDF_MALLOC_MEMORY_ALLOCATOR_H


namespace ogdf
{

//! Implements a simple memory manager using \c malloc() and \c free().
class OGDF_EXPORT MallocMemoryAllocator
{
    struct MemElem
    {
        MemElem* m_next;
    };
    typedef MemElem* MemElemPtr;

public:

    MallocMemoryAllocator() { }
    ~MallocMemoryAllocator() { }


    static void init() { }
    static void initThread() { }
    static void cleanup() { }

    static bool checkSize(size_t /* nBytes */)
    {
        return true;
    }

    //! Allocates memory of size \a nBytes.
    static void* allocate(size_t nBytes, const char*, int)
    {
        return allocate(nBytes);
    }

    //! Allocates memory of size \a nBytes.
    static void* allocate(size_t nBytes)
    {
        void* p = malloc(nBytes);
        if(OGDF_UNLIKELY(p == 0)) OGDF_THROW(ogdf::InsufficientMemoryException);
        return p;
    }


    //! Deallocates memory at address \a p which is of size \a nBytes.
    static void deallocate(size_t /* nBytes */, void* p)
    {
        free(p);
    }

    //! Deallocate a complete list starting at \a pHead and ending at \a pTail.
    /**
     * The elements are assumed to be chained using the first word of each element and
     * elements are of size \a nBytes.
     */
    static void deallocateList(size_t /* nBytes */, void* pHead, void* pTail)
    {
        MemElemPtr q, pStop = MemElemPtr(pTail)->m_next;
        while(pHead != pStop)
        {
            q = MemElemPtr(pHead)->m_next;
            free(pHead);
            pHead = q;
        }
    }

    static void flushPool() { }
    static void flushPool(__uint16 /* nBytes */) { }

    //! Always returns 0, since no blocks are allocated.
    static size_t memoryAllocatedInBlocks()
    {
        return 0;
    }

    //! Always returns 0, since no blocks are allocated.
    static size_t memoryInFreelist()
    {
        return 0;
    }
};

} // namespace ogdf

#endif
