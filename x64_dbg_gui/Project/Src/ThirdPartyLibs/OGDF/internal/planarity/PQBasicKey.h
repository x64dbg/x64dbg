/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of the class PQBasicKey.
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


#ifndef OGDF_PQ_BASICKEY_H
#define OGDF_PQ_BASICKEY_H



#include <stdlib.h>
#include <ogdf/internal/planarity/PQBasicKeyRoot.h>


namespace ogdf
{

/**
 * The class template PQBasicKey is an abstract base class. It enables the user
 * of the PQ-tree to store different informations at every node of
 * the tree.
 *
 * The implementation of the PQ-tree provides the storage of three
 * different types of information.
 *   - General information that is stored at P- and Q-nodes and
 *     leaves likewise (see also PQNodeKey).
 *   - Information that is only supported for internal nodes (see
 *     also internalKey).
 *   - The keys of the leaves (see also leafKey).
 *     The keys are constructed to carry the
 *     elements of  a user defined set of any type, where permissible
 *     permutations have to be
 *     found. In order to use the datastructure PQ-tree as class template
 *     PQTree, the user has to specify a set of arbitrary elements that
 *     form the leaves of the PQ-tree. The keys function as storage class
 *     of the elements of the set.
 *
 * All three storage classes are derived class templates of
 * PQBasicKey. The class PQBasicKey has a pointer \a m_nodePointer to a PQNode,
 * beeing either a leaf or an internal
 * PQInternalNode. The base class itself does not provide any storage of
 * the informations, it is hidden in the derived classes. PQBasicKey
 * only declares a few pure virtual functions that are overloaded in
 * the derived classes and which give access to the information stored
 * in the derived classes.
 *
 * The information stored in an element of a derived class of
 * PQBasicKey is assigned to a unique node in the PQ-tree. This
 * unique node can be identified with the \a m_nodePointer. The
 * maintenance of this pointer is left to the user in the derived
 * concrete classes PQNodeKey and internalKey. By keeping the
 * responsibillity for these classes by the client,
 * nodes with certain informations can
 * be accessed by the client in constant time. This makes
 * the adaption of algorithms fast and easy.
 *
 * Only the derived concrete class template leafKey
 * is treated in a different way by the class template PQTree.
 * When initializing the PQTree with a set of elements of type leafKey, the
 * class template PQTree sets the pointer \a m_nodePointer of every element.
 * This is due to the fact that a PQ-tree is always defined over some set,
 * whose elements are
 * stored in the leaves. Hence the class PQtree expects such a set
 * and supports its maintainance. Storing extra information at every
 * node may be omitted and makes the PQtree easy applicable.
 *
 * We now give a short overview of the class template declaration
 * PQBasicKey. The class template PQBasicKey is used as a base
 * class template that specifies three different types of information.
 * The type of information used at a node is depending on the type of
 * the node. These
 * informations have to be specified by the user.
 *
 * The formal type parameters of the class template PQBasicKey are
 * categorized as follows.
 *   - \a T is a formal type parameter for the information stored in
 *     leafKey.
 *   - \a X is a formal type parameter for the information stored in
 *     PQNodeKey.
 *   - \a Y is a formal type parameter for the information stored in
 *     internalKey.
 *
 * The class template PQBasicKey contains a few pure virtual member
 * functions that are overloaded in the derived class leafKey,
 * PQNodeKey and internalKey. These functions enable the client to
 * access the information stored at a node.
 */

template<class T, class X, class Y> class PQNode;



template<class T, class X, class Y>
class PQBasicKey: public PQBasicKeyRoot
{

public:

    // Constructor
    PQBasicKey() : m_nodePointer(0) { }


    /**
     * The function nodePointer()
     * returns a pointer to an element of type
     * PQNode. This element can be either of type leaf or
     * PQInternalNode. PQBasicKey, or rather its derived classes store
     * informations of this PQNode. The user is able identify with the
     * help of this function for every information its corresponding node.
     * Nevertheless, the private member \a m_nodePointer that stores
     * the pointer to this member <b>is not set</b> within the PQ-tree,
     * unless it is a derived class template of type leafKey.
     *
     * Setting the \a m_nodePointer has to be done explicitly by
     * the client with the help of the
     * function setNodePointer().
     * This offers as much freedom to the
     * client as possible, since this enables the client to keep control
     * over the informations stored at different nodes and to access
     * nodes with specified informations in constant time.
     */
    PQNode<T, X, Y>* nodePointer()
    {
        return m_nodePointer;
    }

    /**
     * The function print() is a virtual function, that can be overloaded
     * by the user in order to print out the information stored at any of
     * the derived classes. Deriving this function, the user can choose any
     * format for printing out the information. Currently, the return value
     * of the function print() is an empty string.
     */

    virtual ostream & print(ostream & os)
    {
        return os;
    }

    /**
     * The function setNodePointer() sets the private member
     * \a m_nodePointer. The private member \a m_nodePointer stores the
     * address of the corresponding node in the PQTree.
     * Using this function enables the client to identify
     * certain informations with a node in the PQ-tree.
     */
    void setNodePointer(PQNode<T, X, Y>* node)
    {
        m_nodePointer = node;
    }

    //! Returns the key of a leaf.
    virtual T userStructKey()  = 0;

    //! Returns the information of any node.
    virtual X userStructInfo()  = 0;

    //! Returns the information of any internal node.
    virtual Y userStructInternal()  = 0;


private:

    /** Stores the adress of a node. This node has to
     * be specified by the client via the function \a setNodePointer.
     */
    PQNode<T, X, Y>* m_nodePointer;

};

}

#endif
