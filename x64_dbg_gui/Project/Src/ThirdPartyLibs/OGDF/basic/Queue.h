/*
 * $Revision: 2615 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 14:23:36 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of list-based queues
 *        (classes QueuePure<E> and Queue<E>).
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

#ifndef OGDF_QUEUE_H
#define OGDF_QUEUE_H


#include <ogdf/basic/SList.h>


namespace ogdf
{


//! The parameterized class \a QueuePure<E> implements list-based queues.
/**
 * In contrast to Queue<E>, instances of \a QueuePure<E> do not store the
 * number of elements contained in the queue.
 *
 * @tparam E is the element type.
 */
template<class E> class QueuePure : private SListPure<E>
{
public:
    //! Constructs an empty queue.
    QueuePure() { }

    //! Constructs a queue that is a copy of \a Q.
    QueuePure(const QueuePure<E> & Q) : SListPure<E>(Q) { }

    // destruction
    ~QueuePure() { }

    //! Returns true iff the queue is empty.
    bool empty() const
    {
        return SListPure<E>::empty();
    }

    //! Returns a reference to the front element.
    const E & top() const
    {
        return SListPure<E>::front();
    }

    //! Returns a reference to the front element.
    E & top()
    {
        return SListPure<E>::front();
    }

    //! Returns a reference to the back element.
    const E & bottom() const
    {
        return SListPure<E>::back();
    }

    //! Returns a reference to the back element.
    E & bottom()
    {
        return SListPure<E>::back();
    }

    //! Assignment operator.
    QueuePure<E> & operator=(const QueuePure<E> & Q)
    {
        SListPure<E>::operator=(Q);
        return *this;
    }

    //! Adds \a x at the end of queue.
    SListIterator<E> append(const E & x)
    {
        return SListPure<E>::pushBack(x);
    }

    //! Removes front element and returns it.
    E pop()
    {
        E x = top();
        SListPure<E>::popFront();
        return x;
    }

    //! Makes the queue empty.
    void clear()
    {
        SListPure<E>::clear();
    }

    //! Conversion to const SListPure.
    const SListPure<E> & getListPure() const
    {
        return *this;
    }

    OGDF_NEW_DELETE
}; // class QueuePure


//! The parameterized class \a Queue<E> implements list-based queues.
/**
 * In contrast to QueuePure<E>, instances of \a Queue<E> store the
 * number of elements contained in the queue.
 *
 * @tparam E is the element type.
 */
template<class E> class Queue : private SList<E>
{
public:
    //! Constructs an empty queue.
    Queue() { }

    //! Constructs a queue that is a copy of \a Q.
    Queue(const Queue<E> & Q) : SList<E>(Q) { }

    // destruction
    ~Queue() { }

    //! Returns true iff the queue is empty.
    bool empty() const
    {
        return SList<E>::empty();
    }

    //! Returns the number of elements in the queue.
    int size() const
    {
        return SList<E>::size();
    }

    //! Returns a reference to the front element.
    const E & top() const
    {
        return SList<E>::front();
    }

    //! Returns a reference to the front element.
    E & top()
    {
        return SList<E>::front();
    }

    //! Returns a reference to the back element.
    const E & bottom() const
    {
        return SListPure<E>::back();
    }

    //! Returns a reference to the back element.
    E & bottom()
    {
        return SListPure<E>::back();
    }

    //! Assignment operator.
    Queue<E> & operator=(const Queue<E> & Q)
    {
        SList<E>::operator=(Q);
        return *this;
    }

    //! Adds \a x at the end of queue.
    SListIterator<E> append(const E & x)
    {
        return SList<E>::pushBack(x);
    }

    //! Removes front element and returns it.
    E pop()
    {
        E x = top();
        SList<E>::popFront();
        return x;
    }

    //! Makes the queue empty.
    void clear()
    {
        SList<E>::clear();
    }

    //! Conversion to const SList.
    const SList<E> & getList() const
    {
        return *this;
    }
    //! Conversion to const SListPure.
    const SListPure<E> & getListPure() const
    {
        return SList<E>::getListPure();
    }

    OGDF_NEW_DELETE
}; // class Queue


// prints queue to output stream os using delimiter delim
template<class E>
void print(ostream & os, const QueuePure<E> & Q, char delim = ' ')
{
    print(os, Q.getListPure(), delim);
}

// prints queue to output stream os using delimiter delim
template<class E>
void print(ostream & os, const Queue<E> & Q, char delim = ' ')
{
    print(os, Q.getListPure(), delim);
}


// output operator
template<class E>
ostream & operator<<(ostream & os, const QueuePure<E> & Q)
{
    print(os, Q);
    return os;
}

template<class E>
ostream & operator<<(ostream & os, const Queue<E> & Q)
{
    print(os, Q);
    return os;
}


} // end namespace ogdf


#endif
