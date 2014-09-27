/*
 * $Revision: 2583 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-12 01:02:21 +0200 (Do, 12. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declares & Implements Binary Heap, and Top10Heap
 *
 * \author Markus Chimani
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

#ifndef OGDF_MIN_HEAP_H
#define OGDF_MIN_HEAP_H

#include<ogdf/basic/Array.h>

namespace ogdf
{

//! Augments any data elements of type \a X with keys of type \a Score.
/**
 * Also defines comparator function using the keys.
 * This class is intended as a helpful convenience class for using with BinaryHeapSimple, Top10Heap,..
 */
template<class X, class Priority = double> class Prioritized
{
    X x;
    Priority p;
public:
    //! Constructor of empty element. Be careful!
    Prioritized() : x(0), p(0) { }
    //! Constructor using a key/value pair
    Prioritized(X xt, Priority pt) : x(xt), p(pt) { }
    //! Copy-constructor
    Prioritized(const Prioritized & P) : x(P.x), p(P.p) { }
    //! Returns the key of the element
    Priority priority() const
    {
        return p;
    }
    //! Returns the data of the element
    X item() const
    {
        return x;
    }
    //! Comparison oprator based on the compare-operator for the key type (\a Priority)
    bool operator<(const Prioritized<X, Priority> & P) const
    {
        return p < P.p;
    }
    //! Comparison oprator based on the compare-operator for the key type (\a Priority)
    bool operator<=(const Prioritized<X, Priority> & P) const
    {
        return p <= P.p;
    }
    //! Comparison oprator based on the compare-operator for the key type (\a Priority)
    bool operator>(const Prioritized<X, Priority> & P) const
    {
        return p > P.p;
    }
    //! Comparison oprator based on the compare-operator for the key type (\a Priority)
    bool operator>=(const Prioritized<X, Priority> & P) const
    {
        return p >= P.p;
    }
    //! Comparison oprator based on the compare-operator for the key type (\a Priority)
    bool operator==(const Prioritized<X, Priority> & P) const
    {
        return p == P.p;
    }
    //! Comparison oprator based on the compare-operator for the key type (\a Priority)
    bool operator!=(const Prioritized<X, Priority> & P) const
    {
        return p != P.p;
    }
};


//! Dynamically growing binary heap tuned for efficiency on a small interface (compared to BinaryHeap).
/**
 * It assumes that the data-elements are themselves comparable, i.e., the compare-function
 * of the items implicitly defines the keys. Hence this datastructure allows no key-changing
 * operations (decreaseKey, etc.).
 *
 * The heap grows (using doubling) dynamically, if there are more elements added. Furthermore,
 * BinaryHeapSimple allows to be directly indexed using traditional array-syntax, e.g., for iterating over
 * all its elements.
 *
 * If your intended datastructure does not offer a (suitable) compare function, but you have
 * certain key-values (scores, etc.), you may want to use the convenience-class
 * Prioritized < Score,X > to bind both together and use within BinaryHeapSimple.
 */
template<class X, class INDEX = int>
class BinaryHeapSimple
{
private:
    Array<X, INDEX> data; // array starts at index 1
    INDEX num;
public:
    //! Construtor, giving initial array size
    BinaryHeapSimple(INDEX size) : data(1, size), num(0) {}

    //! Returns true if the heap is empty
    bool empty() const
    {
        return num == 0;
    }
    //! Returns the number of elements in the heap
    INDEX size() const
    {
        return num;
    }

    //! empties the heap [O(1)]
    void clear()
    {
        num = 0;
    }

    //! Returns a reference to the top (i.e., smallest) element of the heap. It does not remove it. [Same as getMin(), O(1)]
    const X & top() const
    {
        return data[1];
    }
    //! Returns a reference to the top (i.e., smallest) element of the heap. It does not remove it. [Same as top(), O(1)]
    inline const X & getMin() const
    {
        return top();
    }

    //! Adds an element to the heap [Same as insert(), O(log n)]
    void push(X & x)
    {
        X y;
        if(num == capacity())
            data.grow(capacity(), y); // double the size & init with nulls
        data[++num] = x;
        heapup(num);
    }
    //! Adds an element to the heap [Same as push(), O(log n)]
    inline void insert(X & x)
    {
        push(x);
    }

