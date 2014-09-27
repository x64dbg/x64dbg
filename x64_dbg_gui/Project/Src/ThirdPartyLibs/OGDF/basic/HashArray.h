/*
 * $Revision: 2615 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 14:23:36 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of HashArray class.
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

#ifndef OGDF_HASH_ARRAY_H
#define OGDF_HASH_ARRAY_H


#include <ogdf/basic/Hashing.h>


namespace ogdf
{


//! Indexed arrays using hashing for element access.
/**
 * @tparam I is the index type.
 * @tparam E is the element type.
 * @tparam H is the hash function type. Optional; its default uses the class DefHashFunc.
 *
 * A hashing array can be used like a usual array but has a general
 * index type.
 *
 * The hashing array is only defined for a subset <I>I<SUB>def</SUB></I> of the
 * index set (set of all elements of the index type). At construction, this set
 * is empty. Whenever an index is assigned an element, this index is added
 * to <I>I<SUB>def</SUB></I>. There are also method for testing if an index
 * is defined (is in <I>I<SUB>def</SUB></I>).
 *
 * <H3>Example</H3>
 * The following code snippet demonstrates how to use a hashing array. First,
 * the example inserts elements into a hashing array simulating a tiny
 * German&ndash;English dictionary, then it prints some elements via array
 * access, and finally it iterates over all defined indices and prints the
 * dictionary entries. We use a the const reference \a Hc, since we want to
 * avoid that array access for undefined indices creates these elements.
 *
 * \code
 *   HashArray<String,String> H("[undefined]");
 *   const HashArray<String,String> &Hc = H;
 *
 *   H["Hund"]  = "dog";
 *   H["Katze"] = "cat";
 *   H["Maus"]  = "mouse";
 *
 *   cout << "Katze:   " << Hc["Katze"]   << endl;
 *   cout << "Hamster: " << Hc["Hamster"] << endl;
 *
 *   cout << "\nAll elements:" << endl;
 *   HashConstIterator<String,String> it;
 *   for(it = Hc.begin(); it.valid(); ++it)
 *     cout << it.key() << " -> " << it.info() << endl;
 * \endcode
 *
 * The produced output is as follows:
 * \code
 * Katze:   cat
 * Hamster: [undefined]
 *
 * All elements:
 * Hund -> dog
 * Maus -> mouse
 * Katze -> cat
 * \endcode
 */
template<class I, class E, class H = DefHashFunc<I> >
class HashArray : private Hashing<I, E, H>
{
    E m_defaultValue; //! The default value for elements.

public:
    //! The type of const-iterators for hash arrays.
    typedef HashConstIterator<I, E, H> const_iterator;

    //! Creates a hashing array; the default value is the default value of the element type.
    HashArray() : Hashing<I, E, H>() { }

    //! Creates a hashing array with default value \a defaultValue.
    HashArray(const E & defaultValue, const H & hashFunc = H())
        : Hashing<I, E, H>(256, hashFunc), m_defaultValue(defaultValue) { }

    //! Copy constructor.
    HashArray(const HashArray<I, E, H> & A) : Hashing<I, E, H>(A), m_defaultValue(A.m_defaultValue) { }

    //! Returns an iterator to the first element in the list of all elements.
    HashConstIterator<I, E, H> begin() const
    {
        return Hashing<I, E, H>::begin();
    }

    //! Returns the number of defined indices (= number of elements in hash table).
    int size() const
    {
        return Hashing<I, E, H>::size();
    }

    //! Returns if any indices are defined (= if the hash table is empty)
    int empty() const
    {
        return Hashing<I, E, H>::empty();
    }


    //! Returns the element with index \a i.
    const E & operator[](const I & i) const
    {
        HashElement<I, E>* pElement = Hashing<I, E, H>::lookup(i);
        if(pElement) return pElement->info();
        else return m_defaultValue;
    }

    //! Returns a reference to the element with index \a i.
    E & operator[](const I & i)
    {
        HashElement<I, E>* pElement = Hashing<I, E, H>::lookup(i);
        if(!pElement) pElement = Hashing<I, E, H>::fastInsert(i, m_defaultValue);
        return pElement->info();
    }

    //! Returns true iff index \a i is defined.
    bool isDefined(const I & i) const
    {
        return Hashing<I, E, H>::member(i);
    }

    //! Undefines index \a i.
    void undefine(const I & i)
    {
        Hashing<I, E, H>::del(i);
    }

    //! Assignment operator.
    HashArray<I, E, H> & operator=(const HashArray<I, E, H> & A)
    {
        m_defaultValue = A.m_defaultValue;
        Hashing<I, E, H>::operator =(A);
        return *this;
    }

    //! Undefines all indices.
    void clear()
    {
        Hashing<I, E, H>::clear();
    }
};


} // end namespace ogdf

#endif
