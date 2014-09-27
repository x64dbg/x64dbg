/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of binary heap class that allows the
 * decreaseKey operation.
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

#ifndef OGDF_BINARY_HEAP2_H
#define OGDF_BINARY_HEAP2_H

#include <ogdf/basic/HeapBase.h>

namespace ogdf
{

/**
 * \brief Min-heap priority queue realized by a data array.
 *
 * Heaps store objects that are weighted with costs;
 * the minimal cost object is accessible over
 * member function minRet() and extracted by extractMin().
 *
 * The class uses two template parameters:
 *   - \a key is the key type.
 *   - \a HeapElement is the type of the elements that are stored.
 *
 * HeapObjects can be all types with copy constructor,
 * as copies of inserted elements are created;
 * \a key should be of a type with compare operators.
 * We only allow integer as index/size type; the array index starts with 1.
 *
 * To allow direct access to the underlying array structure
 * in order to minimize decreaseKey() runningtime,
 * a pointer to an integer storage can be provided as input()
 * parameter that will be kept updated with the index position
 * during heap operations.
 *
 * <H3>Running Time</H3>
 * The worst case running times of the methods is given by the following
 * table, where \a n is the current number of elements.
 *
 * <table>
 *   <tr>
 *     <th>method<th>worst-case<th>amortized
 *   </tr><tr>
 *     <td>extractMin()<td>O(n)<td>O(lg(\a n))
 *   </tr><tr>
 *     <td>siftDown()<td>O(lg(\a n))<td>
 *   </tr><tr>
 *     <td>siftUp()<td>O(lg(\a n))<td>
 *   </tr><tr>
 *     <td>minRet()<td>O(1)<td>
 *   </tr><tr>
 *     <td>insert()<td>O(\a n)<td>O(lg(\a n))
 *   </tr>
 * </table>
 */


//to allow directobject adress, a pointer to an integer storage
//can be provided, where the array index is updated by the
//heap class


template <class key, class HeapObject>
class BinaryHeap2 : public HeapBase<key, HeapObject>
{
public:
    //! Creates a binary heap.
    BinaryHeap2(int startSize = 128);

    //copy Constructor, todo
    //BinaryHeap2(const BinaryHeap2& source);

    // Destructor, deletes the heap array.
    virtual ~BinaryHeap2()
    {
        if(m_heapArray) delete[] m_heapArray;
    }//destructor


    //! Assignment operator.
    const BinaryHeap2 & operator=(const BinaryHeap2<key, HeapObject> & rhs);

    //------------------------------------------------------------
    //modification:

    //! Inserts a new element \a obj with priority \a p and pointer for index update.
    void insert(HeapObject & obj, key & p, int* keyUpdate = 0);

    //! Obtains heap property, only needed if the elements are not inserted by insert method.
    virtual void makeHeap();
    //delete
    //it is not clear how a delete without explicit
    //given heapentry pointer  should behave, e.g. if equal values
    //for objects are allowed

    //! Returns minimum priority element and removes it from the heap.
    // arraySize is decreased if size < 1/3arraySize (amortized runtime O(1))
    HeapObject extractMin();

    //! Decreases priority of an object that is addressed by \a index.
    // use updated m_foreign position index to address entry for decreasekey
    virtual void decreaseKey(int index, key priority);
    //TODO: version mit Aenderungswert statt absolutem Wert

    //--------------------------------------------------------------
    //const access functions

    //! Returns minimum priority element.
    HeapObject minRet() const
    {
        return m_heapArray[1].m_object;
    }

    key getPriority(int index) const
    {
        OGDF_ASSERT((index > 0) && (index <= HeapBase<key, HeapObject>::m_size));
        return m_heapArray[index].m_priority;
    }//getPriority

    //! Returns the current size.
    int capacity() const
    {
        return m_arraySize;
    }

    //! Returns the number of stored elements.
    int size() const
    {
        return HeapBase<key, HeapObject>::m_size;
    }

