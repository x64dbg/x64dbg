/*
 * $Revision: 2632 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-17 21:04:24 +0200 (Di, 17. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of doubly linked lists and iterators
 *
 * \author Carsten Gutwenger and Sebastian Leipert
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

#ifndef OGDF_LIST_H
#define OGDF_LIST_H


#include <ogdf/internal/basic/list_templates.h>


namespace ogdf
{


template<class E> class List;
template<class E> class ListPure;
template<class E> class ListIterator;
template<class E> class ListConstIterator;


//! The parameterized class \a ListElement<E> represents the structure for elements of doubly linked lists.
template<class E>
class ListElement
{
    friend class ListPure<E>;
    friend class List<E>;
    friend class ListIterator<E>;
    friend class ListConstIterator<E>;

    ListElement<E>* m_next; //!< Pointer to successor element.
    ListElement<E>* m_prev; //!< Pointer to predecessor element.
    E m_x; //!< Stores the content.

    //! Constructs a ListElement.
    ListElement() : m_next(0), m_prev(0) { }
    //! Constructs a ListElement.
    ListElement(const E & x) : m_next(0), m_prev(0), m_x(x) { }
    //! Constructs a ListElement.
    ListElement(const E & x, ListElement<E>* next, ListElement<E>* prev) :
        m_next(next), m_prev(prev), m_x(x) { }

    OGDF_NEW_DELETE
}; // class ListElement



//! The parameterized class \a ListIterator<E> encapsulates a pointer to a dlist element.
/**
 * It is used in order to iterate over doubly linked lists,
 * and to specify a position in a doubly linked list. It is possible that
 * an iterator encapsulates a null pointer.
 */

template<class E> class ListIterator
{
    ListElement<E>* m_pX; // pointer to associated list element

    friend class ListConstIterator<E>;
    friend class ListPure<E>;

    //! Conversion to pointer to list element.
    operator ListElement<E>* ()
    {
        return m_pX;
    }
    //! Conversion to pointer to list element.
    operator const ListElement<E>* () const
    {
        return m_pX;
    }

public:
    //! Constructs an iterator pointing to no element.
    ListIterator() : m_pX(0) { }
    //! Constructs an iterator pointing to \a pX.
    ListIterator(ListElement<E>* pX) : m_pX(pX) { }
    //! Constructs an iterator that is a copy of \a it.
    ListIterator(const ListIterator<E> & it) : m_pX(it.m_pX) { }

    //! Returns true iff the iterator points to an element.
    bool valid() const
    {
        return m_pX != 0;
    }

    //! Equality operator.
    bool operator==(const ListIterator<E> & it) const
    {
        return m_pX == it.m_pX;
    }

    //! Inequality operator.
    bool operator!=(const ListIterator<E> & it) const
    {
        return m_pX != it.m_pX;
    }

    //! Returns successor iterator.
    ListIterator<E> succ() const
    {
        return m_pX->m_next;
    }

    //! Returns predecessor iterator.
    ListIterator<E> pred() const
    {
        return m_pX->m_prev;
    }

    //! Returns a reference to the element content.
    E & operator*() const
    {
        return m_pX->m_x;
    }

    //! Assignment operator.
    ListIterator<E> & operator=(const ListIterator<E> & it)
    {
        m_pX = it.m_pX;
        return *this;
    }

    //! Increment operator (prefix).
    ListIterator<E> & operator++()
    {
        m_pX = m_pX->m_next;
        return *this;
    }

    //! Increment operator (postfix).
    ListIterator<E> operator++(int)
    {
        ListIterator<E> it = *this;
        m_pX = m_pX->m_next;
        return it;
    }

    //! Decrement operator (prefix).
    ListIterator<E> & operator--()
    {
        m_pX = m_pX->m_prev;
        return *this;
    }

    //! Decrement operator (postfix).
    ListIterator<E> operator--(int)
    {
        ListIterator<E> it = *this;
        m_pX = m_pX->m_prev;
        return it;
    }

    OGDF_NEW_DELETE
}; // class ListIterator



//---------------------------------------------------------
// ListConstIterator<E>
// const iterator for doubly linked lists
//---------------------------------------------------------
//! The parameterized class \a ListIterator<E> encapsulates a constant pointer to a list element.
/**
 * It is used in order to iterate over doubly linked lists,
 * and to specify a position in a doubly linked list. It is possible that
 * an iterator encapsulates a null pointer. In contrast to ListIterator,
 * it is not possible to change the list element pointed to.
 */

template<class E> class ListConstIterator
{
    const ListElement<E>* m_pX; // pointer to list element

    friend class ListPure<E>;

    //! Conversion to pointer to list element.
    operator const ListElement<E>* ()
    {
        return m_pX;
    }

public:
    //! Constructs an iterator pointing to no element.
    ListConstIterator() : m_pX(0) { }

    //! Constructs an iterator pointing to \a pX.
    ListConstIterator(const ListElement<E>* pX) : m_pX(pX) { }

    //! Constructs an iterator that is a copy of \a it.
    ListConstIterator(const ListIterator<E> & it) : m_pX((const ListElement<E>*)it) { }
    //! Constructs an iterator that is a copy of \a it.
    ListConstIterator(const ListConstIterator & it) : m_pX(it.m_pX) { }

    //! Returns true iff the iterator points to an element.
    bool valid() const
    {
        return m_pX != 0;
    }

    //! Equality operator.
    bool operator==(const ListConstIterator<E> & it) const
    {
        return m_pX == it.m_pX;
    }

    //! Inequality operator.
    bool operator!=(const ListConstIterator<E> & it) const
    {
        return m_pX != it.m_pX;
    }

    //! Returns successor iterator.
    ListConstIterator<E> succ() const
    {
        return m_pX->m_next;
    }

    //! Returns predecessor iterator.
    ListConstIterator<E> pred() const
    {
        return m_pX->m_prev;
    }

    //! Returns a reference to the element content.
    const E & operator*() const
    {
        return m_pX->m_x;
    }

    //! Assignment operator.
    ListConstIterator<E> & operator=(const ListConstIterator<E> & it)
    {
        m_pX = it.m_pX;
        return *this;
    }

    //! Increment operator (prefix).
    ListConstIterator<E> & operator++()
    {
        m_pX = m_pX->m_next;
        return *this;
    }

    //! Increment operator (postfix).
    ListConstIterator<E> operator++(int)
    {
        ListConstIterator<E> it = *this;
        m_pX = m_pX->m_next;
        return it;
    }

    //! Decrement operator (prefix).
    ListConstIterator<E> & operator--()
    {
        m_pX = m_pX->m_prev;
        return *this;
    }

    //! Decrement operator (postfix).
    ListConstIterator<E> operator--(int)
    {
        ListConstIterator<E> it = *this;
        m_pX = m_pX->m_prev;
        return it;
    }

    OGDF_NEW_DELETE
}; // class ListConstIterator



