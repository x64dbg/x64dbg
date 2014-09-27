/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of the class PQleaf.
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


#ifndef OGDF_PQ_LEAF_H
#define OGDF_PQ_LEAF_H



#include <ogdf/internal/planarity/PQNode.h>

namespace ogdf
{


/**
 * The datastructure PQ-tree was designed to present a set of
 * permutations on an arbitrary set of elements. These elements are the
 * leafs of a PQ-tree. The client has to specify, what kind
 * of elements he uses. The element of a node is stored in the PQLeafKey
 * of a PQLeaf. The PQLeaf is the only concrete class
 * template of the abstract base class template PQNode
 * that is allowed to have a key.
 */

template<class T, class X, class Y>
class PQLeaf : public PQNode<T, X, Y>
{
public:

    /**
     * The client may choose between two different constructors.
     * In both cases the constructor expects an integer value \a count,
     * setting the value of the variable \a m_identificationNumber in the base class,
     * an integer value \a status setting the variable \a m_status of
     * PQLeaf and a pointer to an element of type PQLeafKey.
     *
     * One of the constructors expects additional information of type
     * PQNodeKey and will automatically set
     * the \a m_nodePointer (see basicKey) of the element of type
     * PQNodeKey to the newly allocated PQLeaf (see also
     * PQNode). The second constructor is called, if no
     * information for the PQLeaf is available or necessary.
     * Both constructors will automatically set the \a m_nodePointer of the
     * \a keyPtr to the newly allocated PQLeaf.
     */
    PQLeaf(
        int count,
        PQNodeRoot::PQNodeStatus stat,
        PQLeafKey<T, X, Y>* keyPtr,
        PQNodeKey<T, X, Y>* infoPtr)
        : PQNode<T, X, Y>(count, infoPtr)
    {
        m_status = stat;
        m_pointerToKey = keyPtr;
        m_mark = PQNodeRoot::UNMARKED;
        keyPtr->setNodePointer(this);
    }

    // Constructor
    PQLeaf(
        int count,
        PQNodeRoot::PQNodeStatus stat,
        PQLeafKey<T, X, Y>* keyPtr)
        : PQNode<T, X, Y>(count)
    {
        m_status = stat;
        m_pointerToKey = keyPtr;
        m_mark = PQNodeRoot::UNMARKED;
        keyPtr->setNodePointer(this);
    }

    /**
     * The destructor does not delete any
     * accompanying information class as PQLeafKey,
     * PQNodeKey and PQInternalKey.
     * This has been avoided, since applications may need the existence of
     * these information classes after the corresponding node has been
     * deleted. If the deletion of an accompanying information class should
     * be performed with the deletion of a node, either derive a new class
     * with an appropriate destructor, or make use of the function
     * CleanNode() of the class template PQTree.
     */
    virtual ~PQLeaf() {}

    /**
     * getKey() returns a pointer to the PQLeafKey
     * of PQLeaf. The adress of the PQLeafKey is stored in the
     * private variable \a m_pointerToKey.
     * The key contains informations of the element that is represented by
     * the PQLeaf in the PQ-tree and is of type PQLeafKey.
     */
    virtual PQLeafKey<T, X, Y>* getKey() const
    {
        return m_pointerToKey;
    }

    /**
     * setKey() sets the pointer variable \a m_pointerToKey to the
     * specified address of \a pointerToKey that is of type PQLeafKey.
     *
     * Observe that \a pointerToKey has
     * to be instantiated by the client. The function setKey() does
     * not instantiate the corresponding variable in the derived class.
     * Using this function will automatically set the \a m_nodePointer
     * of the element of type key (see PQLeafKey)
     * to this PQLeaf. The return value is always 1 unless \a pointerKey
     * was equal to 0.
     */
    virtual bool setKey(PQLeafKey<T, X, Y>* pointerToKey)
    {
        m_pointerToKey = pointerToKey;
        if(pointerToKey != 0)
        {
            m_pointerToKey->setNodePointer(this);
            return true;
        }
        else
            return false;
    }

