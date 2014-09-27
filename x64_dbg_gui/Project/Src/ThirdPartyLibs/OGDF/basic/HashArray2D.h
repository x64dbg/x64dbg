/*
 * $Revision: 2615 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 14:23:36 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class HashArray2D.
 *
 * This is a class implementing a 2-dimensional Hash array.
 * It uses templates for the keys and the data of the objects
 * stored in it.
 *
 * \author René Weiskircher
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

#include <ogdf/basic/HashArray.h>
#include <ogdf/basic/tuples.h>
#include <ogdf/basic/HashIterator2D.h>

#ifndef OGDF_HASH_ARRAY_2D_H
#define OGDF_HASH_ARRAY_2D_H


namespace ogdf
{


//! Indexed 2-dimensional arrays using hashing for element access.
/**
 * @tparam I1 is the first index type.
 * @tparam I2 is the second index type.
 * @tparam E  is the element type.
 * @tparam H1 is the hash function type for \a I1. Optional; uses the class DefHashFunc by default.
 * @tparam H2 is the hash function type for \a I2. Optional; uses the class DefHashFunc by default.
 *
 * A 2D-hash array can be used like a usual 2-dimensional array but with a general
 * index type.
 */
template <
    class I1,
    class I2,
    class E,
    class H1 = DefHashFunc<I1>,
    class H2 = DefHashFunc<I2> >
class HashArray2D : private Hashing< Tuple2<I1, I2>, E, HashFuncTuple<I1, I2, H1, H2> >
{
public:
    //! The type of const-iterators for 2D-hash arrays.
    typedef HashConstIterator2D<I1, I2, E, H1, H2> const_iterator;

    //! Creates a 2D-hash array.
    HashArray2D() { }

    //! Creates a 2D-hash array and sets the default value to \a x.
    HashArray2D(const E & defaultValue, const H1 & hashFunc1 = H1(), const H2 & hashFunc2 = H2()) :
        Hashing<Tuple2<I1, I2>, E, HashFuncTuple<I1, I2, H1, H2> >(
            256,
            HashFuncTuple<I1, I2, H1, H2>(hashFunc1, hashFunc2)),
        m_defaultValue(defaultValue) { }

    //! Copy constructor.
    HashArray2D(const HashArray2D<I1, I2, E, H1, H2> & A) :
        Hashing<Tuple2<I1, I2>, E, HashFuncTuple<I1, I2, H1, H2> >(A),
        m_defaultValue(A.m_defaultValue) { }

    //! Assignment operator.
    HashArray2D & operator=(const HashArray2D<I1, I2, E, H1, H2> & A)
    {
        m_defaultValue = A.m_defaultValue;
        Hashing<Tuple2<I1, I2>, E, HashFuncTuple<I1, I2, H1, H2> >::operator=(A);

        return *this;
    }

    ~HashArray2D() { }

    //! Returns a const reference to entry (\a i,\a j).
    const E & operator()(const I1 & i, const I2 & j) const
    {
        HashElement<Tuple2<I1, I2>, E>* pElement =
            Hashing<Tuple2<I1, I2>, E, HashFuncTuple<I1, I2, H1, H2> >::lookup(Tuple2<I1, I2>(i, j));
        return (pElement) ? pElement->info() : m_defaultValue;
    }

    //! Returns a reference to entry (\a i,\a j).
    E & operator()(const I1 & i, const I2 & j)
    {
        Tuple2<I1, I2> t(i, j);
        HashElement<Tuple2<I1, I2>, E>* pElement =
            Hashing<Tuple2<I1, I2>, E, HashFuncTuple<I1, I2, H1, H2> >::lookup(t);
        if(!pElement)
            pElement = Hashing<Tuple2<I1, I2>, E, HashFuncTuple<I1, I2, H1, H2> >::fastInsert(t, m_defaultValue);
        return pElement->info();
    }

    //! Returns true iff entry (\a i,\a j) is defined.
    bool isDefined(const I1 & i, const I2 & j) const
    {
        return Hashing<Tuple2<I1, I2>, E, HashFuncTuple<I1, I2, H1, H2> >::member(Tuple2<I1, I2>(i, j));
    }

    //! Undefines the entry at index (\a i,\a j).
    void undefine(const I1 & i, const I2 & j)
    {
        return Hashing<Tuple2<I1, I2>, E, HashFuncTuple<I1, I2, H1, H2> >::del(Tuple2<I1, I2>(i, j));
    }

    //! Returns an iterator pointing to the first element.
    HashConstIterator2D<I1, I2, E, H1, H2> begin() const
    {
        return HashConstIterator2D<I1, I2, E>(
                   Hashing<Tuple2<I1, I2>, E, HashFuncTuple<I1, I2, H1, H2> >::begin());
    }

    //! Returns the number of defined elements in the table.
    int size() const
    {
        return Hashing<Tuple2<I1, I2>, E, HashFuncTuple<I1, I2, H1, H2> >::size();
    }

    //! Returns if any indices are defined
    int empty() const
    {
        return Hashing<Tuple2<I1, I2>, E, HashFuncTuple<I1, I2, H1, H2> >::empty();
    }


    //! Undefines all indices.
    void clear()
    {
        Hashing<Tuple2<I1, I2>, E, HashFuncTuple<I1, I2, H1, H2> >::clear();
    }

private:
    E m_defaultValue; //!< The default value of the array.
};

}

#endif
