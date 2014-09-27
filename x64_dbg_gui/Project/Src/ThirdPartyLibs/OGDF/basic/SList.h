/*
 * $Revision: 2615 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 14:23:36 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of singly linked lists
 * (SListPure<E> and SList<E>) and iterators (SListConstIterator<E>
 * and SListIterator<E>).
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

#ifndef OGDF_SLIST_H
#define OGDF_SLIST_H


#include <ogdf/internal/basic/list_templates.h>


namespace ogdf
{


template<class E> class SListPure;
template<class E> class StackPure;
template<class E> class SListIterator;
template<class E> class SListConstIterator;


//! The parameterized class \a SListElement<E> represents the structure for elements of singly linked lists.
template<class E>
class SListElement
{
    friend class SListPure<E>;
    friend class StackPure<E>;
    friend class SListIterator<E>;
    friend class SListConstIterator<E>;

    SListElement<E>* m_next; //!< Pointer to successor element.
    E m_x; //!< Stores the content.

    //! Constructs an SListElement.
    SListElement() : m_next(0) { }
    //! Constructs an SListElement.
    SListElement(const E & x) : m_next(0), m_x(x) { }
    //! Constructs an SListElement.
    SListElement(const E & x, SListElement<E>* next) :
        m_next(next), m_x(x) { }

    OGDF_NEW_DELETE
}; // class SListElement



//! The parameterized class \a SListIterator<E> encapsulates a pointer to an slist element.
/**
 * It is used in order to iterate over singly linked lists,
 * and to specify a position in a singly linked list. It is possible that
 * an iterator encapsulates a null pointer.
 */

template<class E> class SListIterator
{
    SListElement<E>* m_pX; //!< Pointer to slist element.

    friend class SListConstIterator<E>;
    friend class SListPure<E>;

    //! Conversion to pointer to slist element.
    operator SListElement<E>* ()
    {
        return m_pX;
    }
    //! Conversion to pointer to slist element.
    operator const SListElement<E>* () const
    {
        return m_pX;
    }

public:
    //! Constructs an iterator pointing to no element.
    SListIterator() : m_pX(0) { }
    //! Constructs an iterator pointing to \a pX.
    SListIterator(SListElement<E>* pX) : m_pX(pX) { }
    //! Constructs an iterator that is a copy of \a it.
    SListIterator(const SListIterator<E> & it) : m_pX(it.m_pX) { }

    //! Returns true iff the iterator points to an element.
    bool valid() const
    {
        return m_pX != 0;
    }

    //! Equality operator.
    bool operator==(const SListIterator<E> & it) const
    {
        return m_pX == it.m_pX;
    }

    //! Inequality operator.
    bool operator!=(const SListIterator<E> & it) const
    {
        return m_pX != it.m_pX;
    }

    //! Returns successor iterator.
    SListIterator<E> succ() const
    {
        return m_pX->m_next;
    }

    //! Returns a reference to the element content.
    E & operator*() const
    {
        return m_pX->m_x;
    }

    //! Assignment operator.
    SListIterator<E> & operator=(const SListIterator<E> & it)
    {
        m_pX = it.m_pX;
        return *this;
    }

    //! Increment operator (prefix).
    SListIterator<E> & operator++()
    {
        m_pX = m_pX->m_next;
        return *this;
    }

    //! Increment operator (postfix).
    SListIterator<E> operator++(int)
    {
        SListIterator<E> it = *this;
        m_pX = m_pX->m_next;
        return it;
    }

    OGDF_NEW_DELETE
}; // class SListIterator



//! The parameterized class \a SListIterator<E> encapsulates a constant pointer to an slist element.
/**
 * It is used in order to iterate over singly linked lists,
 * and to specify a position in a singly linked list. It is possible that
 * an iterator encapsulates a null pointer. In contrast to SListIterator,
 * it is not possible to change the slist element pointed to.
 */

