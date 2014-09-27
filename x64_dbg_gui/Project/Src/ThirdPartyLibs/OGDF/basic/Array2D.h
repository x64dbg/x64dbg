/*
 * $Revision: 2615 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 14:23:36 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of class Array2D which
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

#ifndef OGDF_ARRAY2D_H
#define OGDF_ARRAY2D_H


#include <ogdf/basic/basic.h>
#include <math.h>


namespace ogdf
{


//! The parameterized class \a Array2D<E> implements dynamic two-dimensional arrays.
/**
 * @tparam E denotes the element type.
 */
template<class E> class Array2D
{
public:
    // constructors

    //! Creates a two-dimensional array with empty index set.
    Array2D()
    {
        construct(0, -1, 0, -1);
    }

    //! Creates a two-dimensional array with index set [\a a..\a b]*[\a c..\a d].
    Array2D(int a, int b, int c, int d)
    {
        construct(a, b, c, d);
        initialize();
    }

    //! Creates a two-dimensional array with index set [\a a..\a b]*[\a c..\a d] and initailizes all elements with \a x.
    Array2D(int a, int b, int c, int d, const E & x)
    {
        construct(a, b, c, d);
        initialize(x);
    }

    //! Creates a two-dimensional array that is a copy of \a A.
    Array2D(const Array2D<E> & array2)
    {
        copy(array2);
    }

    // destructor
    ~Array2D()
    {
        deconstruct();
    }

    //! Returns the minimal array index in dimension 1.
    int low1() const
    {
        return m_a;
    }

    //! Returns the maximal array index in dimension 1.
    int high1() const
    {
        return m_b;
    }

    //! Returns the minimal array index in dimension 2.
    int low2() const
    {
        return m_c;
    }

    //! Returns the maximal array index in dimension 2.
    int high2() const
    {
        return m_d;
    }

    //! Returns the size (number of elements) of the array.
    int size() const
    {
        return size1() * size2();
    }

    //! Returns the length of the index interval (number of entries) in dimension 1.
    int size1() const
    {
        return m_b - m_a + 1;
    }

    //! Returns the length of the index interval (number of entries) in dimension 2.
    int size2() const
    {
        return m_lenDim2;
    }

    //! Returns the determinant of the matrix
    /*! \note use only for square matrices and floating point values */
    float det() const;

    //! Returns a reference to the element with index (\a i,\a j).
    const E & operator()(int i, int j) const
    {
        OGDF_ASSERT(m_a <= i && i <= m_b && m_c <= j && j <= m_d);
        return m_vpStart[(i - m_a) * m_lenDim2 + j];
    }

    //! Returns a reference to the element with index (\a i,\a j).
    E & operator()(int i, int j)
    {
        OGDF_ASSERT(m_a <= i && i <= m_b && m_c <= j && j <= m_d);
        return m_vpStart[(i - m_a) * m_lenDim2 + j];
    }

    //! Reinitializes the array to an array with empty index set.
    void init()
    {
        init(0, -1, 0, -1);
    }

    //! Reinitializes the array to an array with index set [\a a..\a b]*[\a c,\a d].
    void init(int a, int b, int c, int d)
    {
        deconstruct();
        construct(a, b, c, d);
        initialize();
    }

    //! Reinitializes the array to an array with index set [\a a..\a b]*[\a c,\a d] and initializes all entries with \a x.
    void init(int a, int b, int c, int d, const E & x)
    {
        deconstruct();
        construct(a, b, c, d);
        initialize(x);
    }

    //! Assignment operator.
    Array2D<E> & operator=(const Array2D<E> & array2)
    {
        deconstruct();
        copy(array2);
        return *this;
    }

    //! Sets all elements to \a x.
    void fill(const E & x)
    {
        E* pDest = m_pStop;
        while(pDest > m_pStart)
            *--pDest = x;
    }

private:
    E*   m_vpStart; //!< The virtual start of the array (address of A[0,0]).
    int  m_a; //!< The lowest index in dimension 1.
    int  m_lenDim2; //!< The  number of elements in dimension 2.
    E*   m_pStart; //!< The real start of the array (address of A[low1,low2]).
    E*   m_pStop; //!< Successor of last element (address of A[high1,high2+1]).
    int  m_b; //!< The highest index in dimension 1.
    int  m_c; //!< The lowest index in dimension 2.
    int  m_d; //!< The highest index in dimension 2.

    void construct(int a, int b, int c, int d);

    void initialize();
    void initialize(const E & x);

    void deconstruct();

    void copy(const Array2D<E> & array2);

};



