/*
 * $Revision: 2615 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 14:23:36 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of class Tuple2, Tuple3
 *        and Tuple4.
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

#ifndef OGDF_TUPLE_H
#define OGDF_TUPLE_H


#include <ogdf/basic/basic.h>
#include <ogdf/basic/Hashing.h>


namespace ogdf
{

//! Tuples of two elements (2-tuples).
/**
 * @tparam E1 is the data type for the first element.
 * @tparam E2 is the data type for the second element.
 */
template<class E1, class E2> class Tuple2
{
public:
    E1 m_x1; //!< The first element.
    E2 m_x2; //!< The second element.

    //! Constructs a 2-tuple using default constructors.
    Tuple2() { }
    //! Constructs a 2-tuple for given values.
    Tuple2(const E1 & y1, const E2 & y2) : m_x1(y1), m_x2(y2) { }
    //! Constructs a 2-tuple that is a copy of \a t2.
    Tuple2(const Tuple2<E1, E2> & t2) : m_x1(t2.m_x1), m_x2(t2.m_x2) { }

    //! Returns a reference the first element.
    const E1 & x1() const
    {
        return m_x1;
    }
    //! Returns a reference the second element.
    const E2 & x2() const
    {
        return m_x2;
    }

    //! Returns a reference the first element.
    E1 & x1()
    {
        return m_x1;
    }
    //! Returns a reference the second element.
    E2 & x2()
    {
        return m_x2;
    }

    // default assignment operator

    OGDF_NEW_DELETE
};

//! Equality operator for 2-tuples
template<class E1, class E2>
bool operator==(const Tuple2<E1, E2> & t1, const Tuple2<E1, E2> & t2)
{
    return t1.x1() == t2.x1() && t1.x2() == t2.x2();
}

//! Inequality operator for 2-tuples
template<class E1, class E2>
bool operator!=(const Tuple2<E1, E2> & t1, const Tuple2<E1, E2> & t2)
{
    return t1.x1() != t2.x1() || t1.x2() != t2.x2();
}

//! Output operator for 2-tuples.
template<class E1, class E2>
ostream & operator<<(ostream & os, const Tuple2<E1, E2> & t2)
{
    os << "(" << t2.x1() << " " << t2.x2() << ")";
    return os;
}


//! Tuples of three elements (3-tuples).
/**
 * @tparam E1 is the data type for the first element.
 * @tparam E2 is the data type for the second element.
 * @tparam E3 is the data type for the third element.
 */
template<class E1, class E2, class E3> class Tuple3
{
public:
    E1 m_x1; //!< The first element.
    E2 m_x2; //!< The second element.
    E3 m_x3; //!< The third element.

    //! Constructs a 3-tuple using default constructors.
    Tuple3() { }
    //! Constructs a 3-tuple for given values.
    Tuple3(const E1 & y1, const E2 & y2, const E3 & y3) :
        m_x1(y1), m_x2(y2), m_x3(y3) { }
    //! Constructs a 3-tuple that is a copy of \a t3.
    Tuple3(const Tuple3<E1, E2, E3> & t3) :
        m_x1(t3.m_x1), m_x2(t3.m_x2), m_x3(t3.m_x3) { }

    //! Returns a reference the first element.
    const E1 & x1() const
    {
        return m_x1;
    }
    //! Returns a reference the second element.
    const E2 & x2() const
    {
        return m_x2;
    }
    //! Returns a reference the third element.
    const E3 & x3() const
    {
        return m_x3;
    }

    //! Returns a reference the first element.
    E1 & x1()
    {
        return m_x1;
    }
    //! Returns a reference the second element.
    E2 & x2()
    {
        return m_x2;
    }
    //! Returns a reference the third element.
    E3 & x3()
    {
        return m_x3;
    }

    // default assignment operator

    OGDF_NEW_DELETE
};

//! Equality operator for 3-tuples
template<class E1, class E2, class E3>
bool operator==(const Tuple3<E1, E2, E3> & t1, const Tuple3<E1, E2, E3> & t2)
{
    return t1.x1() == t2.x1() && t1.x2() == t2.x2() && t1.x3() == t2.x3();
}

