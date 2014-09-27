/*
 * $Revision: 2615 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 14:23:36 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of bounded stack class.
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

#ifndef OGDF_B_STACK_H
#define OGDF_B_STACK_H


#include <ogdf/basic/basic.h>


namespace ogdf
{

template<class E, class INDEX> class BoundedStack;

// output
template<class E, class INDEX>
void print(ostream & os, const BoundedStack<E, INDEX> & S, char delim = ' ');


//! The parameterized class \a BoundedStack<E,INDEX> implements stacks with bounded size.
/**
 * @tparam E     is the element type.
 * @tparam INDEX is the index type. The default index type is \c int, other possible types
 *               are \c short and <code>long long</code> (on 64-bit systems).
 */
template<class E, class INDEX = int> class BoundedStack
{

    E* m_pTop;   //!< Pointer to top element.
    E* m_pStart; //!< Pointer to first element.
    E* m_pStop;  //!< Pointer to one past last element.

public:
    //! Constructs an empty bounded stack for no elements at all.
    /**
     * The default constructor does not allocate any space for elements; before
     * using the stack, it is required to initialize the stack with init().
     */
    BoundedStack()
    {
        m_pTop = m_pStart = m_pStop = 0;
    }

    //! Constructs an empty bounded stack for at most \a n elements.
    explicit BoundedStack(INDEX n)
    {
        OGDF_ASSERT(n >= 1)
        m_pStart = new E[n];
        if(m_pStart == 0) OGDF_THROW(InsufficientMemoryException);
        m_pTop  = m_pStart - 1;
        m_pStop = m_pStart + n;
    }

    //! Constructs a bounded stack that is a copy of \a S.
    BoundedStack(const BoundedStack<E> & S)
    {
        copy(S);
    }

    // destruction
    ~BoundedStack()
    {
        delete [] m_pStart;
    }

    //! Returns top element.
    const E & top() const
    {
        OGDF_ASSERT(m_pTop != m_pStart - 1)
        return *m_pTop;
    }

    //! Returns top element.
    E & top()
    {
        OGDF_ASSERT(m_pTop != m_pStart - 1)
        return *m_pTop;
    }

    //! Returns current size of the stack.
    INDEX size() const
    {
        return m_pTop - (m_pStart - 1);
    }

    //! Returns true iff the stack is empty.
    bool empty()
    {
        return m_pTop == (m_pStart - 1);
    }

    //! Returns true iff the stack is full.
    bool full()
    {
        return m_pTop == (m_pStop - 1);
    }

    //! Returns true iff the stack was initialized.
    bool valid() const
    {
        return m_pStart != 0;
    }

    //! Returns the capacity of the bounded stack.
    INDEX capacity() const
    {
        return m_pStop - m_pStart;
    }

    //! Reinitializes the stack for no elements at all (actually frees memory).
    void init()
    {
        delete [] m_pStart;
        m_pTop = m_pStart = m_pStart = 0;
    }

    //! Reinitializes the stack for \a n elements.
    void init(INDEX n)
    {
        OGDF_ASSERT(n >= 1)

        delete [] m_pStart;

        m_pStart = new E[n];
        if(m_pStart == 0) OGDF_THROW(InsufficientMemoryException);
        m_pTop = m_pStart - 1;
        m_pStop = m_pStart + n;
    }

    //! Assignment operator.
    BoundedStack<E> & operator=(const BoundedStack & S)
    {
        delete [] m_pStart;
        copy(S);
        return *this;
    }

    //! Adds element \a x as top-most element to the stack.
    void push(const E & x)
    {
        OGDF_ASSERT(m_pTop != m_pStop - 1)
        *++m_pTop = x;
    }

    //! Removes the top-most element from the stack and returns it.
    E pop()
    {
        OGDF_ASSERT(m_pTop != (m_pStart - 1))
        return *m_pTop--;
    }

    //! Makes the stack empty.
    void clear()
    {
        m_pTop = m_pStart - 1;
    }

    //! Prints the stack to output stream \a os.
    void print(ostream & os, char delim = ' ') const
    {
        for(const E* pX = m_pStart; pX != m_pTop;)
            os << *++pX << delim;
    }

private:
    void copy(const BoundedStack<E> & S)
    {
        if(!S.valid())
        {
            m_pTop = m_pStart = m_pStop = 0;
        }
        else
        {
            INDEX sz = S.m_pStop - S.m_pStart;
            m_pStart = new E[sz + 1];
            if(m_pStart == 0) OGDF_THROW(InsufficientMemoryException);
            m_pStop = m_pStart + sz;
            m_pTop  = m_pStart - 1;
            for(E* pX = S.m_pStart - 1; pX != S.m_pTop;)
                *++m_pTop = *++pX;
        }
    }
}; // class BoundedStack



// output operator
template<class E, class INDEX>
ostream & operator<<(ostream & os, const BoundedStack<E, INDEX> & S)
{
    S.print(os);
    return os;
}

} // end namespace ogdf


#endif