template<class E> class SListConstIterator
{
    const SListElement<E>* m_pX; //!< Pointer to slist element.

    friend class SListPure<E>;

    //! Conversion to pointer to slist element.
    operator const SListElement<E>* ()
    {
        return m_pX;
    }

public:
    //! Constructs an iterator pointing to no element.
    SListConstIterator() : m_pX(0) { }

    //! Constructs an iterator pointing to \a pX.
    SListConstIterator(const SListElement<E>* pX) : m_pX(pX) { }

    //! Constructs an iterator that is a copy of \a it.
    SListConstIterator(const SListIterator<E> & it) : m_pX((const SListElement<E>*)it) { }
    //! Constructs an iterator that is a copy of \a it.
    SListConstIterator(const SListConstIterator & it) : m_pX(it.m_pX) { }

    //! Returns true iff the iterator points to an element.
    bool valid() const
    {
        return m_pX != 0;
    }

    //! Equality operator.
    bool operator==(const SListConstIterator<E> & it) const
    {
        return m_pX == it.m_pX;
    }

    //! Inequality operator.
    bool operator!=(const SListConstIterator<E> & it) const
    {
        return m_pX != it.m_pX;
    }

    //! Returns successor iterator.
    SListConstIterator<E> succ() const
    {
        return m_pX->m_next;
    }

    //! Returns a reference to the element content.
    const E & operator*() const
    {
        return m_pX->m_x;
    }

    //! Assignment operator.
    SListConstIterator<E> & operator=(const SListConstIterator<E> & it)
    {
        m_pX = it.m_pX;
        return *this;
    }


    //! Increment operator (prefix).
    SListConstIterator<E> & operator++()
    {
        m_pX = m_pX->m_next;
        return *this;
    }

    //! Increment operator (postfix).
    SListConstIterator<E> operator++(int)
    {
        SListConstIterator<E> it = *this;
        m_pX = m_pX->m_next;
        return it;
    }

    OGDF_NEW_DELETE
}; // class SListConstIterator


//! The parameterized class \a SListPure<E> represents singly linked lists with content type \a E.
/**
 * Elements of the list are instances of type SListElement<E>.
 * Use SListConstIterator<E> or SListIterator<E> in order to iterate over the list.
 *
 * In contrast to SList<E>, instances of \a SListPure<E> do not store the length of the list.
 *
 * @tparam E is the data type stored in list elements.
 */

