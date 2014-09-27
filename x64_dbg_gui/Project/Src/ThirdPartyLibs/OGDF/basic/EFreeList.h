/*
 * $Revision: 2579 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-11 15:28:17 +0200 (Mi, 11. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of a simple freelist and an
 * index pool which generates unique indices for elements.
 *
 * \author Martin Gronemann
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

#ifndef OGDF_EFREE_LIST_H
#define OGDF_EFREE_LIST_H

#include <ogdf/basic/EList.h>

namespace ogdf
{

template<typename E, E* E::*next> class EFreeList;
template<typename E, E* E::*next, int E::*index> class EFreeListIndexPool;

template<typename E, E* E::*next> class EFreeListTypes;


//! Simple implementation of a FreeList which buffers the memory allocation of an embedded list item.
template<typename E, E* E::*next>
class EFreeList
{
    friend class EFreeListTypes<E, next>;
public:
    //! Constructs a new freelist
    inline EFreeList()
    {
        EFreeListTypes<E, next>::FreeStack::init(this);
    }

    //! Destructor. Releases the mem used by the remaining elements on the stack.
    ~EFreeList()
    {
        this->freeFreeList();
    }

    //! Returns a new instance of E by either using an instance from the stack or creating a new one.
    inline E* alloc()
    {
        if(!EFreeListTypes<E, next>::FreeStack::empty(this))
            return EFreeListTypes<E, next>::FreeStack::popRet(this);
        else
            return new E();
    }

    //! Returns true if the stack is empty.
    inline bool empty() const
    {
        return EFreeListTypes<E, next>::FreeStack::empty(this);
    }

    //! Frees an item buy putting it onto the stack of free instances
    inline void free(E* ptr)
    {
        EFreeListTypes<E, next>::FreeStack::push(this, ptr);
    }

protected:
    //! deletes all instances in the list
    inline void freeFreeList()
    {
        while(!EFreeListTypes<E, next>::FreeStack::empty(this))
        {
            delete EFreeListTypes<E, next>::FreeStack::popRet(this);
        }
    }

    //! Top of the stack
    E* m_pTop;

    //! Typedef for the embedded stack
    //typedef EStack<EFreeList<E, next>, E, &EFreeList<E, next>::m_pTop, next> FreeStack;
};

//! Type declarations for EFreeList.
template<typename E, E* E::*next>
class EFreeListTypes
{
public:
    typedef EStack<EFreeList<E, next>, E, &EFreeList<E, next>::m_pTop, next> FreeStack;
};


//! More complex implementation of a FreeList, which is able to generate indeices for the elements.
template<typename E, E* E::*next, int E::*index>
class EFreeListIndexPool
{
public:
    //! Creates a new IndexPool and a FreeList.
    EFreeListIndexPool() : m_nextFreeIndex(0) { }

    //! Frees an element using the FreeList
    inline void free(E* ptr)
    {
        m_freeList.free(ptr);
    }

    //! The value indicates that all indices in 0..numUsedIndices-1 might be in use.
    inline int numUsedIndices() const
    {
        return m_nextFreeIndex;
    }

    //! Allocates a new Element by either using the free list or allocating a new one with a brand new index.
    inline E* alloc()
    {
        if(m_freeList.empty())
        {
            E* res = new E();
            res->*index = m_nextFreeIndex++;
            return res;
        }
        else
        {
            return m_freeList.alloc();
        }
    }

protected:
    //! The next brand new index.
    int m_nextFreeIndex;

    //! The free list for allocating the memory.
    EFreeList<E, next> m_freeList;
};

} // end of namespace ogdf

#endif
