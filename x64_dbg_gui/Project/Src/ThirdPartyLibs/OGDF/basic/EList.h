/*
 * $Revision: 2579 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-11 15:28:17 +0200 (Mi, 11. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of embedded stack and list
 * functionality which is useful for embedded chains of elements
 * (classes with internal next and previous pointer).
 * \par The templates declared in this file can be used whenever
 * a so called embedded list has to be implemented. Example:
 *
 * \author Martin Gronemann
 *
 * \code
 * class MyNode
 * {
 * public:
 * ...
 * private:
 *      MyNode* prev;
 *      MyNode* next;
 * };
 *
 * class MyDataStructure
 * {
 *  private:
 *      MyNode* m_head;
 *      MyNode* m_tail;
 *      int m_numNodes;
 *      MyNode* m_stackTop;
 *  public:
 *      typedef EList<  MyDataStructure,
 *                      MyNode,
 *                      &MyDataStructure::m_numNodes,
 *                      &MyDataStructure::m_head,
 *                      &MyDataStructure::m_tail,
 *                      &MyNode::prev,
 *                      &MyNode::next> NodeChain;
 *
 *      typedef EStack< MyDataStructure,
 *                      MyNode,
 *                      &MyDataStructure::m_stackTop,
 *                      &MyNode::next> NodeStack;
 * };
 *  ...
 *  MyDataStructure ds;
 *  MyDataStructure::NodeChain.init(&ds);
 *  MyDataStructure::NodeStack.init(&ds);
 *  ...
 *  MyNode* ptr = new MyNode();
 *  MyDataStructure::NodeChain.pushBack(&ds, ptr);
 *  ...
 *  ptr = MyDataStructure::NodeChain.front(&ds);
 *  MyDataStructure::NodeStack.push(&ds,ptr);
 *  ...
 *  for (MyDataStructure::NodeChain::iterator it = MyDataStructure::NodeChain.begin(&ds);
 *       it.valid(); it++)
 *  {
 *      ptr = *it;
 *  }
 * \endcode
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

#ifndef OGDF_ELIST_H
#define OGDF_ELIST_H

namespace ogdf
{

// EStack forward declaration
template<typename S, typename E, E* S::*first, E* E::*next> class EStack;
// EListIterator forward declaration
template<typename E, E* E::*prev, E* E::*next> class EListIterator;
// EList forward declaration
template<typename L, typename E, int L::*numElem, E* L::*first, E* L::*last, E* E::*next, E* E::*prev> class EList;

//! The embedded stack class template
template<typename S, typename E, E* S::*first, E* E::*next>
class EStack
{
public:
    //! Initializes \a pStack as empty stack
    static inline void init(S* pStack)
    {
        pStack->*first = 0;
    }

    //! Removes the top element from \a pStack without returning it
    static inline void pop(S* pStack)
    {
        pStack->*first = pStack->*first->*next;
    }

    //! Removes the top element and returns it.
    static inline E* popRet(S* pStack)
    {
        E* res = pStack->*first;
        pStack->*first = pStack->*first->*next;
        return res;
    }

    //! Pushes the new element \a pElem onto \a pStack.
    static inline void push(S* pStack, E* pElem)
    {
        pElem->*next = pStack->*first;
        pStack->*first = pElem;
    }

    //! Returns a pointer to the top element of \a pStack.
    static inline E* top(const S* pStack)
    {
        return pStack->*first;
    }

    //! Returns true if \a pStack is empty
    static inline bool empty(const S* pStack)
    {
        return (pStack->*first == 0);
    }
};


//! Implementation of an embedded list iterator used by \a EList
template<typename E, E* E::*prev, E* E::*next>
class EListIterator
{
public:
    //! Constructs an iterator pointing at NULL
    inline EListIterator() : m_ptr(0) {}

    //! Constructs an iterator pointing at ptr
    inline EListIterator(E* ptr) : m_ptr(ptr) {}

    //! constructs an iterator pointing at the same element as \a other
    inline EListIterator(const EListIterator<E, prev, next> & other) : m_ptr(other.m_ptr) {}

    //! returns false if the iterator points at NULL
    inline bool valid() const
    {
        return (m_ptr != 0);
    }

    //! Equality operator.
    inline bool operator==(const EListIterator<E, prev, next> & other) const
    {
        return m_ptr == other.m_ptr;
    }

    //! Inequality operator.
    inline bool operator!=(const EListIterator<E, prev, next> & other) const
    {
        return m_ptr != other.m_ptr;
    }

    //! Returns successor iterator.
    inline EListIterator<E, prev, next> succ() const
    {
        return m_ptr->*next;
    }

    //! Returns predecessor iterator.
    inline EListIterator<E, prev, next> pred() const
    {
        return m_ptr->*prev;
    }

    //! Returns a reference to the element.
    //E& operator*() const { return *m_ptr; }

    //! Returns a pointer to the element.
    E* operator*() const
    {
        return m_ptr;
    }

    //! Assignment operator.
    inline EListIterator<E, prev, next> & operator=(const EListIterator<E, prev, next> & other)
    {
        m_ptr = other.m_ptr;
        return *this;
    }

    //! Increment operator (prefix).
    inline EListIterator<E, prev, next> & operator++()
    {
        m_ptr = m_ptr->*next;
        return *this;
    }

    //! Increment operator (postfix).
    inline EListIterator<E, prev, next> operator++(int)
    {
        EListIterator<E, prev, next> it = *this;
        m_ptr = m_ptr->*next;
        return it;
    }

    //! Decrement operator (prefix).
    inline EListIterator<E, prev, next> & operator--()
    {
        m_ptr = m_ptr->*prev;
        return *this;
    }

    //! Decrement operator (postfix).
    inline EListIterator<E, prev, next> operator--(int)
    {
        EListIterator<E, prev, next> it = *this;
        m_ptr = m_ptr->*prev;
        return it;
    }

private:
    //! The pointer to the element the iterator is pointing at.
    E* m_ptr;
};


//! The embedded list template.
template <
    typename L,
    typename E,
    int L::* numElem,
    E* L::* first,
    E* L::* last,
    E* E::* next,
    E* E::* prev
    >
class EList
{
public:
    //! The iterator typdef for elements of type E and prev, next pointer
    typedef EListIterator<E, prev, next> iterator;

    //! Initializes \a pList as an empty embedded list.
    static inline void init(L* pList)
    {
        pList->*first = 0;
        pList->*last = 0;
        pList->*numElem = 0;
    }

    //! Returns the number of elements in this embedded list by reading the numElem var.
    static inline int size(const L* pList)
    {
        return (pList->*numElem);
    }

    //! Returns true if /a pList is empty.
    static inline bool empty(const L* pList)
    {
        return (pList->*first == 0);
    }

    //! Returns a pointer to the first element.
    static inline E* front(const L* pList)
    {
        return pList->*first;
    }

    //! Returns a pointer to the last element.
    static inline E* back(const L* pList)
    {
        return pList->*last;
    }

    //! Appends the element \a elem to the end of \a pList.
    static inline iterator pushBack(L* pList, E* elem)
    {
        elem->*next = 0;
        elem->*prev = pList->*last;
        if(pList->*last)
            pList->*last = pList->*last->*next = elem;
        else
            pList->*last = pList->*first = elem;

        (pList->*numElem)++;
        return iterator(elem);
    }

    //! Adds element \a x at the begin of the list.
    static inline iterator pushFront(L* pList, E* elem)
    {
        elem->*next = pList->*first;
        elem->*prev = 0;

        if(pList->*first)
            pList->*first = pList->*first->*prev = elem;
        else
            pList->*first = pList->*last = elem;

        (pList->*numElem)++;

        return iterator(elem);
    }

    //! Inserts \a elem into \a pList before \a pNext
    static inline iterator insertBefore(L* pList, E* elem, E* pNext)
    {
        E* pPrev;

        if(pNext)
            pPrev = pNext->*prev;
        else
            pPrev = 0;

        elem->*next = pNext;
        elem->*prev = pPrev;

        if(pNext)
            pNext->*prev = elem;
        else
            pList->*last = elem;
        if(pPrev)
            pPrev->*next = elem;
        else
            pList->*first = elem;

        (pList->*numElem)++;
        return iterator(elem);
    }

    //! Inserts \a elem into \a pList before position \a itNext
    static inline iterator insertBefore(L* pList, E* elem, const iterator & itNext)
    {
        return insertBefore(pList, elem, (E*)(*itNext));
    }

    //! Inserts \a elem into \a pList before \a pPrev
    static inline iterator insertAfter(L* pList, E* elem, E* pPrev)
    {
        E* pNext;

        if(pPrev)
            pNext = pPrev->*next;
        else
            pNext = 0;

        elem->*next = pNext;
        elem->*prev = pPrev;

        if(pNext)
            pNext->*prev = elem;
        else
            pList->*last = elem;
        if(pPrev)
            pPrev->*next = elem;
        else
            pList->*first = elem;

        (pList->*numElem)++;
        return iterator(elem);
    }

    //! Inserts \a elem into \a pList after position \a itPrev
    static inline iterator insertAfter(L* pList, E* elem, const iterator & itPrev)
    {
        return insertAfter(pList, elem, (E*)(*itPrev));
    }

    //! Removes the first element of \a pList.
    inline static void popFront(L* pList)
    {
        if(front(pList))
            remove(pList, front(pList));
    }

    //! Removes the last element of \a pList.
    inline static void popBack(L* pList)
    {
        if(back(pList))
            remove(pList, back(pList));
    }

    //! Removes \a elem from \a pList.
    static inline iterator remove(L* pList, E* elem)
    {
        E* pPrev = elem->*prev;
        E* pNext = elem->*next;
        if(pPrev)
            pPrev->*next = pNext;
        else
            pList->*first = pNext;
        if(pNext)
            pNext->*prev = pPrev;
        else
            pList->*last = pPrev;

        (pList->*numElem)--;
        return iterator(pNext);
    }

    //! Removes the element \a it is pointing at from \a pList.
    static inline iterator remove(L* pList, const iterator & it)
    {
        return remove(pList, (E*)(*it));
    }

    template <
        typename L_other,
        E* L_other::* other_first,
        E* L_other::* other_last,
        int L_other::* other_numElem
        >
    inline static void appendFrom(L* pList, L_other* pListOther)
    {
        if(!pListOther->*other_first)
            return;

        if(empty(pList))
        {
            pList->*first = pListOther->*other_first;
            pList->*last = pListOther->*other_last;
        }
        else
        {
            // link the pList->last element to other->first
            pList->*last->*next = pListOther->*other_first;
            pListOther->*other_first->*prev = pList->*last;

            // link the pList->last element to other->first
            pList->*last = pListOther->*other_first;
        }

        // add the size of the other pList
        pList->*numElem += pListOther->*other_numElem;
        // clear other pList
        pListOther->*other_numElem = 0;
        pListOther->*other_first = 0;
        pListOther->*other_last = 0;
    }

    //! Returns an iterator pointing at the first element of \a pList.
    static inline iterator begin(const L* pList)
    {
        return iterator(pList->*first);
    }

    //! Returns an iterator pointing at NULL.
    static inline iterator end(const L* pList)
    {
        return iterator();
    }

    //! Returns a reverse iterator pointing at the last element of \a pList.
    static inline iterator rbegin(const L* pList)
    {
        return iterator(pList->*last);
    }

    //! Returns a reverse iterator pointing at NULL.
    static inline iterator rend(const L* pList)
    {
        return iterator();
    }

    template<typename Func>
    static inline void forall(const L* pList, const Func & func)
    {
        for(iterator it = begin(pList); it.valid(); it++)
        {
            func(*it);
        }
    }

    template<typename A1>
    static inline void forall_call(const L* pList, void (E::*func)(A1), const A1 & a1)
    {
        for(iterator it = begin(pList); it.valid(); it++)
        {
            ((*it)->*func)(a1);
        }
    }

    //! Constructor.
    inline EList(L* pList) : m_pList(pList) { }

    inline void init()
    {
        init(m_pList);
    }

    inline int size() const
    {
        return size(m_pList);
    }
    inline bool empty() const
    {
        return empty(m_pList);
    }

    inline E* front() const
    {
        return front(m_pList);
    }
    inline E* back() const
    {
        return back(m_pList);
    }

    inline iterator pushBack(E* elem)
    {
        return pushBack(m_pList, elem);
    }
    inline iterator pushFront(E* elem)
    {
        return pushFront(m_pList, elem);
    }

    inline iterator insertBefore(E* elem, E* pNext)
    {
        return insertBefore(m_pList, elem, pNext);
    }
    inline iterator insertBefore(E* elem, const iterator & it)
    {
        return insertBefore(m_pList, elem, it);
    }

    inline iterator insertAfter(E* elem, E* pPrev)
    {
        return insertAfter(m_pList, elem, pPrev);
    } ;
    inline iterator insertAfter(E* elem, const iterator & it)
    {
        return insertAfter(m_pList, elem, it);
    } ;

    inline void popFront()
    {
        popFront(m_pList);
    }
    inline void popBack()
    {
        popBack(m_pList);
    }

    void operator<<(E* elem)
    {
        pushBack(elem);
    }

    inline iterator remove(E* elem)
    {
        return remove(m_pList, elem);
    }
    inline iterator remove(const iterator & it)
    {
        return remove(m_pList, (E*)(*it));
    }

    inline iterator begin() const
    {
        return begin(m_pList);
    }
    inline iterator end() const
    {
        return end(m_pList);
    }
    inline iterator rbegin() const
    {
        return rbegin(m_pList);
    }
    inline iterator rend() const
    {
        return rend(m_pList);
    }

private:
    L* m_pList;
};


} // end of namespace ogdf

#endif