template<class E> class SListPure
{
    SListElement<E>* m_head; //!< Pointer to first element.
    SListElement<E>* m_tail; //!< Pointer to last element.

public:
    //! Constructs an empty singly linked list.
    SListPure() : m_head(0), m_tail(0) { }

    //! Constructs a singly linked list that is a copy of \a L.
    SListPure(const SListPure<E> & L) : m_head(0), m_tail(0)
    {
        copy(L);
    }

    // destruction
    ~SListPure()
    {
        clear();
    }

    typedef E value_type;
    typedef SListElement<E> element_type;
    typedef SListConstIterator<E> const_iterator;
    typedef SListIterator<E> iterator;

    //! Returns true iff the list is empty.
    bool empty() const
    {
        return m_head == 0;
    }

    //! Returns the length of the list
    /**
     * Notice that this method requires to run through the whole list and takes linear running time!
     */
    int size() const
    {
        int count = 0;
        for(SListElement<E>* pX = m_head; pX; pX = pX->m_next)
            ++count;
        return count;
    }

    //! Returns an iterator to the first element of the list.
    /**
     * If the list is empty, a null pointer iterator is returned.
     */
    SListConstIterator<E> begin() const
    {
        return m_head;
    }

    //! Returns an iterator to the first element of the list.
    /**
     * If the list is empty, a null pointer iterator is returned.
     */
    SListIterator<E> begin()
    {
        return m_head;
    }

    //! Returns an iterator to one-past-last element of the list.
    /**
     * This is always a null pointer iterator.
     */
    SListConstIterator<E> end() const
    {
        return SListConstIterator<E>();
    }

    //! Returns an iterator to one-past-last element of the list.
    /**
     * This is always a null pointer iterator.
     */
    SListIterator<E> end()
    {
        return SListIterator<E>();
    }

    //! Returns an iterator to the last element of the list.
    /**
     * If the list is empty, a null pointer iterator is returned.
     */
    SListConstIterator<E> rbegin() const
    {
        return m_tail;
    }

    //! Returns an iterator to the last element of the list.
    /**
     * If the list is empty, a null pointer iterator is returned.
     */
    SListIterator<E> rbegin()
    {
        return m_tail;
    }


    //! Returns an iterator pointing to the element at position \a pos.
    /**
     * The running time of this method is linear in \a pos.
     */
    SListConstIterator<E> get(int pos) const
    {
        SListElement<E>* pX;
        for(pX = m_head; pX != 0; pX = pX->m_next)
            if(pos-- == 0) break;
        return pX;
    }

    //! Returns an iterator pointing to the element at position \a pos.
    /**
     * The running time of this method is linear in \a pos.
     */
    SListIterator<E> get(int pos)
    {
        SListElement<E>* pX;
        for(pX = m_head; pX != 0; pX = pX->m_next)
            if(pos-- == 0) break;
        return pX;
    }

    //! Returns the position (starting with 0) of \a it in the list.
    /**
     * Positions are numbered 0,1,...
     * \pre \a it is an iterator pointing to an element in this list.
     */
    int pos(SListConstIterator<E> it) const
    {
        OGDF_ASSERT(it.valid())
        int p = 0;
        for(SListElement<E>* pX = m_head; pX != 0; pX = pX->m_next, ++p)
            if(pX == it) break;
        return p;
    }


    //! Returns a reference to the first element.
    /**
     * \pre The list is not empty!
     */
    const E & front() const
    {
        OGDF_ASSERT(m_head != 0)
        return m_head->m_x;
    }

    //! Returns a reference to the first element.
    /**
     * \pre The list is not empty!
     */
    E & front()
    {
        OGDF_ASSERT(m_head != 0)
        return m_head->m_x;
    }

    //! Returns a reference to the last element.
    /**
     * \pre The list is not empty!
     */
    const E & back() const
    {
        OGDF_ASSERT(m_tail != 0)
        return m_tail->m_x;
    }

    //! Returns a reference to the last element.
    /**
     * \pre The list is not empty!
     */
    E & back()
    {
        OGDF_ASSERT(m_tail != 0)
        return m_tail->m_x;
    }

    //! Returns an iterator to the cyclic successor of \a it.
    /**
     * \pre \a it points to an element in this list!
     */
    SListConstIterator<E> cyclicSucc(SListConstIterator<E> it) const
    {
        const SListElement<E>* pX = it;
        return (pX->m_next) ? pX->m_next : m_head;
    }

    //! Returns an iterator to the cyclic successor of \a it.
    /**
     * \pre \a it points to an element in this list!
     */
    SListIterator<E> cyclicSucc(SListIterator<E> it)
    {
        SListElement<E>* pX = it;
        return (pX->m_next) ? pX->m_next : m_head;
    }

    //! Assignment operator.
    SListPure<E> & operator=(const SListPure<E> & L)
    {
        clear();
        copy(L);
        return *this;
    }

    //! Adds element \a x at the begin of the list.
    SListIterator<E> pushFront(const E & x)
    {
        m_head = OGDF_NEW SListElement<E>(x, m_head);
        if(m_tail == 0) m_tail = m_head;
        return m_head;
    }

    //! Adds element \a x at the end of the list.
    SListIterator<E> pushBack(const E & x)
    {
        SListElement<E>* pNew = OGDF_NEW SListElement<E>(x);
        if(m_head == 0)
            m_head = m_tail = pNew;
        else
            m_tail = m_tail->m_next = pNew;
        return m_tail;
    }

    //! Inserts element \a x after \a pBefore.
    /**
     * \pre \a pBefore points to an element in this list.
     */
    SListIterator<E> insertAfter(const E & x, SListIterator<E> itBefore)
    {
        SListElement<E>* pBefore = itBefore;
        OGDF_ASSERT(pBefore != 0)
        SListElement<E>* pNew = OGDF_NEW SListElement<E>(x, pBefore->m_next);
        if(pBefore == m_tail) m_tail = pNew;
        return (pBefore->m_next = pNew);
    }

    //! Removes the first element from the list.
    /**
     * \pre The list is not empty!
     */
    void popFront()
    {
        OGDF_ASSERT(m_head != 0)
        SListElement<E>* pX = m_head;
        if((m_head = m_head->m_next) == 0) m_tail = 0;
        delete pX;
    }

    //! Removes the first element from the list and returns it.
    /**
     * \pre The list is not empty!
     */
    E popFrontRet()
    {
        E el = front();
        popFront();
        return el;
    }

    //! Removes the succesor of \a pBefore.
    /**
     * \pre \a pBefore points to an element in this list.
     */
    void delSucc(SListIterator<E> itBefore)
    {
        SListElement<E>* pBefore = itBefore;
        OGDF_ASSERT(pBefore != 0)
        SListElement<E>* pDel = pBefore->m_next;
        OGDF_ASSERT(pDel != 0)
        if((pBefore->m_next = pDel->m_next) == 0) m_tail = pBefore;
        delete pDel;
    }

    //! Moves the first element of this list to the begin of list \a L2.
    void moveFrontToFront(SListPure<E> & L2)
    {
        OGDF_ASSERT(m_head != 0)
        OGDF_ASSERT(this != &L2)
        SListElement<E>* pX = m_head;
        if((m_head = m_head->m_next) == 0) m_tail = 0;
        pX->m_next = L2.m_head;
        L2.m_head = pX;
        if(L2.m_tail == 0) L2.m_tail = L2.m_head;
    }

    //! Moves the first element of this list to the end of list \a L2.
    void moveFrontToBack(SListPure<E> & L2)
    {
        OGDF_ASSERT(m_head != 0)
        OGDF_ASSERT(this != &L2)
        SListElement<E>* pX = m_head;
        if((m_head = m_head->m_next) == 0) m_tail = 0;
        pX->m_next = 0;
        if(L2.m_head == 0)
            L2.m_head = L2.m_tail = pX;
        else
            L2.m_tail = L2.m_tail->m_next = pX;
    }

    //! Moves the first element of this list to list \a L2 inserted after \a itBefore.
    /**
     * \pre \a itBefore points to an element in \a L2.
     */
    void moveFrontToSucc(SListPure<E> & L2, SListIterator<E> itBefore)
    {
        OGDF_ASSERT(m_head != 0)
        OGDF_ASSERT(this != &L2)
        SListElement<E>* pBefore = itBefore;
        SListElement<E>* pX = m_head;
        if((m_head = m_head->m_next) == 0) m_tail = 0;
        pX->m_next = pBefore->m_next;
        pBefore->m_next = pX;
        if(pBefore == L2.m_tail) L2.m_tail = pX;
    }

    //! Removes all elements from the list.
    void clear()
    {
        if(m_head == 0) return;

#if (_MSC_VER == 1100)
        // workaround for bug in Visual Studio 5.0

        while(!empty())
            popFront();

#else

        if(doDestruction((E*)0))
        {
            for(SListElement<E>* pX = m_head; pX != 0; pX = pX->m_next)
                pX->m_x.~E();
        }
        OGDF_ALLOCATOR::deallocateList(sizeof(SListElement<E>), m_head, m_tail);

#endif

        m_head = m_tail = 0;
    }

    //! Appends \a L2 to this list and makes \a L2 empty.
    void conc(SListPure<E> & L2)
    {
        if(m_head)
            m_tail->m_next = L2.m_head;
        else
            m_head = L2.m_head;
        if(L2.m_tail != 0) m_tail = L2.m_tail;
        L2.m_head = L2.m_tail = 0;
    }

    //! Reverses the order of the list elements.
    void reverse()
    {
        SListElement<E>* p, *pNext, *pPred = 0;
        for(p = m_head; p; p = pNext)
        {
            pNext = p->m_next;
            p->m_next = pPred;
            pPred = p;
        }
        swap(m_head, m_tail);
    }

    //! Conversion to const SListPure.
    const SListPure<E> & getListPure() const
    {
        return *this;
    }

    //! Sorts the list using Quicksort.
    void quicksort()
    {
        ogdf::quicksortTemplate(*this);
    }

    //! Sorts the list using Quicksort and comparer \a comp.
    template<class COMPARER>
    void quicksort(const COMPARER & comp)
    {
        ogdf::quicksortTemplate(*this, comp);
    }

    //! Sorts the list using bucket sort.
    /**
     * @param l is the lowest bucket that will occur.
     * @param h is the highest bucket that will occur.
     * @param f returns the bucket for each element.
     * \pre The bucket function \a f will only return bucket values between \a l
     * and \a h for this list.
     */
    void bucketSort(int l, int h, BucketFunc<E> & f);

    //! Sorts the list using bucket sort.
    void bucketSort(BucketFunc<E> & f);

    //! Randomly permutes the elements in the list.
    void permute()
    {
        permute(size());
    }

    //! Scans the list for the specified element and returns its position in the list, or -1 if not found.
    int search(const E & e) const
    {
        int x = 0;
        for(SListConstIterator<E> i = begin(); i.valid(); ++i, ++x)
            if(*i == e) return x;
        return -1;
    }

    //! Scans the list for the specified element (using the user-defined comparer) and returns its position in the list, or -1 if not found.
    template<class COMPARER>
    int search(const E & e, const COMPARER & comp) const
    {
        int x = 0;
        for(SListConstIterator<E> i = begin(); i.valid(); ++i, ++x)
            if(comp.equal(*i, e)) return x;
        return -1;
    }

protected:
    void copy(const SListPure<E> & L)
    {
        for(SListElement<E>* pX = L.m_head; pX != 0; pX = pX->m_next)
            pushBack(pX->m_x);
    }

    void permute(const int n);

    OGDF_NEW_DELETE
}; // class SListPure



