/*
 * $Revision: 2524 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-03 09:54:22 +0200 (Tue, 03 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of a base class for planar representations
 *        of graphs and cluster graphs.
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



#ifndef OGDF_MIN_PRIORITY_QUEUE_H
#define OGDF_MIN_PRIORITY_QUEUE_H

#include<ogdf/basic/Array.h>


namespace ogdf
{


template<class Score, class X>
class HeapElement
{

    template<class T1, class T2> friend class MinPriorityQueue;

private:
    Score v;
    X x;
    int pos; // the position in the heapElement array
public:
    HeapElement() {}
    //copy constructor
    HeapElement(const HeapElement<Score, X> & orig): v(orig.v), x(orig.x), pos(orig.pos) {}
    HeapElement(Score vt, X xt) : v(vt), x(xt) {}

    //return the score
    Score score_value() const
    {
        return v;
    }
    // return the element
    X element() const
    {
        return x;
    }
};



// using min heaps
// follow the pseudo code of "introduction to algorithms"
template <class Score, class X>
class MinPriorityQueue
{

private :

    HeapElement<Score, X>** heapElements;

    int number; // the number of elements in the Queue
    int s; // size

    void swap(int pos1, int pos2)
    {
        HeapElement<Score, X>* tmp = heapElements[pos1];
        heapElements[pos1] = heapElements[pos2];
        heapElements[pos2] = tmp;
        // update the position
        heapElements[pos2]->pos = pos2;
        heapElements[pos1]->pos = pos1;
    }

    void minHeapify(int pos)
    {
        int l = getLeft(pos);
        int r = getRight(pos);
        int smallest;
        if(l <= number && heapElements[l]->score_value() < heapElements[pos]->score_value())
            smallest = l;
        else
            smallest = pos;
        if(r <= number && heapElements[r]->score_value() < heapElements[smallest]->score_value())
            smallest = r;
        if(smallest != pos)
        {
            swap(pos, smallest);
            minHeapify(smallest);
        }
    }

    int getParent(int pos) const
    {
        return pos / 2;
    }
    int getLeft(int pos) const
    {
        return pos * 2;
    }
    int getRight(int pos) const
    {
        return pos * 2 + 1;
    }


public:

    // contructor, only fixed size
    MinPriorityQueue(int _size) : number(0), s(_size)
    {
        heapElements = new HeapElement<Score, X>* [_size + 1]; // allocate
        for(int i = 0; i < s + 1; ++i)
        {
            heapElements[i] = 0;
        }
    }


    ~MinPriorityQueue()
    {
        for(int i = 0; i < s + 1; ++i)
        {
            if(heapElements[i] != 0)
            {
                delete heapElements[i];
                heapElements[i] = 0;
            }
        }
        delete [] heapElements;
    }


    bool empty() const
    {
        return number == 0;
    }
    int count() const
    {
        return number;
    }
    int size() const
    {
        return s;
    }

    //return the Object with the min. score
    const X & getMin() const
    {
        return heapElements[1]->element();
    }



    void decreasePriority(const HeapElement<Score, X>* elem, // handle to the element
                          Score sc // new score, muss be smaller then the old score
                         )
    {
        int i = elem->pos;
        OGDF_ASSERT(i <= s);
        if(heapElements[i]->score_value() < sc)
            throw "New key is greater than current key.";
        heapElements[i]->v = sc;
        while(i > 1 && (heapElements[getParent(i)]->score_value() > heapElements[i]->score_value()))
        {
            swap(i, getParent(i));
            i = getParent(i);
        }
    }


    // return a handle to the new inserted element
    const HeapElement<Score, X>* insert(const HeapElement<Score, X> & elem)
    {
        HeapElement<Score, X>* h_elem = new HeapElement<Score, X>(elem); // make a copy
        ++number;
        OGDF_ASSERT(number <= s);
        h_elem->pos = number; // store the position
        heapElements[number] = h_elem;
        int i = number;
        while((i > 1) && (heapElements[getParent(i)]->score_value() > heapElements[i]->score_value()))
        {
            swap(i, getParent(i));
            i = getParent(i);
        }
        return h_elem;
    }


    // return the smallest element and remove it from the queue
    HeapElement<Score, X> pop()
    {
        if(empty())
            throw "Heap underflow error!";
        HeapElement<Score, X> obj = *heapElements[1];
        HeapElement<Score, X>* p = heapElements[1];
        swap(1, number);
        --number;
        delete p;
        minHeapify(1);
        OGDF_ASSERT(number + 1 <= s);
        heapElements[number + 1] = 0;
        return obj;
    }



    /*********************************************************************************/
    // debug
    void outHeap()
    {
        cout << "\nHeap Array: \n";
        for(int i = 0; i < s + 1; i++)
        {
            HeapElement<Score, X>* obj = heapElements[i];
            if(obj != NULL)
                cout << "score: " << obj->score_value() << "; elem: " << obj->element() << "; index " << i << "; pos: " << obj->pos << endl << flush;
            else
                cout << "index: " << i << " value: null;" <<  endl << flush;
        }
    }
};


}// namespace

#endif
