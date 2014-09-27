/*
 * $Revision: 2524 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-03 09:54:22 +0200 (Tue, 03 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of binary heap class that allows the
 * decreaseKey operation.
 *
 * \author Hoi-Ming Wong
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

#ifndef OGDF_BINARY_HEAP_H
#define OGDF_BINARY_HEAP_H

#include<ogdf/basic/Array.h>

namespace ogdf
{



// using min heaps
template <class X, class Priority = double, class INDEX = int>
class OGDF_EXPORT BinaryHeap
{

public:

    class Element
    {

        friend class BinaryHeap<X, Priority, INDEX>;

    private:
        Priority priority;
        X elem;
        INDEX backIdx ; // the position in element array

        // empty constructor
        Element() { }

        // construct element with object and priority
        Element(X x, Priority prior) : priority(prior), elem(x) { }

        //copy constructor
        Element(const Element & origElem): priority(origElem.priority), elem(origElem.elem), backIdx(origElem.backIdx) { }

    public:

        //return the priority
        Priority getPriority() const
        {
            return priority;
        }

        // return reference to elem
        const X & getElement() const
        {
            return elem;
        }

        // return the position of this element in the heap array
        INDEX getPos() const
        {
            return backIdx;
        }
    };


    //! construct a binary heap with capacity c
    BinaryHeap(INDEX c) : data(1, c, 0), s(0) { }

    //! destructor
    ~BinaryHeap()
    {
        for(INDEX i = 1; i <= s; ++i)
        {
            delete data[i];
            data[i] = 0;
        }
    }

    //! return true if heap is empty
    bool empty() const
    {
        return s == 0;
    }

    //! return the number of elements in the heap
    INDEX size() const
    {
        return s;
    }



    //! decrease the priority of elem
    void decPriority(const Element & elem, // handle to the element
                     Priority prior // new priority, must be smaller then the old value
                    )
    {
        INDEX i = elem.backIdx;

        OGDF_ASSERT(i <= s);

        if(data[i]->getPriority() < prior)
            throw "New key is greater than current key.";

        data[i]->priority = prior;
        while(i > 1 && (data[getParent(i)]->getPriority() > data[i]->getPriority()))
        {
            swap(i, getParent(i));
            i = getParent(i);
        }
    }

    //! insert elem and return a handle
    const Element & insert(X obj, Priority prior)
    {
        Element* h_elem = new Element(obj, prior);
        ++s;
        if(s == capacity())
            data.grow(capacity(), 0); // double the size
        h_elem->backIdx = s; // store the position
        data[s] = h_elem;
        INDEX i = s;
        while((i > 1) && (data[getParent(i)]->getPriority() > data[i]->getPriority()))
        {
            swap(i, getParent(i));
            i = getParent(i);
        }
        return *h_elem;
    }

    //! ! insert elem and return a handle [same as insert]
    const Element & push(X obj, Priority prior)
    {
        return insert(obj, prior);
    }

    //! return the Object with the min. score
    const X & getMin() const
    {
        return data[1]->getElement();
    }

    //! return the Object with the min. score [same as getMin]
    const X & top() const
    {
        return getMin();
    }

    //! return the smallest element and remove it from heap
    X extractMin()
    {
        if(empty())
            throw "Heap underflow error!";

        Element copy = *data[1];
        Element* p = data[1];
        swap(1, s);
        --s;
        delete p;
        minHeapify(1);
        data[s + 1] = 0;
        return copy.getElement();
    }

    //! return the smallest element and remove it from heap [same as extractMin]
    X pop()
    {
        return extractMin();
    }

    //! empty the heap
    void clear()
    {
        for(INDEX i = 1; i <= s; ++i)
        {
            delete data[i];
            data[i] = 0;
        }
        s = 0;
    }

    //! obtain const references to the element at index \a idx (the smallest heap array index is 0.).
    const X & operator[](INDEX idx) const
    {
        return data[idx + 1]->getElement();
    }


private :

    Array<Element*, INDEX> data;  // heap array starts at index 1
    INDEX s; // current number of elements

    //! return current capacity of the heap
    INDEX capacity() const
    {
        return data.size();
    }

    void swap(INDEX pos1, INDEX pos2)
    {
        Element* tmp = data[pos1];
        data[pos1] = data[pos2];
        data[pos2] = tmp;
        // update the position
        data[pos2]->backIdx = pos2;
        data[pos1]->backIdx = pos1;
    }



    void minHeapify(INDEX pos)
    {
        INDEX l = getLeft(pos);
        INDEX r = getRight(pos);
        INDEX smallest;
        if(l <= size() && data[l]->getPriority() < data[pos]->getPriority())
            smallest = l;
        else
            smallest = pos;
        if(r <= size() && data[r]->getPriority() < data[smallest]->getPriority())
            smallest = r;
        if(smallest != pos)
        {
            swap(pos, smallest);
            minHeapify(smallest);
        }
    }


    INDEX getParent(INDEX pos) const
    {
        return pos / 2;
    }

    INDEX getLeft(INDEX pos) const
    {
        return pos * 2;
    }

    INDEX getRight(INDEX pos) const
    {
        return pos * 2 + 1;
    }

};

}//namespace ogdf

#endif