//! The parameterized class \a SList<E> represents singly linked lists with content type \a E.
/**
 * Elements of the list are instances of type SListElement<E>.
 * Use SListConstIterator<E> or SListIterator<E> in order to iterate over the list.
 * In contrast to SListPure<E>, instances of \a SList<E> store the length of the list
 * and thus allow constant time access to the length.
 *
 * @tparam E is the data type stored in list elements.
 */

template<class E>
class SList : private SListPure<E>
{

    int m_count; //!< The length of the list.

public:
    //! Constructs an empty singly linked list.
    SList() : m_count(0) { }

    //! Constructs a singly linked list that is a copy of \a L.
    SList(const SList<E> & L) : SListPure<E>(L), m_count(L.m_count) { }

    // destruction
    ~SList() { }

    typedef E value_type;
    typedef SListElement<E> element_type;
    typedef SListConstIterator<E> const_iterator;
    typedef SListIterator<E> iterator;

    //! Returns true iff the list is empty.
    bool empty() const
    {
        return SListPure<E>::empty();
    }

    //! Returns the length of the list.
    int size() const
    {
        return m_count;
    }

    //! Returns an iterator to the first element of the list.
    /**
     * If the list is empty, a null pointer iterator is returned.
     */
    const SListConstIterator<E> begin() const
    {
        return SListPure<E>::begin();
    }

