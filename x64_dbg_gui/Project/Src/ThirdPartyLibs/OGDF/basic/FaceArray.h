/*
 * $Revision: 2615 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 14:23:36 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief declaration and implementation of FaceArray class
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

#ifndef OGDF_FACE_ARRAY_H
#define OGDF_FACE_ARRAY_H


#include <ogdf/basic/Array.h>
#include <ogdf/basic/CombinatorialEmbedding.h>


namespace ogdf
{


//! Abstract base class for face arrays.
/**
 * Defines the interface for event handling used by the
 * CombinatorialEmbedding class.
 * Use the parameterized class FaceArray for creating face arrays.
 */
class FaceArrayBase
{
    /**
     * Pointer to list element in the list of all registered face
     * arrays which references this array.
     */
    ListIterator<FaceArrayBase*> m_it;

public:
    const ConstCombinatorialEmbedding* m_pEmbedding; //!< The associated combinatorial embedding.

    //! Initializes a face array not associated with a combinatorial embedding.
    FaceArrayBase() : m_pEmbedding(0) { }
    //! Initializes a face array associated with \a pE.
    FaceArrayBase(const ConstCombinatorialEmbedding* pE) : m_pEmbedding(pE)
    {
        if(pE) m_it = pE->registerArray(this);
    }

    // destructor, unregisters the array
    virtual ~FaceArrayBase()
    {
        if(m_pEmbedding) m_pEmbedding->unregisterArray(m_it);
    }

    // event interface used by CombinatorialEmbedding
    //! Virtual function called when table size has to be enlarged.
    virtual void enlargeTable(int newTableSize) = 0;
    //! Virtual function called when table has to be reinitialized.
    virtual void reinit(int initTableSize) = 0;

    //! Associates the array with a new combinatorial embedding.
    void reregister(const ConstCombinatorialEmbedding* pE)
    {
        if(m_pEmbedding) m_pEmbedding->unregisterArray(m_it);
        if((m_pEmbedding = pE) != 0) m_it = pE->registerArray(this);
    }
}; // class FaceArrayBase


//! Dynamic arrays indexed with faces of a combinatorial embedding.
/**
 * Face arrays represent a mapping from faces to data of type \a T.
 * They adjust their table size automatically when the number of faces in the
 * corresponding combinatorial embedding increases.
 *
 * @tparam T is the element type.
 */
template<class T> class FaceArray : private Array<T>, protected FaceArrayBase
{
    T m_x; //!< The default value for array elements.

public:
    //! Constructs an empty face array associated with no combinatorial embedding.
    FaceArray() : Array<T>(), FaceArrayBase() { }
    //! Constructs a face array associated with \a E.
    FaceArray(const ConstCombinatorialEmbedding & E) :
        Array<T>(E.faceArrayTableSize()), FaceArrayBase(&E) { }
    //! Constructs a face array associated with \a E.
    /**
     * @param E is the associated combinatorial embedding.
     * @param x is the default value for all array elements.
     */
    FaceArray(const ConstCombinatorialEmbedding & E, const T & x) :
        Array<T>(0, E.faceArrayTableSize() - 1, x), FaceArrayBase(&E), m_x(x) { }
    //! Constructs an face array that is a copy of \a A.
    /**
     * Associates the array with the same combinatorial embedding as
     * \a A and copies all elements.
     */
    FaceArray(const FaceArray<T> & A) : Array<T>(A), FaceArrayBase(A.m_pEmbedding), m_x(A.m_x) { }

    //! Returns true iff the array is associated with a combinatorial embedding.
    bool valid() const
    {
        return (Array<T>::low() <= Array<T>::high());
    }

    //! Returns a pointer to the associated combinatorial embedding.
    const ConstCombinatorialEmbedding* embeddingOf() const
    {
        return m_pEmbedding;
    }

    //! Returns a reference to the element with index \a f.
    const T & operator[](face f) const
    {
        OGDF_ASSERT(f != 0 && f->embeddingOf() == m_pEmbedding)
        return Array<T>::operator [](f->index());
    }

    //! Returns a reference to the element with index \a f.
    T & operator[](face f)
    {
        OGDF_ASSERT(f != 0 && f->embeddingOf() == m_pEmbedding)
        return Array<T>::operator [](f->index());
    }

    //! Returns a reference to the element with index \a index.
    /**
     * \attention Make sure that \a index is a valid index for a face
     * in the associated combinatorial embedding!
     */
    const T & operator[](int index) const
    {
        return Array<T>::operator [](index);
    }

    //! Returns a reference to the element with index \a index.
    /**
     * \attention Make sure that \a index is a valid index for a face
     * in the associated combinatorial embedding!
     */
    T & operator[](int index)
    {
        return Array<T>::operator [](index);
    }

    //! Assignment operator.
    FaceArray<T> & operator=(const FaceArray<T> & a)
    {
        Array<T>::operator =(a);
        m_x = a.m_x;
        reregister(a.m_pEmbedding);
        return *this;
    }

    //! Reinitializes the array. Associates the array with no combinatorial embedding.
    void init()
    {
        Array<T>::init();
        reregister(0);
    }

    //! Reinitializes the array. Associates the array with \a E.
    void init(const ConstCombinatorialEmbedding & E)
    {
        Array<T>::init(E.faceArrayTableSize());
        reregister(&E);
    }

    //! Reinitializes the array. Associates the array with \a E.
    /**
     * @param E is the associated combinatorial embedding.
     * @param x is the default value.
     */
    void init(const ConstCombinatorialEmbedding & E, const T & x)
    {
        Array<T>::init(0, E.faceArrayTableSize() - 1, m_x = x);
        reregister(&E);
    }

    //! Sets all array elements to \a x.
    void fill(const T & x)
    {
        int high = m_pEmbedding->maxFaceIndex();
        if(high >= 0)
            Array<T>::fill(0, high, x);
    }

private:
    virtual void enlargeTable(int newTableSize)
    {
        Array<T>::grow(newTableSize - Array<T>::size(), m_x);
    }

    virtual void reinit(int initTableSize)
    {
        Array<T>::init(0, initTableSize - 1, m_x);
    }

    OGDF_NEW_DELETE

}; // class FaceArray<T>


} // end namespace ogdf


#endif