    /**
     * getInternal() returns 0. The function is designed to
     * return a pointer to the PQInternalKey
     * information of a node, in case that
     * the node is supposed to have internal information. The class
     * template PQLeaf does not have PQInternalKey information.
     */
    virtual PQInternalKey<T, X, Y>* getInternal() const
    {
        return 0;
    }

    /**
     * setInternal() accepts only pointers \a pointerToInternal = 0.
     *
     * The function setInternal() is designed to set a
     * specified pointer variable in a derived class
     * of PQNode to the adress stored in \a pointerToInternal.
     * which is of type PQInternalKey.
     * The class template PQLeaf does not store
     * informations of type PQInternalKey.
     *
     * setInternal() ignores the informations as long as
     * \a pointerToInternal = 0. The return value then is 1.
     * In case that \a pointerToInternal != 0, the return value is 0.
     */
    virtual bool setInternal(PQInternalKey<T, X, Y>* pointerToInternal)
    {
        if(pointerToInternal != 0)
            return false;
        else
            return true;
    }

    //! Returns the variable \a m_mark.
    /**
     * The variable \a m_mark describes the designation used in
     * the first pass of Booth and Luekers algorithm called Bubble(). A
     * PQLeaf is either marked \b BLOCKED, \b UNBLOCKED or \b QUEUED (see
     * PQNode).
     */
    virtual PQNodeRoot::PQNodeMark  mark() const
    {
        return m_mark;
    }

    //! Sets the variable \a m_mark.
    virtual void mark(PQNodeRoot::PQNodeMark m)
    {
        m_mark = m;
    }

    //! Returns the variable \a m_status in the derived class PQLeaf.
    /**
     * The functions manage the status of a node in the PQ-tree. A status is
     * any kind of information of the current situation in the frontier of
     * a node (the frontier of a node are all descendant leaves of the
     * node). A status can be anything such as \b EMPTY, \b FULL or
     * \b PARTIAL (see PQNode). Since there might be more than those three
     * possibilities,
     * (e.g. in computing planar subgraphs) this
     * function may to be overloaded by the client.
     */
    virtual PQNodeRoot::PQNodeStatus status() const
    {
        return m_status;
    }

    //! Sets the variable \a m_status in the derived class PQLeaf.
    virtual void status(PQNodeRoot::PQNodeStatus s)
    {
        m_status = s;
    }

    //! Returns the variable \a m_type in the derived class PQLeaf.
    /**
     * The type of a node is either \b PNode, \b QNode or
     * \b leaf (see PQNodeRoot).
     * Since the type of an element of type PQLeaf is \b leaf every
     * input is ignored and the return value will always be \b leaf.
     */
    virtual PQNodeRoot::PQNodeType type() const
    {
        return PQNodeRoot::leaf;
    }

    //! Sets the variable \a m_type in the derived class PQLeaf.
    virtual void type(PQNodeRoot::PQNodeType) { }

private:

    /**
     * \a m_mark is a variable, storing if the PQLeaf is
     * \b QUEUEUD, \b BLOCKED or \b UNBLOCKED (see PQNode)
     * during the first phase of the procedure Bubble().
     */
    PQNodeRoot::PQNodeMark m_mark;

    /**
     * \a m_pointerToKey stores the adress of the corresponding
     * PQLeafKey.
     * This PQLeafKey can be overloaded by the
     * client in order to represent different sets of elements, where
     * possible permutations have to be examined by the PQ-tree.
     */
    PQLeafKey<T, X, Y>* m_pointerToKey;

    /**
     * \a m_status is a variable storing the status of a PQLeaf.
     * A PQLeaf can be either \b FULL or \b EMPTY (see PQNode).
     */
    PQNodeRoot::PQNodeStatus m_status;

};

}

#endif