    //! Returns an iterator to the first element of the list.
    /**
     * If the list is empty, a null pointer iterator is returned.
     */
    SListIterator<E> begin()
    {
        return SListPure<E>::begin();
    }

    //! Returns an iterator to one-past-last element of the list.
    /**
     * This is always a null pointer iterator.
     */
    SListConstIterator<E> end() const
    {
        return SListConstIterator<E>();
    }

    //! Returns an iterator to one-past-last element of the list.
    /**
     * This is always a null pointer iterator.
     */
    SListIterator<E> end()
    {
        return SListIterator<E>();
    }

    //! Returns an iterator to the last element of the list.
    /**
     * If the list is empty, a null pointer iterator is returned.
     */
    const SListConstIterator<E> rbegin() const
    {
        return SListPure<E>::rbegin();
    }

    //! Returns an iterator to the last element of the list.
    /**
     * If the list is empty, a null pointer iterator is returned.
     */
    SListIterator<E> rbegin()
    {
        return SListPure<E>::rbegin();
    }


    //! Returns an iterator pointing to the element at position \a pos.
    /**
     * The running time of this method is linear in \a pos.
     */
    SListConstIterator<E> get(int pos) const
    {
        return SListPure<E>::get(pos);
    }

    //! Returns an iterator pointing to the element at position \a pos.
    /**
     * The running time of this method is linear in \a pos.
     */
    SListIterator<E> get(int pos)
    {
        return SListPure<E>::get(pos);
    }

    //! Returns the position (starting with 0) of \a it in the list.
    /**
     * Positions are numbered 0,1,...
     * \pre \a it is an iterator pointing to an element in this list.
     */
    int pos(SListConstIterator<E> it) const
    {
        return SListPure<E>::pos(it);;
    }


    //! Returns a reference to the first element.
    /**
     * \pre The list is not empty!
     */
    const E & front() const
    {
        return SListPure<E>::front();
    }

    //! Returns a reference to the first element.
    /**
     * \pre The list is not empty!
     */
    E & front()
    {
        return SListPure<E>::front();
    }