template<class E>
void Array2D<E>::construct(int a, int b, int c, int d)
{
    m_a = a;
    m_b = b;
    m_c = c;
    m_d = d;

    int lenDim1 = b - a + 1;
    m_lenDim2   = d - c + 1;

    if(lenDim1 < 1 || m_lenDim2 < 1)
    {
        m_pStart = m_vpStart = m_pStop = 0;

    }
    else
    {
        int len = lenDim1 * m_lenDim2;
        m_pStart = (E*)malloc(len * sizeof(E));
        if(m_pStart == 0)
            OGDF_THROW(InsufficientMemoryException);

        m_vpStart = m_pStart - c;
        m_pStop   = m_pStart + len;
    }
}


template<class E>
void Array2D<E>::initialize()
{
    E* pDest = m_pStart;
    try
    {
        for(; pDest < m_pStop; pDest++)
            new(pDest) E;
    }
    catch(...)
    {
        while(--pDest >= m_pStart)
            pDest->~E();
        free(m_pStart);
        throw;
    }
}


template<class E>
void Array2D<E>::initialize(const E & x)
{
    E* pDest = m_pStart;
    try
    {
        for(; pDest < m_pStop; pDest++)
            new(pDest) E(x);
    }
    catch(...)
    {
        while(--pDest >= m_pStart)
            pDest->~E();
        free(m_pStart);
        throw;
    }
}


template<class E>
void Array2D<E>::deconstruct()
{
    if(doDestruction((E*)0))
    {
        for(E* pDest = m_pStart; pDest < m_pStop; pDest++)
            pDest->~E();
    }
    free(m_pStart);
}


template<class E>
void Array2D<E>::copy(const Array2D<E> & array2)
{
    construct(array2.m_a, array2.m_b, array2.m_c, array2.m_d);

    if(m_pStart != 0)
    {
        E* pSrc  = array2.m_pStop;
        E* pDest = m_pStop;
        while(pDest > m_pStart)
            new(--pDest) E(*--pSrc);
    }
}



template<class E>
float Array2D<E>::det() const
{
    int a = m_a;
    int b = m_b;
    int c = m_c;
    int d = m_d;
    int m = m_b - m_a + 1;
    int n = m_lenDim2;

    int i, j;
    int rem_i, rem_j, column;

    float determinant = 0.0;

    OGDF_ASSERT(m == n);

    switch(n)
    {
    case 0:
        break;
    case 1:
        determinant = (float)((*this)(a, c));
        break;
    case 2:
        determinant = (float)((*this)(a, c) * (*this)(b, d) - (*this)(a, d) * (*this)(b, c));
        break;

    // Expanding along the first row (Laplace's Formula)
    default:
        Array2D<E> remMatrix(0, n - 2, 0, n - 2);         // the remaining matrix
        for(column = c; column <= d; column++)
        {
            rem_i = 0;
            rem_j = 0;
            for(i = a; i <= b; i++)
            {
                for(j = c; j <= d; j++)
                {
                    if(i != a && j != column)
                    {
                        remMatrix(rem_i, rem_j) = (*this)(i, j);
                        if(rem_j < n - 2)
                        {
                            rem_j++;
                        }
                        else
                        {
                            rem_i++;
                            rem_j = 0;
                        }
                    }
                }
            }
            determinant += pow(-1.0, (a + column)) * (*this)(a, column) * remMatrix.det();
        }
    }

    return determinant;
}



} // end namespace ogdf


#endif