//! Inequality operator for 3-tuples
template<class E1, class E2, class E3>
bool operator!=(const Tuple3<E1, E2, E3> & t1, const Tuple3<E1, E2, E3> & t2)
{
    return t1.x1() != t2.x1() || t1.x2() != t2.x2() || t1.x3() != t2.x3();
}

//! Output operator for 3-tuples
template<class E1, class E2, class E3>
ostream & operator<<(ostream & os, const Tuple3<E1, E2, E3> & t3)
{
    os << "(" << t3.x1() << " " << t3.x2() << " " << t3.x3() << ")";
    return os;
}


//! Tuples of four elements (4-tuples).
/**
 * @tparam E1 is the data type for the first element.
 * @tparam E2 is the data type for the second element.
 * @tparam E3 is the data type for the third element.
 * @tparam E4 is the data type for the fourth element.
 */
template<class E1, class E2, class E3, class E4> class Tuple4
{
public:
    E1 m_x1; //!< The first element.
    E2 m_x2; //!< The second element.
    E3 m_x3; //!< The third element.
    E4 m_x4; //!< The fourth element.

    //! Constructs a 4-tuple using default constructors.
    Tuple4() { }
    //! Constructs a 4-tuple for given values.
    Tuple4(const E1 & y1, const E2 & y2, const E3 & y3, const E4 & y4) :
        m_x1(y1), m_x2(y2), m_x3(y3), m_x4(y4) { }
    //! Constructs a 4-tuple that is a copy of \a t4.
    Tuple4(const Tuple4<E1, E2, E3, E4> & t4) :
        m_x1(t4.m_x1), m_x2(t4.m_x2), m_x3(t4.m_x3), m_x4(t4.m_x4) { }

    //! Returns a reference the first element.
    const E1 & x1() const
    {
        return m_x1;
    }
    //! Returns a reference the second element.
    const E2 & x2() const
    {
        return m_x2;
    }
    //! Returns a reference the third element.
    const E3 & x3() const
    {
        return m_x3;
    }
    //! Returns a reference the fourth element.
    const E4 & x4() const
    {
        return m_x4;
    }

    //! Returns a reference the first element.
    E1 & x1()
    {
        return m_x1;
    }
    //! Returns a reference the second element.
    E2 & x2()
    {
        return m_x2;
    }
    //! Returns a reference the third element.
    E3 & x3()
    {
        return m_x3;
    }
    //! Returns a reference the fourth element.
    E4 & x4()
    {
        return m_x4;
    }

    // default assignment operator

    OGDF_NEW_DELETE
};

//! Equality operator for 4-tuples
template<class E1, class E2, class E3, class E4>
bool operator==(const Tuple4<E1, E2, E3, E4> & t1, const Tuple4<E1, E2, E3, E4> & t2)
{
    return t1.x1() == t2.x1() && t1.x2() == t2.x2() &&
           t1.x3() == t2.x3() && t1.x4() == t2.x4();
}

//! Inequality operator for 4-tuples
template<class E1, class E2, class E3, class E4>
bool operator!=(const Tuple4<E1, E2, E3, E4> & t1, const Tuple4<E1, E2, E3, E4> & t2)
{
    return t1.x1() != t2.x1() || t1.x2() != t2.x2() ||
           t1.x3() != t2.x3() || t1.x4() != t2.x4();
}

//! Output operator for 4-tuples
template<class E1, class E2, class E3, class E4>
ostream & operator<<(ostream & os, const Tuple4<E1, E2, E3, E4> & t4)
{
    os << "(" << t4.x1() << " " << t4.x2() << " " <<
       t4.x3() << " " << t4.x4() << ")";
    return os;
}

template<typename K1_, typename K2_,
         typename Hash1_ = DefHashFunc<K1_>,
         typename Hash2_ = DefHashFunc<K2_> >
class HashFuncTuple
{
public:
    HashFuncTuple() { }

    HashFuncTuple(const Hash1_ &hash1, const Hash2_ &hash2)
        : m_hash1(hash1), m_hash2(hash2) { }

    size_t hash(const Tuple2<K1_, K2_> & key) const
    {
        return 23 * m_hash1.hash(key.x1()) + 443 * m_hash2.hash(key.x2());
    }

private:
    Hash1_ m_hash1;
    Hash2_ m_hash2;
};

} // namespace ogdf


#endif