    //! Returns the top (i.e., smallest) element and removed it from the heap [Same as extractMin(), O(log n)]
    X pop()
    {
        data.swap(1, num--);
        heapdown();
        return data[num + 1];
    }
    //! Returns the top (i.e., smallest) element and removed it from the heap [Same as  pop(), O(log n)]
    inline X extractMin()
    {
        return pop();
    }

    //! obtain const references to the element at index \a idx (the smallest array index is 0, as for traditional C-arrays)
    const X & operator[](INDEX idx) const
    {
        return data[idx + 1];
    }


protected:
    //! Returns the current array-size of the heap, i.e., the number of elements which can be added before the next resize occurs.
    INDEX capacity() const
    {
        return data.size();
    }

    void heapup(INDEX idx)
    {
        INDEX papa;
        while((papa = idx / 2) > 0)
        {
            if(data[papa] > data[idx])
            {
                data.swap(papa, idx);
                idx = papa;
            }
            else return;   //done
        }
    }

    void heapdown()
    {
        INDEX papa = 1;
        INDEX son;
        while(true)
        {
            if((son = 2 * papa) < num && data[son + 1] < data[son])
                son++;
            if(son <= num && data[son] < data[papa])
            {
                data.swap(papa, son);
                papa = son;
            }
            else return;
        }
    }
};

//! A variant of BinaryHeapSimple which always holds only the X (e.g. X=10) elements with the highest keys.
/**
 * It assumes that the data-elements are themselves comparable, i.e., the compare-function
 * of the items implicitly defines the keys.
 *
 * If your intended datastructure do not dorectly offer a compare function, but you have
 * certain key-values (scores, etc.), you may want to use the convenience-class
 * Prioritized < Priority,X > to bind both together and use within BinaryHeapSimple.
 */
template<class X, class INDEX = int>
class Top10Heap : protected BinaryHeapSimple<X, INDEX>  // favors the 10 highest values...
{
public:
    //! The type for results of a Top10Heap::push operation
    enum PushResult { Accepted, Rejected, Swapped };

    //! Convenience function: Returns true if the PushResults states that the newly pushed element is new in the heap
    static bool successful(PushResult r)
    {
        return r != Rejected;
    }
    //! Convenience function: Returns true if the PushResults states that push caused an element to be not/no-longer in the heap
    static bool returnedSomething(PushResult r)
    {
        return r != Accepted;
    }

    //! Constructor generating a heap which holds the 10 elements with highest value ever added to the heap
    Top10Heap() : BinaryHeapSimple<X, INDEX>(10) {}
    //! Constructor generating a heap which holds the \a size elements with highest value ever added to the heap
    Top10Heap(INDEX size) : BinaryHeapSimple<X, INDEX>(size) {}

    //! Returns true if the heap contains no elements
    bool empty() const
    {
        return BinaryHeapSimple<X, INDEX>::empty();
    }
    //! Returns true if the heap is completely filled (i.e. the next push operation will return something)
    bool full() const
    {
        return size() == capacity();
    }
    //! Returns the number of elements in the heap
    INDEX size() const
    {
        return BinaryHeapSimple<X, INDEX>::size();
    }
    //! Returns the size of the heap specified when constructing: this is the number of top elements stored.
    INDEX capacity() const
    {
        return BinaryHeapSimple<X, INDEX>::capacity();
    }

    //! empties the heap
    void clear()
    {
        BinaryHeapSimple<X, INDEX>::clear();
    }

    //! Tries to push the element \a x onto the heap (and may return a removed element as \a out).
    /**
     * If the heap is not yet completely filled, the pushed element is accepted and added to the heap.
     * The function returns \a Accepted, and the \a out parameter is not touched.
     *
     * If the heap is filled and the key of the pushed element is too small to be accepted
     * (i.e. the heap is filled with all larger elements), then the element if rejected: The funtion
     * returns \a Rejected, and the \a out parameter is set to \a x.
     *
     * If the heap is filled and the key of the pushed element is large enough to belong to the top
     * elements, the element is accepted and the currently smallest element in the heap is removed
     * from the heap. The function returns \a Swapped and sets the \a out parameter to the element
     * removed from the heap.
     *
     * You may want to use the convenience funtions \a successful and \a returnedSomething on the
     * return-value if you are only interested certain aspects of the push.
     */
    PushResult push(X & x, X & out) // returns element that got kicked out - out is uninitialized if heap wasn't full (i.e. PushResult equals Accepted)
    {
        PushResult ret = Accepted;
        if(capacity() == size())
        {
            if(BinaryHeapSimple<X, INDEX>::top() >= x) // reject new item since it's too bad
            {
                out = x;
                return Rejected;
            }
            out = BinaryHeapSimple<X, INDEX>::pop(); // remove worst first
            ret = Swapped;
        }
        BinaryHeapSimple<X, INDEX>::push(x);
        return ret;
    }
    //! Alternative name for push().
    inline PushResult insert(X & x, X & out)
    {
        return push(x, out);
    }