    //! Returns true iff the heap is empty.
    int empty() const
    {
        return HeapBase<key, HeapObject>::empty();
    }

    //! Reinitializes the data structure.
    /**
     * Deletes the array and reallocates it with size that was passed at
     * construction time.
     */
    void clear();

protected:
    //! Establishes heap property by moving element up in heap if necessary.
    void siftUp(int pos);

    //! Establishes heap property by moving element down in heap if necessary.
    void siftDown(int pos);

    //----------------------------------------------------------
    //modelling the binary tree structure on the data array
    //array position 0 is left empty, positions are from 1..m_size
    //! Array index of parent node.
    int parentIndex(int num)
    {
        OGDF_ASSERT(num > 0);
        return num / 2;
    }//parent

    //! Array index of left child.
    int leftChildIndex(int num)
    {
        OGDF_ASSERT(num > 0);
        return 2 * num;
    }//leftChild

    //! Array index of right child.
    int rightChildIndex(int num)
    {
        OGDF_ASSERT(num > 0);
        return 2 * num + 1;
    }//rightChild

    //! Returns true if left child exists.
    bool hasLeft(int num)
    {
        OGDF_ASSERT(num > 0);
        return (leftChildIndex(num) <= HeapBase<key, HeapObject>::m_size);
    }

    //! Returns true if right child exists.
    bool hasRight(int num)
    {
        OGDF_ASSERT(num > 0);
        return (rightChildIndex(num) <= HeapBase<key, HeapObject>::m_size);
    }

    //----------------------------------------------------------
    //helper functions for internal maintainance
    int arrayBound(int arraySize)
    {
        return arraySize + 1;
    }
    int higherArrayBound(int arraySize)
    {
        return 2 * arraySize + 1;
    }
    int higherArraySize(int arraySize)
    {
        return 2 * arraySize;
    }
    int lowerArrayBound(int arraySize)
    {
        return arraySize / 2 + 1;
    }
    int lowerArraySize(int arraySize)
    {
        return arraySize / 2;
    }

    void init(int initSize);

private:
    //holding object and priority key
    struct HeapEntry
    {
        key m_priority;
        HeapObject m_object;

        //we maintain positions during operations
        int m_pos;
        int* m_foreignPos; //storage structure given by user

        //! Initializes HeapEntry object.
        HeapEntry()
        {
            m_priority = 0;
            m_pos = 0;
            m_foreignPos = 0;
        }

        //! Initializes HeapEntry object with priority.
        /**
        * @param k ist the priority.
        * @param ob is the corresponding HeapObject.
        */
        HeapEntry(key k, const HeapObject & ob)
        {
            m_priority = k;
            m_object = ob;
            m_foreignPos = 0;
            //m_pos = ob.m_pos;
        }

        //! Initializes HaepEntry object with priority.
        /**
        * @param k ist the priority.
        * @param ob is the corresponding HeapObject.
        * @param pos is the position of the object within the array.
        * @param fp is a pointer to the index.
        */
        HeapEntry(key k, const HeapObject & ob, int pos, int* fp)
        {
            m_priority = k;
            m_object = ob;
            if(fp == 0) m_foreignPos = 0;
            else m_foreignPos = fp;
            m_pos = pos;
        }
    };

    HeapEntry* m_heapArray; //dynamically maintained array of heapentries

    //in addition to m_size, the inherited number of objects from class HeapBase,
    //we store the actual size of the array, valid array object positions
    //are from 1 to m_size
    int m_arraySize; //current size of the heap

