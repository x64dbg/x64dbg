/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of the class PQLeafKey.
 *
 * \author Sebastian Leipert
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


#ifndef OGDF_PQ_LEAF_KEY_H
#define OGDF_PQ_LEAF_KEY_H



#include <stdlib.h>
#include <ogdf/internal/planarity/PQBasicKey.h>

namespace ogdf
{


template<class T, class X, class Y> class PQNode;

/**
 * The class template PQLeafKey is a derived class of class template
 * PQBasicKey. PQLeafKey is a concrete class.
 *
 * The class template PQLeafKey is used for carrying the elements of
 * a user defined set of any type, where permissible permutations have to be
 * found. In order to use the datastructure PQ-tree as class template
 * PQTree, the user has to specify a set of arbitrary elements that
 * form the leaves of the PQ-tree.
 *
 * It has to be oberved that leaves have to be treated in almost all
 * manipulations of the PQ-tree as all other nodes in the tree. Therefore
 * the leaves have the same base class PQNode as the P- or Q-nodes.
 * This obviously permits direct manipulation of the leaves in the tree by
 * the client. Hence the client cannot overload the leaves by himself in order
 * to specify the set of elements that he wants to manipulate.
 *
 * Therefore the class template PQLeafKey is used. The user is allowed to
 * manipulate the class at his will, and he can instantiate any kind template
 * class of PQLeafKey.h.
 *
 * Besides the specification, what kind of element of the observed set is
 * carried along, PQLeafKey has a pointer to the leaf
 * symbolizing  this special element in the PQ-tree. On the other hand,
 * a leaf has a pointer to its corresponding PQLeafKey.
 *
 * After instantiating a various amount of PQLeafKey's,
 * the PQ-tree is initialized with this set of PQLeafKey's. Every time
 * a subset of elements has to be reduced, the corresponding subset of
 * PQLeafKey's is handed over to the PQTree. This enables the class template
 * PQTree to identify the corresponing leaves in the tree and
 * to start the reduction process.
 *
 * The class template PQLeafKey is treated in a different way by the
 * class template PQTree than all other information
 * storage classes derived from PQBasicKey.
 * When initializing the PQ-tree, the
 * class template PQTree sets the pointer \a m_nodePointer of PQLeafKey
 * which is contained in the abstract base class PQBasicKey.
 * This pointer identifies a unique leave in the tree that belongs to the information
 * stored in a PQLeafKey. The maintainance
 * of this pointer <b>is not</b> left to the user. It is managed by the
 * PQ-tree but still allows the user to identify and acces leaves
 * with a certain informations in constant time.
 */

template<class T, class X, class Y>
class PQLeafKey : public PQBasicKey<T, X, Y>
{
public:

    /**
     * The \a m_userStructKey has to be overloaded by the client. This
     * element is kept public, since the user has to have the opportunity
     * to manipulate the information that was stored by her algorithm at a
     * node.
     */
    T m_userStructKey;

    // Constructor
    PQLeafKey(T element)
        : PQBasicKey<T, X, Y>()
    {
        m_userStructKey = element;
    }

    //Destructor
    virtual ~PQLeafKey() {}

    //! Returns 0.
    virtual X userStructInfo()
    {
        return 0;
    }

    //! Returns 0;
    virtual Y userStructInternal()
    {
        return 0;
    }

    //! Returns \a m_userStructKey.
    virtual T userStructKey()
    {
        return m_userStructKey;
    }

};

}

#endif