    //! Returns a reference to the last element.
    /**
     * \pre The list is not empty!
     */
    const E & back() const
    {
        return SListPure<E>::back();
    }

    //! Returns a reference to the last element.
    /**
     * \pre The list is not empty!
     */
    E & back()
    {
        return SListPure<E>::back();
    }

    //! Returns an iterator to the cyclic successor of \a it.
    /**
     * \pre \a it points to an element in this list!
     */
    SListConstIterator<E> cyclicSucc(SListConstIterator<E> it) const
    {
        return SListPure<E>::cyclicSucc(it);
    }

    //! Returns an iterator to the cyclic successor of \a it.
    /**
     * \pre \a it points to an element in this list!
     */
    SListIterator<E> cyclicSucc(SListIterator<E> it)
    {
        return SListPure<E>::cyclicSucc(it);
    }

    //! Assignment operator.
    SList<E> & operator=(const SList<E> & L)
    {
        SListPure<E>::operator=(L);
        m_count = L.m_count;
        return *this;
    }

    //! Adds element \a x at the begin of the list.
    SListIterator<E> pushFront(const E & x)
    {
        ++m_count;
        return SListPure<E>::pushFront(x);
    }

    //! Adds element \a x at the end of the list.
    SListIterator<E> pushBack(const E & x)
    {
        ++m_count;
        return SListPure<E>::pushBack(x);
    }

    //! Inserts element \a x after \a pBefore.
    /**
     * \pre \a pBefore points to an element in this list.
     */
    SListIterator<E> insertAfter(const E & x, SListIterator<E> itBefore)
    {
        ++m_count;
        return SListPure<E>::insertAfter(x, itBefore);
    }

    //! Removes the first element from the list.
    /**
     * \pre The list is not empty!
     */
    void popFront()
    {
        --m_count;
        SListPure<E>::popFront();
    }

    //! Removes the first element from the list and returns it.
    /**
     * \pre The list is not empty!
     */
    E popFrontRet()
    {
        E el = front();
        popFront();
        return el;
    }

    //! Removes the succesor of \a pBefore.
    /**
     * \pre \a pBefore points to an element in this list.
     */
    void delSucc(SListIterator<E> itBefore)
    {
        --m_count;
        SListPure<E>::delSucc(itBefore);
    }

    //! Moves the first element of this list to the begin of list \a L2.
    void moveFrontToFront(SList<E> & L2)
    {
        SListPure<E>::moveFrontToFront(L2);
        --m_count;
        ++L2.m_count;
    }

    //! Moves the first element of this list to the end of list \a L2.
    void moveFrontToBack(SList<E> & L2)
    {
        SListPure<E>::moveFrontToBack(L2);
        --m_count;
        ++L2.m_count;
    }

    //! Moves the first element of this list to list \a L2 inserted after \a itBefore.
    /**
     * \pre \a itBefore points to an element in \a L2.
     */
    void moveFrontToSucc(SList<E> & L2, SListIterator<E> itBefore)
    {
        SListPure<E>::moveFrontToSucc(L2, itBefore);
        --m_count;
        ++L2.m_count;
    }

    //! Removes all elements from the list.
    void clear()
    {
        m_count = 0;
        SListPure<E>::clear();
    }

    //! Appends \a L2 to this list and makes \a L2 empty.
    void conc(SList<E> & L2)
    {
        SListPure<E>::conc(L2);
        m_count += L2.m_count;
        L2.m_count = 0;
    }

    //! Reverses the order of the list elements.
    void reverse()
    {
        SListPure<E>::reverse();
    }

    //! Conversion to const SListPure.
    const SListPure<E> & getListPure() const
    {
        return *this;
    }

    //! Sorts the list using Quicksort.
    void quicksort()
    {
        ogdf::quicksortTemplate(*this);
    }

    //! Sorts the list using Quicksort and comparer \a comp.
    template<class COMPARER>
    void quicksort(const COMPARER & comp)
    {
        ogdf::quicksortTemplate(*this, comp);
    }

    //! Sorts the list using bucket sort.
    /**
     * @param l is the lowest bucket that will occur.
     * @param h is the highest bucket that will occur.
     * @param f returns the bucket for each element.
     * \pre The bucket function \a f will only return bucket values between \a l
     * and \a h for this list.
     */
    void bucketSort(int l, int h, BucketFunc<E> & f)
    {
        SListPure<E>::bucketSort(l, h, f);
    }