    int m_startSize; //(decide: optionally??) used to check reallocation bound

};//BinaryHeap2



//**************************************************************
//implementation
//**************************************************************


//**************************************************************
//constructor and initialization
template <class key, class HeapObject>
BinaryHeap2<key, HeapObject>::BinaryHeap2(int startSize)
    : HeapBase<key, HeapObject>()
{
    init(startSize);
}//constructor


template <class key, class HeapObject>
void BinaryHeap2<key, HeapObject>::init(int initSize)
{
    //create an array of HeapEntry Elements
    m_arraySize = initSize;
    m_heapArray = new HeapEntry[arrayBound(m_arraySize)]; //start at 1

    m_startSize = initSize;

    HeapBase<key, HeapObject>::m_size = 0;
}


template <class key, class HeapObject>
void BinaryHeap2<key, HeapObject>::clear()
{
    if(m_heapArray) delete[] m_heapArray;
    init(m_startSize);
}


//**************************************************************
//element shifting operations
//restore heap property by finding correct position for object
//at position pos on higher levels, pos is given as array index (1..m_size)
//updates array index values
template <class key, class HeapObject>
void BinaryHeap2<key, HeapObject>::siftUp(int pos)
{
    OGDF_ASSERT((pos > 0) && (pos <= HeapBase<key, HeapObject>::m_size))

    if(pos == 1)
    {
        m_heapArray[1].m_pos = 1;
        if(m_heapArray[1].m_foreignPos != 0)  //address is defined
            *(m_heapArray[1].m_foreignPos) = 1;
        return;//nothing to do
    }

    HeapEntry tempEntry = m_heapArray[pos];
    int run = pos;
    while((parentIndex(run) >= 1) &&
            (m_heapArray[parentIndex(run)].m_priority > tempEntry.m_priority))
    {
        m_heapArray[run] = m_heapArray[parentIndex(run)];
        if(m_heapArray[run].m_foreignPos != 0) *(m_heapArray[run].m_foreignPos) = run;
        run = parentIndex(run);
    }//while

    m_heapArray[run] = tempEntry;
    m_heapArray[run].m_pos = run;
    if(m_heapArray[run].m_foreignPos != 0) *(m_heapArray[run].m_foreignPos) = run;


}//siftup


//restore heap property by finding correct position for object
//at position pos on lower levels, updates array index values
template <class key, class HeapObject>
void BinaryHeap2<key, HeapObject>::siftDown(int pos)
{
    OGDF_ASSERT((pos > 0) && (pos <= HeapBase<key, HeapObject>::m_size));

    if(pos >= int(HeapBase<key, HeapObject>::m_size / 2) + 1)
    {
        m_heapArray[pos].m_pos = pos;
        if(m_heapArray[pos].m_foreignPos != 0) *(m_heapArray[pos].m_foreignPos) = pos;
        return; //leafs cant move down
    }//if leaf

    key sPrio = getPriority(pos);
    int sIndex = pos;

    if(hasLeft(pos) && (getPriority(leftChildIndex(pos)) < sPrio))
    {
        sIndex = leftChildIndex(pos);
        sPrio = getPriority(leftChildIndex(pos));
    }//if left child smaller
    if(hasRight(pos) && (getPriority(rightChildIndex(pos)) < sPrio))
    {
        sIndex = rightChildIndex(pos);
        sPrio = getPriority(rightChildIndex(pos));
    }//if right child smaller

    if(sIndex != pos)
    {
        HeapEntry tempEntry = m_heapArray[pos];
        m_heapArray[pos] = m_heapArray[sIndex];
        m_heapArray[sIndex] = tempEntry;

        //update both index entries
        m_heapArray[pos].m_pos = pos;
        if(m_heapArray[pos].m_foreignPos != 0) *(m_heapArray[pos].m_foreignPos) = pos;
        m_heapArray[sIndex].m_pos = sIndex;
        if(m_heapArray[sIndex].m_foreignPos != 0) *(m_heapArray[sIndex].m_foreignPos) = sIndex;

        siftDown(sIndex); //TODO: dont use recursion
    }//if sift necessary
    else  //update in case of new elements (non-insert)
    {
        m_heapArray[pos].m_pos = pos;
        if(m_heapArray[pos].m_foreignPos != 0) *(m_heapArray[pos].m_foreignPos) = pos;
    }//else
}//siftdown


template <class key, class HeapObject>
void BinaryHeap2<key, HeapObject>::makeHeap()
{
    //only needed if insertion is not done over insert
    //(if we allow array parameter in constructor)
    for(int i = HeapBase<key, HeapObject>::m_size / 2; i > 0; i--)
        siftDown(i);
}//makeheap


template <class key, class HeapObject>
void BinaryHeap2<key, HeapObject>::decreaseKey(int index, key priority)
{
    HeapEntry & he = m_heapArray[index];

    //check if error value
    if(he.m_priority < priority) OGDF_THROW_PARAM(AlgorithmFailureException, afcIllegalParameter);

    he.m_priority = priority;
    siftUp(index);

}//decreaseKey


//extract the minimum priority object and reallocate array if size < 1/3 arraysize
template <class key, class HeapObject>
HeapObject BinaryHeap2<key, HeapObject>::extractMin()
{
    OGDF_ASSERT((!HeapBase<key, HeapObject>::empty()));

    HeapEntry tempEntry = m_heapArray[1]; //save minimum object

    HeapBase<key, HeapObject>::m_size--;

    if(HeapBase<key, HeapObject>::m_size > 0)
    {
        m_heapArray[1] = m_heapArray[HeapBase<key, HeapObject>::m_size + 1]; //old last leaf

        //check if reallocation is possible
        if((HeapBase<key, HeapObject>::m_size < (m_arraySize / 3)) && (m_arraySize > 2 * m_startSize - 1))
        {
            HeapEntry* tempHeap = new HeapEntry[lowerArrayBound(m_arraySize)];
            for(int i = 1; i <= HeapBase<key, HeapObject>::m_size ; i++)
                tempHeap[i] = m_heapArray[i];
            delete[] m_heapArray;
            m_heapArray = tempHeap;
            m_arraySize = lowerArraySize(m_arraySize);

        }//if small enough

        //restore tree by sifting down old leaf
        siftDown(1);
    }//if not empty

    return tempEntry.m_object;

}//extractMin


//place a copy of the given input element in the queue, doubles
//array size if necessary
template <class key, class HeapObject>
void BinaryHeap2<key, HeapObject>::insert(HeapObject & ho, key & priority, int* keyUpdate)
{
    OGDF_ASSERT((HeapBase<key, HeapObject>::m_size) < m_arraySize);
    HeapBase<key, HeapObject>::m_size++;
    //check if the array size has to be adjusted
    if(HeapBase<key, HeapObject>::m_size == m_arraySize)
    {
        HeapEntry* tempHeap = new HeapEntry[higherArrayBound(m_arraySize)];
        for(int i = 1; i <= m_arraySize ; i++)  //last one is not occupied yet
            tempHeap[i] = m_heapArray[i];
        delete[] m_heapArray;
        m_heapArray = tempHeap;
        m_arraySize = higherArraySize(m_arraySize);

    }//if array full

    //now insert object and reestablish heap property
    m_heapArray[HeapBase<key, HeapObject>::m_size] = HeapEntry(priority, ho, HeapBase<key, HeapObject>::m_size, keyUpdate);

    siftUp(HeapBase<key, HeapObject>::m_size);

}//insert



template <class key, class HeapObject>
const BinaryHeap2<key, HeapObject> & BinaryHeap2<key, HeapObject>::operator=(const BinaryHeap2<key, HeapObject> & rhs)
{
    if(this != &rhs)
    {
        if(m_heapArray && !(m_arraySize == rhs.m_arraySize))
        {
            delete[] m_heapArray;
            m_heapArray = 0;
        }//if

        if(!m_heapArray)
            m_heapArray = new HeapEntry[arrayBound(rhs.m_arraySize)]; //start at 1

        OGDF_ASSERT(m_heapArray);

        HeapBase<key, HeapObject>::m_size = rhs.m_size;

        m_startSize = rhs.m_startSize;
        m_arraySize = rhs.m_arraySize;

        for(int i = 1; i <= HeapBase<key, HeapObject>::m_size ; i++)
            m_heapArray[i] = rhs.m_heapArray[i];

    }//if not self
    return *this;
}



}//namespace ogdf

#endif
