/*
 * $Revision: 2615 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 14:23:36 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of bounded queue class.
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

#ifndef OGDF_B_QUEUE_H
#define OGDF_B_QUEUE_H


#include <ogdf/basic/basic.h>


namespace ogdf
{

template<class E, class INDEX> class BoundedQueue;

// output
template<class E, class INDEX>
void print(ostream & os, const BoundedQueue<E, INDEX> & S, char delim = ' ');


//! The parameterized class \a BoundedQueue<E,INDEX> implements queues with bounded size.
/**
 * @tparam E     is the element type.
 * @tparam INDEX is the index type. The default index type is \c int, other possible types
 *               are \c short and <code>long long</code> (on 64-bit systems).
 */
template<class E, class INDEX = int> class BoundedQueue
{

    E* m_pStart; //! Pointer to first element of current sequence.
    E* m_pEnd;   //! Pointer to one past last element of current sequence.
    E* m_pStop;  //! Pointer to one past last element of total array.
    E* m_pFirst; //! Pointer to  first element of total array.

public:
    //! Constructs an empty bounded queue for at most \a n elements.
    explicit BoundedQueue(INDEX n)
    {
        OGDF_ASSERT(n >= 1)
        m_pStart = m_pEnd = m_pFirst = new E[n + 1];
        if(m_pFirst == 0) OGDF_THROW(InsufficientMemoryException);

        m_pStop = m_pFirst + n + 1;
    }

    //! Constructs a bounded queue that is a copy of \a Q.
    BoundedQueue(const BoundedQueue<E> & Q)
    {
        copy(Q);
    }

    // destruction
    ~BoundedQueue()
    {
        delete [] m_pFirst;
    }

    //! Returns front element.
    const E & top() const
    {
        OGDF_ASSERT(m_pStart != m_pEnd)
        return *m_pStart;
    }

    //! Returns front element.
    E & top()
    {
        OGDF_ASSERT(m_pStart != m_pEnd)
        return *m_pStart;
    }

    //! Returns back element.
    const E & bottom() const
    {
        OGDF_ASSERT(m_pStart != m_pEnd)
        if(m_pEnd == m_pFirst) return *(m_pStop - 1);
        else return *(m_pEnd - 1);
    }

    //! Returns back element.
    E & bottom()
    {
        OGDF_ASSERT(m_pStart != m_pEnd)
        if(m_pEnd == m_pFirst) return *(m_pStop - 1);
        else return *(m_pEnd - 1);
    }

    //! Returns current size of the queue.
    INDEX size() const
    {
        return (m_pEnd >= m_pStart) ?
               (m_pEnd - m_pStart) :
               (m_pEnd - m_pFirst) + (m_pStop - m_pStart);
    }

    //! Returns the capacity of the bounded queue.
    INDEX capacity() const
    {
        return (m_pStop - m_pFirst) - 1;
    }

    //! Returns true iff the queue is empty.
    bool empty()
    {
        return m_pStart == m_pEnd;
    }

    //! Returns true iff the queue is full.
    bool full()
    {
        INDEX h = m_pEnd - m_pStart
                  return (h >= 0) ?
                         (h == m_pStop - m_pFirst - 1) :
                         (h == -1);
    }

    //! Assignment operator.
    BoundedQueue<E> & operator=(const BoundedQueue<E> & Q)
    {
        delete [] m_pFirst;
        copy(Q);
        return *this;
    }

    //! Adds \a x at the end of queue.
    void append(const E & x)
    {
        *m_pEnd++ = x;
        if(m_pEnd == m_pStop) m_pEnd = m_pFirst;
        OGDF_ASSERT(m_pStart != m_pEnd)
    }

    //! Removes front element and returns it.
    E pop()
    {
        OGDF_ASSERT(m_pStart != m_pEnd)
        E x = *m_pStart++;
        if(m_pStart == m_pStop) m_pStart = m_pFirst;
        return x;
    }

    //! Makes the queue empty.
    void clear()
    {
        m_pStart = m_pEnd = m_pFirst;
    }

    //! Prints the queue to output stream \a os.
    void print(ostream & os, char delim = ' ') const
    {
        for(const E* pX = m_pStart; pX != m_pEnd;)
        {
            if(pX != m_pStart) os << delim;
            os << *pX;
            if(++pX == m_pStop) pX = m_pFirst;
        }
    }

private:
    void copy(const BoundedQueue<E> & Q)
    {
        int n = Q.size() + 1;
        m_pEnd = m_pStart = m_pFirst = new E[n];
        if(m_pFirst == 0) OGDF_THROW(InsufficientMemoryException);

        m_pStop = m_pStart + n;
        for(E* pX = Q.m_pStart; pX != Q.m_pEnd;)
        {
            *m_pEnd++ = *pX++;
            if(pX == Q.m_pStop) pX = Q.m_pFirst;
        }
    }
}; // class BoundedQueue



// output operator
template<class E, class INDEX>
ostream & operator<<(ostream & os, const BoundedQueue<E, INDEX> & Q)
{
    Q.print(os);
    return os;
}

} // end namespace ogdf


#endif