    //! Sorts the list using bucket sort.
    void bucketSort(BucketFunc<E> & f)
    {
        SListPure<E>::bucketSort(f);
    }

    //! Randomly permutes the elements in the list.
    void permute()
    {
        SListPure<E>::permute(m_count);
    }

    //! Scans the list for the specified element and returns its position in the list, or -1 if not found.
    int search(const E & e) const
    {
        return SListPure<E>::search(e);
    }

    //! Scans the list for the specified element (using the user-defined comparer) and returns its position in the list, or -1 if not found.
    template<class COMPARER>
    int search(const E & e, const COMPARER & comp) const
    {
        return SListPure<E>::search(e, comp);
    }

    OGDF_NEW_DELETE
}; // class SList




// sorts list L using bucket sort
// computes l and h value
template<class E>
void SListPure<E>::bucketSort(BucketFunc<E> & f)
{
    // if less than two elements, nothing to do
    if(m_head == m_tail) return;

    int l, h;
    l = h = f.getBucket(m_head->m_x);

    SListElement<E>* pX;
    for(pX = m_head->m_next; pX; pX = pX->m_next)
    {
        int i = f.getBucket(pX->m_x);
        if(i < l) l = i;
        if(i > h) h = i;
    }

    bucketSort(l, h, f);
}


// sorts list L using bucket sort
template<class E>
void SListPure<E>::bucketSort(int l, int h, BucketFunc<E> & f)
{
    // if less than two elements, nothing to do
    if(m_head == m_tail) return;

    Array<SListElement<E> *> head(l, h, 0), tail(l, h);

    SListElement<E>* pX;
    for(pX = m_head; pX; pX = pX->m_next)
    {
        int i = f.getBucket(pX->m_x);
        if(head[i])
            tail[i] = (tail[i]->m_next = pX);
        else
            head[i] = tail[i] = pX;
    }

    SListElement<E>* pY = 0;
    for(int i = l; i <= h; i++)
    {
        pX = head[i];
        if(pX)
        {
            if(pY)
                pY->m_next = pX;
            else
                m_head = pX;
            pY = tail[i];
        }
    }

    m_tail = pY;
    pY->m_next = 0;
}


// permutes elements in list randomly; n is the length of the list
template<class E>
void SListPure<E>::permute(const int n)
{
    Array<SListElement<E> *> A(n + 1);
    A[n] = 0;

    int i = 0;
    SListElement<E>* pX;
    for(pX = m_head; pX; pX = pX->m_next)
        A[i++] = pX;

    A.permute(0, n - 1);

    for(i = 0; i < n; i++)
    {
        A[i]->m_next = A[i + 1];
    }

    m_head = A[0];
    m_tail = A[n - 1];
}

// prints list to output stream os using delimiter delim
template<class E>
void print(ostream & os, const SListPure<E> & L, char delim = ' ')
{
    SListConstIterator<E> pX = L.begin();
    if(pX.valid())
    {
        os << *pX;
        for(++pX; pX.valid(); ++pX)
            os << delim << *pX;
    }
}

// prints list to output stream os using delimiter delim
template<class E>
void print(ostream & os, const SList<E> & L, char delim = ' ')
{
    print(L.getListPure(), delim);
}

// output operator
template<class E>
ostream & operator<<(ostream & os, const SListPure<E> & L)
{
    print(os, L);
    return os;
}

template<class E>
ostream & operator<<(ostream & os, const SList<E> & L)
{
    return operator<<(os, L.getListPure());
}


// sort array using bucket sort and bucket object f;
// the values of f must be in the interval [min,max]
template<class E>
void bucketSort(Array<E> & a, int min, int max, BucketFunc<E> & f)
{
    if(a.low() >= a.high()) return;

    Array<SListPure<E> > bucket(min, max);

    int i;
    for(i = a.low(); i <= a.high(); ++i)
        bucket[f.getBucket(a[i])].pushBack(a[i]);

    i = a.low();
    for(int j = min; j <= max; ++j)
    {
        SListConstIterator<E> it = bucket[j].begin();
        for(; it.valid(); ++it)
            a[i++] = *it;
    }
}




} // namespace ogdf


#endif