//! The parameterized class \a ListPure<E> represents doubly linked lists with content type \a E.
/**
 * Elements of the list are instances of type ListElement<E>.
 * Use ListConstIterator<E> or ListIterator<E> in order to iterate over the list.
 *
 * In contrast to List<E>, instances of \a ListPure<E> do not store the length of the list.
 *
 * @tparam E is the data type stored in list elements.
 */

template<class E> class ListPure
{
protected:

    ListElement<E>* m_head; //!< Pointer to first element.
    ListElement<E>* m_tail; //!< Pointer to last element.

public:
    //! Constructs an empty doubly linked list.
    ListPure() : m_head(0), m_tail(0) { }

    //! Constructs a doubly linked list that is a copy of \a L.
    ListPure(const ListPure<E> & L) : m_head(0), m_tail(0)
    {
        copy(L);
    }

    // destruction
    ~ListPure()
    {
        clear();
    }

    typedef E value_type;
    typedef ListElement<E> element_type;
    typedef ListConstIterator<E> const_iterator;
    typedef ListIterator<E> iterator;

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
        for(ListElement<E>* pX = m_head; pX; pX = pX->m_next)
            ++count;
        return count;
    }

    //! Returns an iterator to the first element of the list.
    /**
     * If the list is empty, a null pointer iterator is returned.
     */
    const ListConstIterator<E> begin() const
    {
        return m_head;
    }
    //! Returns an iterator to the first element of the list.
    /**
     * If the list is empty, a null pointer iterator is returned.
     */
    ListIterator<E> begin()
    {
        return m_head;
    }

    //! Returns an iterator to one-past-last element of the list.
    /**
     * This is always a null pointer iterator.
     */
    ListConstIterator<E> end() const
    {
        return ListConstIterator<E>();
    }
    //! Returns an iterator to one-past-last element of the list.
    /**
     * This is always a null pointer iterator.
     */
    ListIterator<E> end()
    {
        return ListIterator<E>();
    }

    //! Returns an iterator to the last element of the list.
    /**
     * If the list is empty, a null pointer iterator is returned.
     */
    const ListConstIterator<E> rbegin() const
    {
        return m_tail;
    }
    //! Returns an iterator to the last element of the list.
    /**
     * If the list is empty, a null pointer iterator is returned.
     */
    ListIterator<E> rbegin()
    {
        return m_tail;
    }

    //! Returns an iterator to one-before-first element of the list.
    /**
     * This is always a null pointer iterator.
     */
    ListConstIterator<E> rend() const
    {
        return ListConstIterator<E>();
    }
    //! Returns an iterator to one-before-first element of the list.
    /**
     * This is always a null pointer iterator.
     */
    ListIterator<E> rend()
    {
        return ListIterator<E>();
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
    ListConstIterator<E> cyclicSucc(ListConstIterator<E> it) const
    {
        const ListElement<E>* pX = it;
        return (pX->m_next) ? pX->m_next : m_head;
    }

    //! Returns an iterator to the cyclic successor of \a it.
    /**
     * \pre \a it points to an element in this list!
     */
    ListIterator<E> cyclicSucc(ListIterator<E> it)
    {
        OGDF_ASSERT(it.valid())
        ListElement<E>* pX = it;
        return (pX->m_next) ? pX->m_next : m_head;
    }

    //! Returns an iterator to the cyclic predecessor of \a it.
    /**
     * \pre \a it points to an element in this list!
     */
    ListConstIterator<E> cyclicPred(ListConstIterator<E> it) const
    {
        OGDF_ASSERT(it.valid())
        const ListElement<E>* pX = it;
        return (pX->m_prev) ? pX->m_prev : m_tail;
    }

    //! Returns an iterator to the cyclic predecessor of \a it.
    /**
     * \pre \a it points to an element in this list!
     */
    ListIterator<E> cyclicPred(ListIterator<E> it)
    {
        OGDF_ASSERT(it.valid())
        ListElement<E>* pX = it;
        return (pX->m_prev) ? pX->m_prev : m_tail;
    }

    //! Returns an iterator pointing to the element at position \a pos.
    /**
     * The running time of this method is linear in \a pos.
     */
    ListConstIterator<E> get(int pos) const
    {
        ListElement<E>* pX;
        for(pX = m_head; pX != 0; pX = pX->m_next)
            if(pos-- == 0) break;
        return pX;
    }

    //! Returns an iterator pointing to the element at position \a pos.
    /**
     * The running time of this method is linear in \a pos.
     */
    ListIterator<E> get(int pos)
    {
        ListElement<E>* pX;
        for(pX = m_head; pX != 0; pX = pX->m_next)
            if(pos-- == 0) break;
        return pX;
    }

    //! Returns the position (starting with 0) of iterator \a it in the list.
    /**
     * \pre \a it is a valid iterator pointing to an element in this list!
     */
    int pos(ListConstIterator<E> it) const
    {
        OGDF_ASSERT(it.valid())
        int p = 0;
        for(ListElement<E>* pX = m_head; pX != 0; pX = pX->m_next, ++p)
            if(pX == it) break;
        return p;
    }

    //! Returns an iterator to a random element in the list (or an invalid iterator if the list is empty)
    /**
     * This method takes linear time.
     */
    ListConstIterator<E> chooseIterator() const
    {
        return empty() ? ListConstIterator<E>() : get(randomNumber(0, size() - 1));
    }

    //! Returns an iterator to a random element in the list (or an invalid iterator if the list is empty)
    /**
     * This method takes linear time.
     */
    ListIterator<E> chooseIterator()
    {
        return empty() ? ListIterator<E>() : get(randomNumber(0, size() - 1));
    }

    //! Returns a random element from the list.
    /**
     * \pre The list is not empty!
     *
     * This method takes linear time.
     */
    const E chooseElement() const
    {
        OGDF_ASSERT(m_head != 0)
        return *chooseIterator();
    }

    //! Returns a random element from the list.
    /**
     * \pre The list is not empty!
     *
     * This method takes linear time.
     */
    E chooseElement()
    {
        return *chooseIterator();
    }

    //! Assignment operator.
    ListPure<E> & operator=(const ListPure<E> & L)
    {
        clear();
        copy(L);
        return *this;
    }

    //! Equality operator.
    bool operator==(const ListPure<E> & L) const
    {
        ListElement<E>* pX = m_head, *pY = L.m_head;
        while(pX != 0 && pY != 0)
        {
            if(pX->m_x != pY->m_x)
                return false;
            pX = pX->m_next;
            pY = pY->m_next;
        }
        return (pX == 0 && pY == 0);
    }

    //! Inequality operator.
    bool operator!=(const ListPure<E> & L) const
    {
        return !operator==(L);
    }

    //! Adds element \a x at the begin of the list.
    ListIterator<E> pushFront(const E & x)
    {
        ListElement<E>* pX = OGDF_NEW ListElement<E>(x, m_head, 0);
        if(m_head)
            m_head = m_head->m_prev = pX;
        else
            m_head = m_tail = pX;
        return m_head;
    }

    //! Adds element \a x at the end of the list.
    ListIterator<E> pushBack(const E & x)
    {
        ListElement<E>* pX = OGDF_NEW ListElement<E>(x, 0, m_tail);
        if(m_head)
            m_tail = m_tail->m_next = pX;
        else
            m_tail = m_head = pX;
        return m_tail;
    }

    //! Inserts element \a x before or after \a it.
    /**
     * @param x is the element to be inserted.
     * @param it is a list iterator in this list.
     * @param dir determines if \a x is inserted before or after \a it.
     *   Possible values are \c ogdf::before and \c ogdf::after.
     * \pre \a it points to an element in this list.
     */
    ListIterator<E> insert(const E & x, ListIterator<E> it, Direction dir = after)
    {
        OGDF_ASSERT(it.valid())
        OGDF_ASSERT(dir == after || dir == before)
        ListElement<E>* pY = it, *pX;
        if(dir == after)
        {
            ListElement<E>* pYnext = pY->m_next;
            pY->m_next = pX = OGDF_NEW ListElement<E>(x, pYnext, pY);
            if(pYnext) pYnext->m_prev = pX;
            else m_tail = pX;
        }
        else
        {
            ListElement<E>* pYprev = pY->m_prev;
            pY->m_prev = pX = OGDF_NEW ListElement<E>(x, pY, pYprev);
            if(pYprev) pYprev->m_next = pX;
            else m_head = pX;
        }
        return pX;
    }

    //! Inserts element \a x before \a it.
    /**
     * \pre \a it points to an element in this list.
     */
    ListIterator<E> insertBefore(const E & x, ListIterator<E> it)
    {
        OGDF_ASSERT(it.valid())
        ListElement<E>* pY = it, *pX;
        ListElement<E>* pYprev = pY->m_prev;
        pY->m_prev = pX = OGDF_NEW ListElement<E>(x, pY, pYprev);
        if(pYprev) pYprev->m_next = pX;
        else m_head = pX;
        return pX;
    }

    //! Inserts element \a x after \a it.
    /**
     * \pre \a it points to an element in this list.
     */
    ListIterator<E> insertAfter(const E & x, ListIterator<E> it)
    {
        OGDF_ASSERT(it.valid())
        ListElement<E>* pY = it, *pX;
        ListElement<E>* pYnext = pY->m_next;
        pY->m_next = pX = OGDF_NEW ListElement<E>(x, pYnext, pY);
        if(pYnext) pYnext->m_prev = pX;
        else m_tail = pX;
        return pX;
    }

    //! Removes the first element from the list.
    /**
     * \pre The list is not empty!
     */
    void popFront()
    {
        OGDF_ASSERT(m_head != 0)
        ListElement<E>* pX = m_head;
        m_head = m_head->m_next;
        delete pX;
        if(m_head) m_head->m_prev = 0;
        else m_tail = 0;
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

    //! Removes the last element from the list.
    /**
     * \pre The list is not empty!
     */
    void popBack()
    {
        OGDF_ASSERT(m_tail != 0)
        ListElement<E>* pX = m_tail;
        m_tail = m_tail->m_prev;
        delete pX;
        if(m_tail) m_tail->m_next = 0;
        else m_head = 0;
    }

    //! Removes the last element from the list and returns it.
    /**
     * \pre The list is not empty!
     */
    E popBackRet()
    {
        E el = back();
        popBack();
        return el;
    }

    //! Removes \a it from the list.
    /**
     * \pre \a it points to an element in this list.
     */
    void del(ListIterator<E> it)
    {
        OGDF_ASSERT(it.valid())
        ListElement<E>* pX = it, *pPrev = pX->m_prev, *pNext = pX->m_next;
        delete pX;
        if(pPrev) pPrev->m_next = pNext;
        else m_head = pNext;
        if(pNext) pNext->m_prev = pPrev;
        else m_tail = pPrev;
    }

    //! Exchanges the positions of \a it1 and \a it2 in the list.
    /**
     * \pre \a it1 and \a it2 point to elements in this list.
     */
    void exchange(ListIterator<E> it1, ListIterator<E> it2)
    {
        OGDF_ASSERT(it1.valid() && it2.valid() && it1 != it2)
        ListElement<E>* pX = it1, *pY = it2;

        std::swap(pX->m_next, pY->m_next);
        std::swap(pX->m_prev, pY->m_prev);

        if(pX->m_next == pX)
        {
            pX->m_next = pY;
            pY->m_prev = pX;
        }
        if(pX->m_prev == pX)
        {
            pX->m_prev = pY;
            pY->m_next = pX;
        }

        if(pX->m_prev) pX->m_prev->m_next = pX;
        else m_head = pX;

        if(pY->m_prev) pY->m_prev->m_next = pY;
        else m_head = pY;

        if(pX->m_next) pX->m_next->m_prev = pX;
        else m_tail = pX;

        if(pY->m_next) pY->m_next->m_prev = pY;
        else m_tail = pY;
    }

    //! Moves \a it to the begin of the list.
    /**
     * \pre \a it points to an element in this list.
     */
    void moveToFront(ListIterator<E> it)
    {
        OGDF_ASSERT(it.valid())
        // remove it
        ListElement<E>* pX = it, *pPrev = pX->m_prev, *pNext = pX->m_next;
        //already at front
        if(!pPrev) return;

        //update old position
        if(pPrev) pPrev->m_next = pNext;
        if(pNext) pNext->m_prev = pPrev;
        else m_tail = pPrev;
        // insert it at front
        pX->m_prev = 0;
        pX->m_next = m_head;
        m_head = m_head->m_prev = pX;
    }//move

    //! Moves \a it to the end of the list.
    /**
     * \pre \a it points to an element in this list.
     */
    void moveToBack(ListIterator<E> it)
    {
        OGDF_ASSERT(it.valid())
        // remove it
        ListElement<E>* pX = it, *pPrev = pX->m_prev, *pNext = pX->m_next;
        //already at back
        if(!pNext) return;

        //update old position
        if(pPrev) pPrev->m_next = pNext;
        else m_head = pNext;
        if(pNext) pNext->m_prev = pPrev;
        // insert it at back
        pX->m_prev = m_tail;
        pX->m_next = 0;
        m_tail = m_tail->m_next = pX;
    }//move

    //! Moves \a it after \a itBefore.
    /**
     * \pre \a it and \a itBefore point to elements in this list.
     */
    void moveToSucc(ListIterator<E> it, ListIterator<E> itBefore)
    {
        OGDF_ASSERT(it.valid() && itBefore.valid())
        // move it
        ListElement<E>* pX = it, *pPrev = pX->m_prev, *pNext = pX->m_next;
        //the same of already in place
        ListElement<E>* pY = itBefore;
        if(pX == pY || pPrev == pY) return;

        // update old position
        if(pPrev) pPrev->m_next = pNext;
        else m_head = pNext;
        if(pNext) pNext->m_prev = pPrev;
        else m_tail = pPrev;
        // move it after itBefore
        ListElement<E>* pYnext = pX->m_next = pY->m_next;
        (pX->m_prev = pY)->m_next = pX;
        if(pYnext) pYnext->m_prev = pX;
        else m_tail = pX;
    }//move

    //! Moves \a it before \a itAfter.
    /**
     * \pre \a it and \a itAfter point to elements in this list.
     */
    void moveToPrec(ListIterator<E> it, ListIterator<E> itAfter)
    {
        OGDF_ASSERT(it.valid() && itAfter.valid())
        // move it
        ListElement<E>* pX = it, *pPrev = pX->m_prev, *pNext = pX->m_next;
        //the same of already in place
        ListElement<E>* pY = itAfter;
        if(pX == pY || pNext == pY) return;

        // update old position
        if(pPrev) pPrev->m_next = pNext;
        else m_head = pNext;
        if(pNext) pNext->m_prev = pPrev;
        else m_tail = pPrev;
        // move it before itAfter
        ListElement<E>* pYprev = pX->m_prev = pY->m_prev;
        (pX->m_next = pY)->m_prev = pX;
        if(pYprev) pYprev->m_next = pX;
        else m_head = pX;
    }//move

    //! Moves \a it to the begin of \a L2.
    /**
     * \pre \a it points to an element in this list.
     */
    void moveToFront(ListIterator<E> it, ListPure<E> & L2)
    {
        OGDF_ASSERT(it.valid())
        OGDF_ASSERT(this != &L2)
        // remove it
        ListElement<E>* pX = it, *pPrev = pX->m_prev, *pNext = pX->m_next;
        if(pPrev) pPrev->m_next = pNext;
        else m_head = pNext;
        if(pNext) pNext->m_prev = pPrev;
        else m_tail = pPrev;
        // insert it at front of L2
        pX->m_prev = 0;
        if((pX->m_next = L2.m_head) != 0)
            L2.m_head = L2.m_head->m_prev = pX;
        else
            L2.m_head = L2.m_tail = pX;
    }

    //! Moves \a it to the end of \a L2.
    /**
     * \pre \a it points to an element in this list.
     */
    void moveToBack(ListIterator<E> it, ListPure<E> & L2)
    {
        OGDF_ASSERT(it.valid())
        OGDF_ASSERT(this != &L2)
        // remove it
        ListElement<E>* pX = it, *pPrev = pX->m_prev, *pNext = pX->m_next;
        if(pPrev) pPrev->m_next = pNext;
        else m_head = pNext;
        if(pNext) pNext->m_prev = pPrev;
        else m_tail = pPrev;
        // insert it at back of L2
        pX->m_next = 0;
        if((pX->m_prev = L2.m_tail) != 0)
            L2.m_tail = L2.m_tail->m_next = pX;
        else
            L2.m_head = L2.m_tail = pX;
    }

    //! Moves \a it to list \a L2 and inserts it after \a itBefore.
    /**
     * \pre \a it points to an element in this list, and \a itBefore
     *      points to an element in \a L2.
     */
    void moveToSucc(ListIterator<E> it, ListPure<E> & L2, ListIterator<E> itBefore)
    {
        OGDF_ASSERT(it.valid() && itBefore.valid())
        OGDF_ASSERT(this != &L2)
        // remove it
        ListElement<E>* pX = it, *pPrev = pX->m_prev, *pNext = pX->m_next;
        if(pPrev) pPrev->m_next = pNext;
        else m_head = pNext;
        if(pNext) pNext->m_prev = pPrev;
        else m_tail = pPrev;
        // insert it in list L2 after itBefore
        ListElement<E>* pY = itBefore;
        ListElement<E>* pYnext = pX->m_next = pY->m_next;
        (pX->m_prev = pY)->m_next = pX;
        if(pYnext) pYnext->m_prev = pX;
        else L2.m_tail = pX;
    }

    //! Moves \a it to list \a L2 and inserts it before \a itAfter.
    /**
     * \pre \a it points to an element in this list, and \a itAfter
     *      points to an element in \a L2.
     */
    void moveToPrec(ListIterator<E> it, ListPure<E> & L2, ListIterator<E> itAfter)
    {
        OGDF_ASSERT(it.valid() && itAfter.valid())
        OGDF_ASSERT(this != &L2)
        // remove it
        ListElement<E>* pX = it, *pPrev = pX->m_prev, *pNext = pX->m_next;
        if(pPrev) pPrev->m_next = pNext;
        else m_head = pNext;
        if(pNext) pNext->m_prev = pPrev;
        else m_tail = pPrev;
        // insert it in list L2 after itBefore
        ListElement<E>* pY = itAfter;
        ListElement<E>* pYprev = pX->m_prev = pY->m_prev;
        (pX->m_next = pY)->m_prev = pX;
        if(pYprev) pYprev->m_next = pX;
        else L2.m_head = pX;
    }

    //! Appends \a L2 to this list and makes \a L2 empty.
    void conc(ListPure<E> & L2)
    {
        OGDF_ASSERT(this != &L2)
        if(m_head)
            m_tail->m_next = L2.m_head;
        else
            m_head = L2.m_head;
        if(L2.m_head)
        {
            L2.m_head->m_prev = m_tail;
            m_tail = L2.m_tail;
        }
        L2.m_head = L2.m_tail = 0;
    }

    //! Prepends \a L2 to this list and makes \a L2 empty.
    void concFront(ListPure<E> & L2)
    {
        OGDF_ASSERT(this != &L2)
        if(m_head)
            m_head->m_prev = L2.m_tail;
        else
            m_tail = L2.m_tail;
        if(L2.m_head)
        {
            L2.m_tail->m_next = m_head;
            m_head = L2.m_head;
        }
        L2.m_head = L2.m_tail = 0;
    }

    //! Exchanges too complete lists in O(1).
    /**
     * The list's content is moved to L2 and vice versa.
     */
    void exchange(ListPure<E> & L2)
    {
        ListElement<E>* t;
        t = this->m_head;
        this->m_head = L2.m_head;
        L2.m_head = t;
        t = this->m_tail;
        this->m_tail = L2.m_tail;
        L2.m_tail = t;
    }

    //! Splits the list at element \a it into lists \a L1 and \a L2.
    /**
     * If \a it is not a null pointer and \a L = x1,...,x{k-1}, \a it,x_{k+1},xn, then
     * \a L1 = x1,...,x{k-1} and \a L2 = \a it,x{k+1},...,xn if \a dir = \c before.
     * If \a it is a null pointer, then \a L1 is made empty and \a L2 = \a L. Finally
     * \a L is made empty if it is not identical to \a L1 or \a L2.
     *
     * \pre \a it points to an element in this list.
     */

    void split(ListIterator<E> it, ListPure<E> & L1, ListPure<E> & L2, Direction dir = before)
    {
        if(&L1 != this) L1.clear();
        if(&L2 != this) L2.clear();

        if(it.valid())
        {
            L1.m_head = m_head;
            L2.m_tail = m_tail;
            if(dir == before)
            {
                L2.m_head = it;
                L1.m_tail = L2.m_head->m_prev;
            }
            else
            {
                L1.m_tail = it;
                L2.m_head = L1.m_tail->m_next;
            }
            L2.m_head->m_prev = L1.m_tail->m_next = 0;

        }
        else
        {
            L1.m_head = L1.m_tail = 0;
            L2.m_head = m_head;
            L2.m_tail = m_tail;
        }

        if(this != &L1 && this != &L2)
        {
            m_head = m_tail = 0;
        }
    }

    //! Splits the list after \a it.
    void splitAfter(ListIterator<E> it, ListPure<E> & L2)
    {
        OGDF_ASSERT(it.valid())
        OGDF_ASSERT(this != &L2)
        L2.clear();
        ListElement<E>* pX = it;
        if(pX != m_tail)
        {
            (L2.m_head = pX->m_next)->m_prev = 0;
            pX->m_next = 0;
            L2.m_tail = m_tail;
            m_tail = pX;
        }
    }

    //! Splits the list before \a it.
    void splitBefore(ListIterator<E> it, ListPure<E> & L2)
    {
        OGDF_ASSERT(it.valid())
        OGDF_ASSERT(this != &L2)
        L2.clear();
        ListElement<E>* pX = it;
        L2.m_head = pX;
        L2.m_tail = m_tail;
        if((m_tail = pX->m_prev) == 0)
            m_head = 0;
        else
            m_tail->m_next = 0;
        pX->m_prev = 0;
    }

    //! Reverses the order of the list elements.
    void reverse()
    {
        ListElement<E>* pX = m_head;
        m_head = m_tail;
        m_tail = pX;
        while(pX)
        {
            ListElement<E>* pY = pX->m_next;
            pX->m_next = pX->m_prev;
            pX = pX->m_prev = pY;
        }
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
            for(ListElement<E>* pX = m_head; pX != 0; pX = pX->m_next)
                pX->m_x.~E();
        }
        OGDF_ALLOCATOR::deallocateList(sizeof(ListElement<E>), m_head, m_tail);

#endif

        m_head = m_tail = 0;
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

    //! Randomly permutes the elements in the list.
    void permute()
    {
        permute(size());
    }

    //! Scans the list for the specified element and returns its position in the list, or -1 if not found.
    int search(const E & e) const
    {
        int x = 0;
        for(ListConstIterator<E> i = begin(); i.valid(); ++i, ++x)
            if(*i == e) return x;
        return -1;
    }

    //! Scans the list for the specified element (using the user-defined comparer) and returns its position in the list, or -1 if not found.
    template<class COMPARER>
    int search(const E & e, const COMPARER & comp) const
    {
        int x = 0;
        for(ListConstIterator<E> i = begin(); i.valid(); ++i, ++x)
            if(comp.equal(*i, e)) return x;
        return -1;
    }

protected:
    void copy(const ListPure<E> & L)
    {
        for(ListElement<E>* pX = L.m_head; pX != 0; pX = pX->m_next)
            pushBack(pX->m_x);
    }

    void permute(const int n);

    OGDF_NEW_DELETE
}; // class ListPure



//! Iteration over all iterators \a it of a list \a L, where L is of Type \c List<\a type>.
/**
 * Automagically creates a \c ListConstIterator<\a type> named \a it, and runs through the List \a L.
 *
 * <h3>Example</h3>
 *
 * The following code runs through the list \a L, and prints each item
 *   \code
 *   List<double> L;
 *   ...
 *   forall_listiterators(double, it, L) {
 *     cout << *it << endl;
 *   }
 *   \endcode
 *
 *   Note that this code is equivalent to the following tedious long version
 *
 *   \code
 *   List<double> L;
 *   ...
 *   for( ListConstIterator<double> it = L.begin(); it.valid(); ++it) {
 *     cout << *it << endl;
 *   }
 *   \endcode
 */
#define forall_listiterators(type, it, L) \
    for(ListConstIterator< type > it = (L).begin(); it.valid(); ++it)

//! Iteration over all iterators \a it of a list \a L, where L is of Type \c List<\a type>, in reverse order.
/**
 * Automagically creates a \c ListConstIterator<\a type> named \a it, and runs through the List \a L, in reverse order.
 * See \c #forall_listiterators for an example.
 */
#define forall_rev_listiterators(type, it, L) \
    for(ListConstIterator< type > it = (L).rbegin(); it.valid(); --it)

//! Iteration over all non-const iterators \a it of a list \a L, where L is of Type \c List<\a type>.
/**
 * Automagically creates a \c ListIterator<\a type> named \a it, and runs through the List \a L.
 * See \c #forall_listiterators for an example.
 */
#define forall_nonconst_listiterators(type, it, L) \
    for(ListIterator< type > it = (L).begin(); it.valid(); ++it)

//! Iteration over all non-const iterators \a it of a list \a L, where L is of Type \c List<\a type>, in reverse order.
/**
 * Automagically creates a \c ListIterator<\a type> named \a it, and runs through the List \a L, in reverse order.
 * See \c #forall_listiterators for an example.
 */
#define forall_rev_nonconst_listiterators(type, it, L) \
    for(ListIterator< type > it = (L).rbegin(); it.valid(); --it)

//! Iteration over all iterators \a it of a list \a L, where L is of Type \c SList<\a type>.
/**
 * Automagically creates a \c SListConstIterator<\a type> named \a it, and runs through the SList \a L.
 * See \c #forall_listiterators for an example.
 */
#define forall_slistiterators(type, it, L) \
    for(SListConstIterator< type > it = (L).begin(); it.valid(); ++it)

//! Iteration over all non-const iterators \a it of a list \a L, where L is of Type \c SList<\a type>.
/**
 * Automagically creates a \c SListIterator<\a type> named \a it, and runs through the SList \a L.
 * See \c #forall_listiterators for an example.
 */
#define forall_nonconst_slistiterators(type, it, L) \
    for(SListIterator< type > it = (L).begin(); it.valid(); ++it)




//! The parameterized class \a ListPure<E> represents doubly linked lists with content type \a E.
/**
 * Elements of the list are instances of type ListElement<E>.
 * Use ListConstIterator<E> or ListIterator<E> in order to iterate over the list.
 *
 * In contrast to ListPure<E>, instances of \a List<E> store the length of the list.
 *
 * See the \c #forall_listiterators macros for the recommended way how to easily iterate through a
 * list.
 *
 * @tparam E is the data type stored in list elements.
 */
template<class E>
class List : private ListPure<E>
{

    int m_count; //!< The length of the list.

public:
    //! Constructs an empty doubly linked list.
    List() : m_count(0) { }

    //! Constructs a doubly linked list that is a copy of \a L.
    List(const List<E> & L) : ListPure<E>(L), m_count(L.m_count) { }

    // destruction
    ~List() { }

    typedef E value_type;
    typedef ListElement<E> element_type;
    typedef ListConstIterator<E> const_iterator;
    typedef ListIterator<E> iterator;

    //! Returns true iff the list is empty.
    bool empty() const
    {
        return ListPure<E>::empty();
    }

    // returns length of list
    int size() const
    {
        return m_count;
    }

    // returns first element of list (0 if empty)
    const ListConstIterator<E> begin() const
    {
        return ListPure<E>::begin();
    }
    // returns first element of list (0 if empty)
    ListIterator<E> begin()
    {
        return ListPure<E>::begin();
    }

    // returns iterator to one-past-last element of list
    ListConstIterator<E> end() const
    {
        return ListConstIterator<E>();
    }
    // returns iterator to one-past-last element of list
    ListIterator<E> end()
    {
        return ListIterator<E>();
    }

    // returns last element of list (0 if empty)
    const ListConstIterator<E> rbegin() const
    {
        return ListPure<E>::rbegin();
    }
    // returns last element of list (0 if empty)
    ListIterator<E> rbegin()
    {
        return ListPure<E>::rbegin();
    }

    // returns iterator to one-before-first element of list
    ListConstIterator<E> rend() const
    {
        return ListConstIterator<E>();
    }
    // returns iterator to one-before-first element of list
    ListIterator<E> rend()
    {
        return ListIterator<E>();
    }

    // returns reference to first element
    const E & front() const
    {
        return ListPure<E>::front();
    }
    // returns reference to first element
    E & front()
    {
        return ListPure<E>::front();
    }

    // returns reference to last element
    const E & back() const
    {
        return ListPure<E>::back();
    }
    // returns reference to last element
    E & back()
    {
        return ListPure<E>::back();
    }

    // returns cyclic successor
    ListConstIterator<E> cyclicSucc(ListConstIterator<E> it) const
    {
        return ListPure<E>::cyclicSucc(it);
    }

    // returns cyclic successor
    ListIterator<E> cyclicSucc(ListIterator<E> it)
    {
        return ListPure<E>::cyclicSucc(it);
    }

    // returns cyclic predecessor
    ListConstIterator<E> cyclicPred(ListConstIterator<E> it) const
    {
        return ListPure<E>::cyclicPred(it);
    }

    // returns cyclic predecessor
    ListIterator<E> cyclicPred(ListIterator<E> it)
    {
        return ListPure<E>::cyclicPred(it);
    }

    // returns the iterator at position pos. Note that this takes time linear in pos.
    ListConstIterator<E> get(int pos) const
    {
        OGDF_ASSERT(0 <= pos && pos < m_count)
        return ListPure<E>::get(pos);
    }

    // returns the iterator at position pos. Note that this takes time linear in pos.
    ListIterator<E> get(int pos)
    {
        OGDF_ASSERT(0 <= pos && pos < m_count)
        return ListPure<E>::get(pos);
    }

    //! Returns the position (starting with 0) of iterator \a it in the list.
    /**
     * \pre \a it is a valid iterator pointing to an element in this list!
     */
    int pos(ListConstIterator<E> it) const
    {
        OGDF_ASSERT(it.valid())
        return ListPure<E>::pos(it);
    }

    //! Returns an iterator to a random element in the list (or an invalid iterator if the list is empty)
    /**
     * This method takes linear time.
     */
    ListConstIterator<E> chooseIterator() const
    {
        return (m_count > 0) ? get(randomNumber(0, m_count - 1)) : ListConstIterator<E>();
    }

    //! Returns an iterator to a random element in the list (or an invalid iterator if the list is empty)
    /**
     * This method takes linear time.
     */
    ListIterator<E> chooseIterator()
    {
        return (m_count > 0) ? get(randomNumber(0, m_count - 1)) : ListIterator<E>();
    }

    //! Returns a random element from the list.
    /**
     * \pre The list is not empty!
     *
     * This method takes linear time.
     */
    const E chooseElement() const
    {
        OGDF_ASSERT(!empty());
        return *chooseIterator();
    }

    //! Returns a random element from the list.
    /**
     * \pre The list is not empty!
     *
     * This method takes linear time.
     */
    E chooseElement()
    {
        OGDF_ASSERT(!empty());
        return *chooseIterator();
    }

    // assignment
    List<E> & operator=(const List<E> & L)
    {
        ListPure<E>::operator=(L);
        m_count = L.m_count;
        return *this;
    }

    //! Equality operator.
    bool operator==(const List<E> & L) const
    {
        if(m_count != L.m_count)
            return false;

        ListElement<E>* pX = ListPure<E>::m_head, *pY = L.m_head;
        while(pX != 0)
        {
            if(pX->m_x != pY->m_x)
                return false;
            pX = pX->m_next;
            pY = pY->m_next;
        }
        return true;
    }

    //! Inequality operator.
    bool operator!=(const List<E> & L) const
    {
        return !operator==(L);
    }

    // adds element x at beginning
    ListIterator<E> pushFront(const E & x)
    {
        ++m_count;
        return ListPure<E>::pushFront(x);
    }

    // adds element x at end
    ListIterator<E> pushBack(const E & x)
    {
        ++m_count;
        return ListPure<E>::pushBack(x);
    }

    // inserts x before or after it
    ListIterator<E> insert(const E & x, ListIterator<E> it, Direction dir = after)
    {
        ++m_count;
        return ListPure<E>::insert(x, it, dir);
    }

    // inserts x before it
    ListIterator<E> insertBefore(const E & x, ListIterator<E> it)
    {
        ++m_count;
        return ListPure<E>::insertBefore(x, it);
    }

    // inserts x after it
    ListIterator<E> insertAfter(const E & x, ListIterator<E> it)
    {
        ++m_count;
        return ListPure<E>::insertAfter(x, it);
    }

    // removes first element
    void popFront()
    {
        --m_count;
        ListPure<E>::popFront();
    }

    // removes first element and returns it
    E popFrontRet()
    {
        E el = front();
        popFront();
        return el;
    }

    // removes last element
    void popBack()
    {
        --m_count;
        ListPure<E>::popBack();
    }

    // removes last element and returns it
    E popBackRet()
    {
        E el = back();
        popBack();
        return el;
    }

    void exchange(ListIterator<E> it1, ListIterator<E> it2)
    {
        ListPure<E>::exchange(it1, it2);
    }

    //! Moves \a it to the beginning of the list
    /**
     * \pre \a it points to an element in the list.
     */
    void moveToFront(ListIterator<E> it)
    {
        ListPure<E>::moveToFront(it);
    }
    //! Moves \a it to the end of the list
    /**
     * \pre \a it points to an element in the list.
     */
    void moveToBack(ListIterator<E> it)
    {
        ListPure<E>::moveToBack(it);
    }
    //! Moves \a it after \a itBefore.
    /**
     * \pre \a it and \a itBefore point to elements in this list.
     */
    void moveToSucc(ListIterator<E> it, ListIterator<E> itBefore)
    {
        ListPure<E>::moveToSucc(it, itBefore);
    }
    //! Moves \a it before \a itAfter.
    /**
     * \pre \a it and \a itAfter point to elements in this list.
     */
    void moveToPrec(ListIterator<E> it, ListIterator<E> itAfter)
    {
        ListPure<E>::moveToPrec(it, itAfter);
    }

    //! Moves \a it to the beginning of \a L2.
    /**
     * \pre \a it points to an element in this list.
     */
    void moveToFront(ListIterator<E> it, List<E> & L2)
    {
        ListPure<E>::moveToFront(it, L2);
        --m_count;
        ++L2.m_count;
    }
    //! Moves \a it to the end of \a L2.
    /**
     * \pre \a it points to an element in this list.
     */
    void moveToBack(ListIterator<E> it, List<E> & L2)
    {
        ListPure<E>::moveToBack(it, L2);
        --m_count;
        ++L2.m_count;
    }

    //! Moves \a it to list \a L2 and inserts it after \a itBefore.
    /**
     * \pre \a it points to an element in this list, and \a itBefore
     *      points to an element in \a L2.
     */
    void moveToSucc(ListIterator<E> it, List<E> & L2, ListIterator<E> itBefore)
    {
        ListPure<E>::moveToSucc(it, L2, itBefore);
        --m_count;
        ++L2.m_count;
    }
    //! Moves \a it to list \a L2 and inserts it after \a itBefore.
    /**
     * \pre \a it points to an element in this list, and \a itBefore
     *      points to an element in \a L2.
     */
    void moveToPrec(ListIterator<E> it, List<E> & L2, ListIterator<E> itAfter)
    {
        ListPure<E>::moveToPrec(it, L2, itAfter);
        --m_count;
        ++L2.m_count;
    }

    // removes it and frees memory
    void del(ListIterator<E> it)
    {
        --m_count;
        ListPure<E>::del(it);
    }

    //! Appends \a L2 to this list and makes \a L2 empty.
    void conc(List<E> & L2)
    {
        ListPure<E>::conc(L2);
        m_count += L2.m_count;
        L2.m_count = 0;
    }

    //! Prepends \a L2 to this list and makes \a L2 empty.
    void concFront(List<E> & L2)
    {
        ListPure<E>::concFront(L2);
        m_count += L2.m_count;
        L2.m_count = 0;
    }

    //! Exchanges too complete lists in O(1).
    /**
     * The list's content is moved to L2 and vice versa.
     */
    void exchange(List<E> & L2)
    {
        ListPure<E>::exchange(L2);
        int t = this->m_count;
        this->m_count = L2.m_count;
        L2.m_count = t;
    }

    //! Splits the list at element \a it into lists \a L1 and \a L2.
    /**
     * If \a it is not a null pointer and \a L = x1,...,x{k-1}, \a it,x_{k+1},xn, then
     * \a L1 = x1,...,x{k-1} and \a L2 = \a it,x{k+1},...,xn if \a dir = \c before.
     * If \a it is a null pointer, then \a L1 is made empty and \a L2 = \a L. Finally
     * \a L is made empty if it is not identical to \a L1 or \a L2.
     *
     * \pre \a it points to an element in this list.
     */
    void split(ListIterator<E> it, List<E> & L1, List<E> & L2, Direction dir = before)
    {
        ListPure<E>::split(it, L1, L2, dir);
        int countL = m_count, countL1 = 0;
        for(ListElement<E>* pX = L1.m_head; pX != 0; pX = pX->m_next)
            ++countL1;

        L1.m_count = countL1;
        L2.m_count = countL - countL1;
        if(this->m_head == 0) m_count = 0;
    }

    // reverses the order of the list elements
    void reverse()
    {
        ListPure<E>::reverse();
    }

    // removes all elements from list
    void clear()
    {
        m_count = 0;
        ListPure<E>::clear();
    }

    //! Conversion to const SListPure.
    const ListPure<E> & getListPure() const
    {
        return *this;
    }

    // sorts list using quicksort
    void quicksort()
    {
        ogdf::quicksortTemplate(*this);
    }

    // sorts list using quicksort and parameterized compare element comp
    template<class COMPARER>
    void quicksort(const COMPARER & comp)
    {
        ogdf::quicksortTemplate(*this, comp);
    }

    // sorts list using bucket sort
    void bucketSort(int l, int h, BucketFunc<E> & f)
    {
        ListPure<E>::bucketSort(l, h, f);
    }

    // permutes elements in list randomly
    void permute()
    {
        ListPure<E>::permute(m_count);
    }

    //! Scans the list for the specified element and returns its position in the list, or -1 if not found.
    int search(const E & e) const
    {
        return ListPure<E>::search(e);
    }

    //! Scans the list for the specified element (using the user-defined comparer) and returns its position in the list, or -1 if not found.
    template<class COMPARER>
    int search(const E & e, const COMPARER & comp) const
    {
        return ListPure<E>::search(e, comp);
    }


    OGDF_NEW_DELETE
}; // class List



template<class E>
void ListPure<E>::bucketSort(int l, int h, BucketFunc<E> & f)
{
    if(m_head == m_tail) return;

    Array<ListElement<E> *> head(l, h, 0), tail(l, h);

    ListElement<E>* pX;
    for(pX = m_head; pX; pX = pX->m_next)
    {
        int i = f.getBucket(pX->m_x);
        if(head[i])
            tail[i] = ((pX->m_prev = tail[i])->m_next = pX);
        else
            head[i] = tail[i] = pX;
    }

    ListElement<E>* pY = 0;
    for(int i = l; i <= h; i++)
    {
        pX = head[i];
        if(pX)
        {
            if(pY)
            {
                (pY->m_next = pX)->m_prev = pY;
            }
            else
                (m_head = pX)->m_prev = 0;
            pY = tail[i];
        }
    }

    m_tail = pY;
    pY->m_next = 0;
}


// permutes elements in list randomly; n is the length of the list
template<class E>
void ListPure<E>::permute(const int n)
{
    Array<ListElement<E> *> A(n + 2);
    A[0] = A[n + 1] = 0;

    int i = 1;
    ListElement<E>* pX;
    for(pX = m_head; pX; pX = pX->m_next)
        A[i++] = pX;

    A.permute(1, n);

    for(i = 1; i <= n; i++)
    {
        pX = A[i];
        pX->m_next = A[i + 1];
        pX->m_prev = A[i - 1];
    }

    m_head = A[1];
    m_tail = A[n];
}


// prints list L to output stream os using delimiter delim
template<class E>
void print(ostream & os, const ListPure<E> & L, char delim = ' ')
{
    ListConstIterator<E> pX = L.begin();
    if(pX.valid())
    {
        os << *pX;
        for(++pX; pX.valid(); ++pX)
            os << delim << *pX;
    }
}

// prints list L to output stream os using delimiter delim
template<class E>
void print(ostream & os, const List<E> & L, char delim = ' ')
{
    print(os, L.getListPure(), delim);
}

// prints list L to output stream os
template<class E>
ostream & operator<<(ostream & os, const ListPure<E> & L)
{
    print(os, L);
    return os;
}

// prints list L to output stream os
template<class E>
ostream & operator<<(ostream & os, const List<E> & L)
{
    return os << L.getListPure();
}


} // end namespace ogdf


#endif
