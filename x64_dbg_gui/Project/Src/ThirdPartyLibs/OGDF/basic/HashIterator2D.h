/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class HashIterator2D.
 *
 * This class implements an iterator for the HashArray2D.
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


#ifndef OGDF_HASHITERATOR2D_H
#define OGDF_HASHITERATOR2D_H


namespace ogdf
{


/**
 * \brief Const-iterator for 2D-hash arrays.
 *
 */
template< class I1_, class I2_, class E_,
          class Hash1_ = DefHashFunc<I1_>,
          class Hash2_ = DefHashFunc<I2_> >
class HashConstIterator2D :
    private HashConstIterator<Tuple2<I1_, I2_>, E_, HashFuncTuple<I1_, I2_, Hash1_, Hash2_> >
{
public:
    //! Creates an (invalid) iterator.
    HashConstIterator2D() { }

    //! Copy constructor.
    HashConstIterator2D(const HashConstIterator2D<I1_, I2_, E_, Hash1_, Hash2_> & it)
        : HashConstIterator<Tuple2<I1_, I2_>, E_, HashFuncTuple<I1_, I2_, Hash1_, Hash2_> >(it) { }

    //! Copy constructor (from HashConstIterator).
    HashConstIterator2D(const HashConstIterator<Tuple2<I1_, I2_>, E_, HashFuncTuple<I1_, I2_, Hash1_, Hash2_> > & it)
        : HashConstIterator<Tuple2<I1_, I2_>, E_, HashFuncTuple<I1_, I2_, Hash1_, Hash2_> >(it) { }

    //! Assignemnt operator.
    HashConstIterator2D<I1_, I2_, E_, Hash1_, Hash2_> &
    operator=(const HashConstIterator2D<I1_, I2_, E_, Hash1_, Hash2_> & it)
    {
        HashConstIterator<Tuple2<I1_, I2_>, E_, HashFuncTuple<I1_, I2_, Hash1_, Hash2_> >::operator=(it);
        return *this;
    }

    //! Returns true iff the iterator points to an element.
    bool valid() const
    {
        return HashConstIterator<Tuple2<I1_, I2_>, E_, HashFuncTuple<I1_, I2_, Hash1_, Hash2_> >::valid();
    }

    //! Returns the first key of the hash element pointed to.
    const I1_ & key1() const
    {
        return HashConstIterator<Tuple2<I1_, I2_>, E_, HashFuncTuple<I1_, I2_, Hash1_, Hash2_> >::key().x1();
    }

    //! Returns the second key of the hash element pointed to.
    const I2_ & key2() const
    {
        return HashConstIterator<Tuple2<I1_, I2_>, E_, HashFuncTuple<I1_, I2_, Hash1_, Hash2_> >::key().x2();
    }

    //! Returns the information of the element pointed to.
    const E_ & info() const
    {
        return HashConstIterator<Tuple2<I1_, I2_>, E_, HashFuncTuple<I1_, I2_, Hash1_, Hash2_> >::info();
    }

    //! Sets the iterator to the next element in the 2D-hash array.
    HashConstIterator2D<I1_, I2_, E_, Hash1_, Hash2_> & operator++()
    {
        HashConstIterator<Tuple2<I1_, I2_>, E_, HashFuncTuple<I1_, I2_, Hash1_, Hash2_> >::operator++();
        return *this;
    }
};

}



#endif

