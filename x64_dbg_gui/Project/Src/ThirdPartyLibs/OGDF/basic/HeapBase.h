/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author:klein $
 *   $Date:2007-10-18 17:23:28 +0200 (Thu, 18 Oct 2007) $
 ***************************************************************/

/** \file
 * \brief Declaration of an abstract heap base class for
 * priority queue implementation.
 *
 * \author Karsten Klein
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

#ifndef OGDF_HEAP_BASE_H
#define OGDF_HEAP_BASE_H


#include <ogdf/basic/GraphCopy.h>


namespace ogdf
{

/**

* \brief A data structure implementing the abstract data type
* priority queue
* A Heap is a data structure implementing the abstract data type
* priority queue, maintaining a set of elements each with an associated key value
* used for keeping the elements in a specific order.

* It supports the following operations:
* insert(element, key) insert element with key in set
* min   return element with minimum key
* extractMin   remove and return element with minimum key
* decreaseKey   decrease the key of a given element to a given value
* delete   remove a given element fromthe structure


* Running times and space requirements are depending on the actual
* implementation type (e.g., binary, binomial or fibonacci heaps)

*/


class HeapEntry;

class HeapEntryPointer;

template <class Priority, class HeapObject>
class HeapBase
{

public:
    //! Constructor
    HeapBase() { }

    virtual ~HeapBase() { }


    //! build a heap out of a given set of elements
    virtual void makeHeap() = 0;

    HeapObject minRet() { }

    //*******************************************************
    //Modification

    //! insert a new element with priority key
    virtual void insert(HeapObject, Priority /* key */) { }
    //extractMin
    //derived classes should decide themselves if they have
    //a specific delete function
    //virtual void delete() = 0;

    //! update the data structure by decreasing the key of an object
    //TODO: Does not make much sense without an object parameter
    virtual void decreaseKey() { }

    //*******************************************************
    //constant functions
    int size() const
    {
        return m_size;
    }

    bool empty() const
    {
        return m_size == 0;
    }


protected:
    int m_size; //number of elements stored in heap

};



} //namespace ogdf

#endif