    //! Simple (and slightly faster) variant of Top10Heap::push.
    /**
     * The behavior is the identical to Top10Heap::push, but there is nothing reported to the outside
     */
    void pushBlind(X & x)
    {
        if(capacity() == size())
        {
            if(BinaryHeapSimple<X, INDEX>::top() >= x) // reject new item since it's too bad
                return;
            BinaryHeapSimple<X, INDEX>::pop(); // remove worst first
        }
        BinaryHeapSimple<X, INDEX>::push(x);
    }
    //! Alternative name for pushBlind().
    inline void insertBlind(X & x, X & out)
    {
        return pushBlind(x, out);
    }

    //! obtain const references to the element at index \a idx
    /**
     * The smallest array index is 0, as for traditional C-arrays.
     * Useful, e.g., when iterating through the final heap elements.
     */
    const X & operator[](INDEX idx) const  // ATTN: simulate index starting at 0, to be "traditional" to the outside!!!
    {
        return BinaryHeapSimple<X, INDEX>::operator[](idx);
    }
};

//! A variant of Top10Heap which deletes the elements that get rejected from the heap
/**
 * The datastructure of course requires the stored data-elements to be pointers (in order to be deletable when
 * rejected). Hence the template parameter only specifies the data-type, without stating axplicitly that we
 * considere pointers to the structure.
 *
 * The datastructure also allows for non-duplicate insertions, i.e., a new element can be rejected if it is
 * already in the heap. Note that only the compare function has to work
 */
template<class X, class Priority = double, class STATICCOMPARER = StdComparer<X>, class INDEX = int >
class DeletingTop10Heap : public Top10Heap<Prioritized<X*, Priority>, INDEX >
{
public:
    //! Construct a DeletingTop10Heap of given maximal capacity
    DeletingTop10Heap(int size) : Top10Heap<Prioritized<X*, Priority>, INDEX >(size) {}
    //! Inserts the element \a x into the heap with priority \a val and deletes the element with smallest priority if the heap is full
    /**
     * Like the Top10Heap, this function pushes the element \a x onto the heap with priority \a val, and extracts the element with
     * smallest priority if the heap was already full. In contrast to the Top10Heap, this element which leaves the heap (or \a x
     * itself if its priority was below all the priorities in the heap) gets deleted, i.e., removed from memory.
     */
    void pushAndDelete(X* x, Priority p)
    {
        Prioritized<X*, Priority> vo;
        Prioritized<X*, Priority> nv(x, p);
        if(returnedSomething(Top10Heap<Prioritized<X*, Priority>, INDEX >::push(nv, vo)))
            delete vo.item();
    }
    //! Alternative name for pushAndDelete().
    inline void insertAndDelete(X* x, Priority p)
    {
        pushAndDelete(x, p);
    }
    //! Analogous to pushandDelete(), but furthermore rejects (and deletes) an element if an equal element is already in the heap.
    /**
     * This function takes linear time in the worst case, and uses the \a compare function of the specified COMP template
     * paremeter class, which can be any function returning \a true if two objects should be considered equal, and \a false otherwise.
     */
    void pushAndDeleteNoRedundancy(X* x, Priority p)
    {
        for(INDEX i = Top10Heap<Prioritized<X*, Priority>, INDEX >::size(); i-- > 0;)
        {
            X* k = Top10Heap<Prioritized<X*, Priority>, INDEX >::operator[](i).item();
            //          OGDF_ASSERT( x )
            //          OGDF_ASSERT( k )
            if(TargetComparer<X, STATICCOMPARER>::equal(k, x))
            {
                delete x;
                return;
            }
        }
        pushAndDelete(x, p);
    }
    //! Alternative name for pushAndKillNoRedundancy().
    inline void insertAndDeleteNoRedundancy(X* x, Priority p)
    {
        pushAndDeleteNoRedundancy(p, x);
    }
};

} // end namespace ogdf


#endif
