/*
 * $Revision: 2615 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 14:23:36 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of ArrayBuffer class.
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

#ifndef OGDF_ARRAY_BUFFER_H
#define OGDF_ARRAY_BUFFER_H

#include <ogdf/basic/Array.h>

namespace ogdf
{

//! An array that keeps track of the number of inserted elements; also usable as an efficient stack.
/**
 * This is a growable array (with some initial size \a s) which starts out being empty. Using
 * stack functions you can put elements into and out of it. The initial array size is automatically
 * expanded if neccessary, but never automatically shrunken. You may also access the elements it
 * contains using the []-operator. Tha valid indices are 0..(\a s - 1).
 *
 * @tparam E     denotes the element type.
 * @tparam INDEX denotes the index type. The index type must be chosen such that it can
 *               express the whole index range of the array instance, as well as its size.
 *               The default index type is \c int, other possible types are \c short and
 *               <code>long long</code> (on 64-bit systems).
 */
template<class E, class INDEX = int>
class ArrayBuffer : private Array<E, INDEX>
{
    INDEX num; //!< The number of elements in te buffer
public:
    //! Constructs an empty ArrayBuffer, without initial memory allocation.
    ArrayBuffer() : Array<E, INDEX>(), num(0) {}
    //! Constructs an empty ArrayBuffer, allocating memory for up to \a size elements.
    explicit ArrayBuffer(INDEX size) : Array<E, INDEX>(size), num(0) {}

    //! Reinitializes the array, clearing it, and without initial memory allocation.
    void init()
    {
        Array<E, INDEX>::init();
    }
    //! Reinitializes the array, clearing it, and allocating memory for up to \a size elements.
    void init(INDEX size)
    {
        Array<E, INDEX>::init(size);
    }

    //! Clears the buffer
    void clear()
    {
        num = 0;
    }

    //! Returns the newest element of the buffer.
    const E & top() const
    {
        OGDF_ASSERT(num > 0);
        return Array<E, INDEX>::operator[](num - 1);
    }
    //! Returns the newest element of the buffer.
    E & top()
    {
        OGDF_ASSERT(num > 0);
        return Array<E, INDEX>::operator[](num - 1);
    }

    //! Puts a new element in the buffer.
    void push(E e)
    {
        if(num == Array<E, INDEX>::size())
            Array<E, INDEX>::grow(max(num, 1)); // double the size
        Array<E, INDEX>::operator[](num++) = e;
    }

    //! Removes the newest element from the buffer.
    void pop()
    {
        OGDF_ASSERT(num > 0);
        --num;
    }
    //! Removes the newest element from the buffer and returns it.
    E popRet()
    {
        OGDF_ASSERT(num > 0);
        return Array<E, INDEX>::operator[](--num);
    }

    //! Returns true if the buffer is empty, false otherwise.
    bool empty() const
    {
        return !num;
    }

    //! Returns number of elements in the buffer.
    INDEX size() const
    {
        return num;
    }

    //! Returns a pointer to the first element.
    E* begin()
    {
        return Array<E, INDEX>::begin();
    }

    //! Returns a pointer to the first element.
    const E* begin() const
    {
        return Array<E, INDEX>::begin();
    }

    //! Returns a pointer to one past the last element.
    E* end()
    {
        return Array<E, INDEX>::begin() + num;
    }

    //! Returns a pointer to one past the last element.
    const E* end() const
    {
        return Array<E, INDEX>::begin() + num;
    }

    //! Returns a pointer to the last element.
    E* rbegin()
    {
        return Array<E, INDEX>::begin() + (num - 1);
    }

    //! Returns a pointer to the last element.
    const E* rbegin() const
    {
        return Array<E, INDEX>::begin() + (num - 1);
    }

    //! Returns a pointer to one before the first element.
    E* rend()
    {
        return Array<E, INDEX>::rend();
    }

    //! Returns a pointer to one before the first element.
    const E* rend() const
    {
        return Array<E, INDEX>::rend();
    }

    //! Returns a reference to the element at position \a i.
    const E & operator[](INDEX i) const
    {
        OGDF_ASSERT(0 <= i && i < num)
        return Array<E, INDEX>::operator[](i);
    }
    //! Returns a reference to the element at position \a i.
    E & operator[](INDEX i)
    {
        OGDF_ASSERT(0 <= i && i < num)
        return Array<E, INDEX>::operator[](i);
    }

    //! Generates a compact copy holding the current elements.
    /**
     * Creates a copy of the ArrayBuffer and stores it into
     * the given Array \a A.
     * \a A has exactly the neccessary size to hold all
     * elements in the buffer.
     *
     * This method uses an elementwise operator=.
     * If you need a bitcopy of the buffer, use compactMemcpy()
     * instead; if you need a traditional array copy (using the Array's
     * copy-constructor) use compactCpyCon() instead.
     */
    void compactCopy(Array<E, INDEX> & A2) const
    {
        OGDF_ASSERT(this != &A2);
        if(num)
        {
            A2.init(num);
            for(INDEX i = num; i-- > 0;)
                A2[i] = (*this)[i];
        }
        else
            A2.init(0);
    }

    //! Generates a compact copy holding the current elements.
    /**
     * Creates a copy of the ArrayBuffer and stores it into
     * the given Array \a A.
     * \a A has exactly the neccessary size to hold all
     * elements in the buffer
     *
     * This method uses the Array's copy constructur. If you
     * need a bitcopy of the buffer, use compactMemcpy()
     * instead; if you neeed a elementwise operator=-copy, use
     * compactCopy() instead.
     */
    void compactCpycon(Array<E, INDEX> & A2) const
    {
        OGDF_ASSERT(this != &A2);
        if(num)
        {
            INDEX tmp = Array<E, INDEX>::m_high; // thank god i'm a friend of Array
            Array<E, INDEX>::m_high = num - 1; // fake smaller size
            A2.copy(*this); // copy
            Array<E, INDEX>::m_high = tmp;
        }
        else
            A2.init(0);
    }

    //! Generates a compact copy holding the current elements.
    /**
     * Creates a copy of the ArrayBuffer and stores it into
     * the given Array \a A.
     * \a A has exactly the neccessary size to hold all
     * elements in the buffer.
     *
     * This method uses memcpy. If you need a traditional
     * arraycopy using a copy constructur, use compactCopy()
     * instead; if you neeed a elementwise operator=-copy, use
     * compactCopy() instead.
     */
    void compactMemcpy(Array<E, INDEX> & A2) const
    {
        OGDF_ASSERT(this != &A2);
        if(num)
        {
            A2.init(num);
            memcpy(A2.m_pStart, this->m_pStart, sizeof(E)*num);
        }
        else
            A2.init(0);
    }

    //! Performs a linear search for element \a x.
    /**
     * Warning: linear running time!
     * Note that the linear search runs from back to front.
     * \return the index of the found element, and low()-1 if not found.
     */
    INDEX linearSearch(const E & x) const
    {
        INDEX i;
        for(i = num; i-- > 0;)
            if(x == Array<E, INDEX>::m_vpStart[i]) break;
        return i;
    }

    //! Performs a linear search for element \a x with comparer \a comp.
    /**
     * Warning: linear running time!
     * Note that the linear search runs from back to front.
     * \return the index of the found element, and low()-1 if not found.
     */
    template<class COMPARER>
    INDEX linearSearch(const E & x, const COMPARER & comp) const
    {
        INDEX i;
        for(i = num; i-- > 0;)
            if(comp.equal(x, Array<E, INDEX>::m_vpStart[i])) break;
        return i;
    }

    OGDF_NEW_DELETE
};

} //namespace

#endif // OGDF_ARRAY_BUFFER_H
