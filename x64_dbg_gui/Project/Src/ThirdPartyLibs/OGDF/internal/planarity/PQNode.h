/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of the class PQNode.
 *
 * This file contains the header for the class template PQNode. The
 * class template PQNode is used as an abstract base class for all
 * nodes in a PQ-tree. The derived classes are a class template for
 * Q- and P-nodes internalNodes (PQInternalNode) and a class template
 * PQLeaf for the leaves of the tree.
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


#ifndef OGDF_PQ_NODE_H
#define OGDF_PQ_NODE_H


#include <ogdf/basic/List.h>
#include <ogdf/internal/planarity/PQNodeRoot.h>


namespace ogdf
{


template<class T, class X, class Y> class PQTree;
template<class T, class X, class Y> class PQLeafKey;
template<class T, class X, class Y> class PQNodeKey;
template<class T, class X, class Y> class PQInternalKey;


template<class T, class X, class Y> class PQNode: public PQNodeRoot
{
    /**
     * All members and member function of PQNode are needed
     * by the class template PQTree. Therefore the class PQTree
     * was made friendof PQNode, since this prevents the use of a large
     * amount of extra public functions.
     */
    friend class PQTree<T, X, Y>;

public:

    /**
     * The (first) constructor combines the node with its information and
     * will automatically set the \a m_nodePointer (see basicKey) of
     * the element of type PQNodeKey.
     */
    PQNode(int count, PQNodeKey<T, X, Y>* infoPtr);


    /**
     * The (second) constructor is called,
     * if no information is available or neccessary.
     */
    PQNode(int count);

    /**
     * The destructor does not delete any accompanying information class as PQLeafKey,
     * PQNodeKey and PQInternalKey. This has been avoided, since applications may
     * need the existence of these information classes after the corresponding node
     * has been deleted. If the deletion of an accompanying information class should
     * be performed with the deletion of a node, either derive a new class
     * with an appropriate destructor, or make use of the function
     * CleanNode() of the class template PQTree.
     */
    virtual ~PQNode()
    {
        delete fullChildren;
        delete partialChildren;
    }

    /**
     * The function changeEndmost() replaces the old endmost child \a oldEnd
     * of the node by a new child \a newEnd.
     * If the node is a Q-node, then it must have two valid
     * pointers to its endmost children. If one of the endmost children is
     * \a oldEnd, it is replaced by \a newEnd.
     * The function changeEndmost() returns 1 if it succeeded in
     * replacing \a oldEnd by \a newEnd. Otherwise the function returns
     * 0, leaving with an error message.
     */
    bool changeEndmost(PQNode<T, X, Y>* oldEnd, PQNode<T, X, Y>* newEnd);

    /**
     * The function changeSiblings() replaces the old sibling \a oldSib of the
     * node by a new sibling \a newSib.
     * If the node has \a oldSib as sibling, then it changes the
     * sibling pointer that references to \a oldSib and places \a newSib
     * at its position.
     * The function changeSiblings() returns 1 if it succeeded in
     * replacing \a oldSib by \a newSib. Otherwise the function returns
     * 0, leaving with an error message.
     */
    bool changeSiblings(PQNode<T, X, Y>* oldSib, PQNode<T, X, Y>* newSib);

    /**
     * The function endmostChild() checks if a node is endmost child of
     * a Q-node. This is 1 if one of the sibling pointers \a m_sibLeft
     * or \a m_sibRight is 0. If the node is endmost child of a Q-node,
     * then it has a valid parent pointer.
     */
    bool endmostChild()
    {
        return (m_sibLeft == 0 || m_sibRight == 0);
    }

    /**
     * Returns one of the endmost children of node, if node is a Q-node.
     * The function getEndmost() accepts as input a pointer to a
     * PQNode stored in \a other. The returned endmost child is unequal
     * to the one specified in \a other.  In case that an arbitrary endmost child
     * should be looked up, set \a other = 0. This makes the function
     * getEndmost() return an arbitrary endmost child (it returns the
     * left endmost child).
     */
    PQNode<T, X, Y>* getEndmost(PQNode<T, X, Y>* other) const
    {
        if(m_leftEndmost != other)
            return m_leftEndmost;
        else if(m_rightEndmost != other)
            return m_rightEndmost;

        return 0;
    }

    /**
     * Returns one of the endmost children of node, if node is a Q-node.
     * The function accepts an integer denoting a direction causing the
     * function to return either the left or the endmost child.
     */
    PQNode<T, X, Y>* getEndmost(int side) const
    {
        if(side == LEFT || side == 0)
            return m_leftEndmost;
        else if(side == RIGHT)
            return m_rightEndmost;

        return 0;
    }

    //! Returns the identification number of a node.
    PQNodeKey<T, X, Y>* getNodeInfo() const
    {
        return m_pointerToInfo;
    }

    /**
     * The function getSib() returns one of the siblings of the node.
     * It accepts an integer denoting a dircetion causing the
     * function to return either the left or the right sibling.
     */
    PQNode<T, X, Y>* getSib(int side) const
    {
        if(side == LEFT)
            return m_sibLeft;
        else if(side == RIGHT)
            return m_sibRight;

        return 0;
    }

    /**
     * The function getNextSib() returns one of the siblings of the node.
     * The function getNextSib() accepts as input a pointer to a
     * PQNode stored in \a other. The returned sibling is unequal to the
     * one specified in \a other. In case
     * that no sibling has been looked up before, set \a other = 0.
     * This makes the function getNextSib() return an arbitrary sibling
     * (it returns the left sibling).
     */
    PQNode<T, X, Y>* getNextSib(PQNode<T, X, Y>* other) const
    {
        if(m_sibLeft != other)
            return m_sibLeft;
        else if(m_sibRight != other)
            return m_sibRight;

        return 0;
    }


    //! Returns the identification number of a node.
    int  identificationNumber() const
    {
        return m_identificationNumber;
    }

    //! Returns the number of children of a node.
    int  childCount() const
    {
        return m_childCount;
    }

    //! Sets the number of children of a node.
    void childCount(int count)
    {
        m_childCount = count;
    }

    /**
     * The function parent() returns a pointer to the parent of a node.
     *
     * \warning After reducing the PQ-tree, some nodes may not have
     * valid parent pointers anymore. This is no fault, the datastructur
     * was designed this way. See also Booth and Lueker.
     */
    PQNode<T, X, Y>* parent() const
    {
        return m_parent;
    }

    /**
     * Sets the parent pointer of a node. This function
     * is needed in more ellaborated algorithms implemented as derivation of
     * the class template PQTree. Here, the parent pointer probably is
     * always needed and therefore has to be set within special functions,
     * used in a pre-run before applying the bubble Phase of the PQTree.
     */
    PQNode<T, X, Y>* parent(PQNode<T, X, Y>* newParent)
    {
        return m_parent = newParent;
    }

    //! Returns the type of the parent of a node.
    int parentType() const
    {
        return m_parentType;
    }

    /**
     * Sets the type of the parent of a node.
     * This does not change the type of the parent!
    */
    void parentType(int newParentType)
    {
        m_parentType = newParentType;
    }

    //! Returs the number of pertinent children of a node.
    int pertChildCount() const
    {
        return m_pertChildCount;
    }

    //! Sets the number of pertinent children of a node.
    void pertChildCount(int count)
    {
        m_pertChildCount = count;
    }

    /**
     * The default function putSibling()
     * stores a new sibling at a free sibling pointer
     * of the node. This is only possible, if the node has at most one sibling.
     * The function then detects a non used sibling pointer and places \a newSib
     * onto it. putSibling() returns 0 if there have been two siblings
     * detected, occupying the two possible pointers. In this case the new sibling
     * \a newSib cannot be stored. If there was at a maximum one sibling stored,
     * the function will place \a newSib on the free pointer and return either
     * \a LEFT or \a RIGHT, depending wich pointer has been used.
     *
     * This function will always scan the pointer to the left brother first.
     */
    SibDirection putSibling(PQNode<T, X, Y>* newSib)
    {
        if(m_sibLeft == 0)
        {
            m_sibLeft = newSib;
            return LEFT;
        }

        OGDF_ASSERT(m_sibRight == 0);
        m_sibRight = newSib;
        return RIGHT;
    }

    /**
     * The function putSibling()
     * with preference stores a new sibling at a free sibling pointer
     * of the node. This is only possible, if the node has at most one sibling.
     * The function then detects a non used sibling pointer and places \a newSib
     * onto it. putSibling() returns 0 if there have been two siblings
     * detected, occupying the two possible pointers. In this case the new sibling
     * \a newSib could not be stored. If there was at a maximum one sibling
     * stored, the function will place \a newSib on the free pointer and
     * return either \a LEFT or \a RIGHT, depending wich pointer has been used.
     *
     * This function scans the brother first, which has been specified in the
     * preference. If the preference has value \a LEFT, it scans the pointer
     * to the left brother first. If the value is \a RIGHT, it scans the pointer
     * to the right brother first.
     */
    SibDirection putSibling(PQNode<T, X, Y>* newSib, int preference)
    {
        if(preference == LEFT)
            return putSibling(newSib);

        OGDF_ASSERT(preference == RIGHT);

        if(m_sibRight == 0)
        {
            m_sibRight = newSib;
            return RIGHT;
        }

        OGDF_ASSERT(m_sibLeft == 0);
        m_sibLeft = newSib;
        return LEFT;
    }

    //! Returns a pointer to the reference child if node is a P-node.
    PQNode<T, X, Y>* referenceChild() const
    {
        return m_referenceChild;
    }

    //! Returns the pointer to the parent if node is a reference child.
    PQNode<T, X, Y>* referenceParent() const
    {
        return m_referenceParent;
    }

    //! Sets the pointer \a m_pointerToInfo to the specified adress of \a pointerToInfo.
    bool  setNodeInfo(PQNodeKey<T, X, Y>* pointerToInfo)
    {
        m_pointerToInfo = pointerToInfo;
        if(pointerToInfo != 0)
        {
            m_pointerToInfo->setNodePointer(this);
            return true;
        }

        return false;
    }

    /**
     * getKey() returns a pointer to the PQLeafKeyof a node, in case that
     * the node is supposed to have a key, such as elements of the derived
     * class template PQLeaf.
     * The key contains information and is of type PQLeafKey.
     */
    virtual PQLeafKey<T, X, Y>* getKey() const = 0;

    //! Sets a specified pointer variable in a derived class to the specified adress of \a pointerToKey that is of type PQLeafKey.
    /**
     * If a derived class, such as PQInternalNode, is not supposed to store
     * informations of type PQLeafKey, setKey() ignores the informations as long as
     * \a pointerToKey = 0. The return value then is 1.
     * In case that \a pointerToKey != 0, the return value is 0.
     *
     * If a derived class, such as PQLeaf is supposed to
     * store informations of type PQLeafKey, \a pointerToKey
     * has to be instantiated by the client. The function setKey() does
     * not instantiate the corresponding variable in the derived class.
     * The return value is always 1 unless \a pointerKey was equal to 0.
     */
    virtual bool setKey(PQLeafKey<T, X, Y>* pointerToKey) = 0;

    /**
     * getInternal() returns a pointer to the PQInternalKey
     * information of a node, in case that
     * the node is supposed to have PQInternalKey information,
     * such as elements of the derived class template PQInternalNode.
     * The internal information is of type PQInternalKey.
     */
    virtual PQInternalKey<T, X, Y>* getInternal() const = 0;

    /*
     * setInternal() sets a specified pointer variable in a derived class
     * to the specified adress of \a pointerToInternal that is of type
     * PQInternalKey.
     *
     * If a derived class, such as PQLeaf,
     * is not supposed to store informations of type PQInternalKey,
     * setInternal() ignores the informations as long as
     * \a pointerToInternal = 0. The return value then is 1.
     * In case that \a pointerToInternal != 0, the return value is 0.
     *
     * If a derived class, such as PQInternalNode is
     * supposed to store informations of type PQInternalKey,
     * \a pointerToInternal has to be instantiated by the client. The
     * function setInternal() does
     * not instantiate the corresponding variable in the derived class.
     * The return value is always 1 unless \a pointerInternal was
     * equal to 0.
     */
    virtual bool setInternal(PQInternalKey<T, X, Y>* pointerToInternal) = 0;

    /**
     * mark() returns the variable \a m_mark in the
     * derived class PQLeaf and PQInternalNode.
     * In a derived class this function has to return the designation used in
     * the first pass of Booth and Luekers algorithm called Bubble(). A
     * node then is either marked \a BLOCKED, \a UNBLOCKED or \a QUEUED (see PQNode).
     */
    virtual PQNodeMark mark() const = 0;

    //! mark() sets the variable \a m_mark in the derived class PQLeaf and PQInternalNode.
    virtual void mark(PQNodeMark) = 0;

    //! Returns the variable \a m_status in the derived class PQLeaf and PQInternalNode.
    /**
     * Its objective is to manage
     * status of a node in the PQ-tree. A status is
     * any kind of information of the current situation in the frontier of
     * a node (the frontier of a node are all descendant leaves of the
     * node). A status is anything such as \a EMPTY, \a FULL or
     * \a PARTIAL (see PQNode). Since there might be more than those three possibilities,
     * (e.g. in computing planar subgraphs this
     * function probably has to be overloaded by the client.
     */
    virtual PQNodeStatus status() const = 0;

    //! Sets the variable \a m_status in the derived class PQLeaf and PQInternalNode.
    virtual void status(PQNodeStatus) = 0;

    //! Returns the variable \a m_type in the derived class PQLeaf and PQInternalNode.
    /**
     * Its objective it to manage the type of a node.
     * node the current node is. The type of a node in the class template
     * PQTree is either \a PNode, \a QNode or \a leaf (see PQNode).
     * There may be of course more types such as <em>sequence indicators</em>.
     *
     * Observe that the derived class template PQLeaf does
     * not have a variable \a m_type, since it obviously is of type \a leaf.
     */
    virtual PQNodeType type() const = 0;

    //! Sets the variable \a m_type in the derived class PQLeaf and PQInternalNode.
    virtual void type(PQNodeType) = 0;


protected:


    // Stores the number of children of the node.
    int m_childCount;

    /**
     * Needed for debuging
     * purposes. The PQ-trees can be visualized with the help of the <em>Tree
     * Interface</em> and the \a m_debugTreeNumber is needed to print out the
     * tree in the correct file format.
     */
    int m_debugTreeNumber;

    /**
     * Each node that has been introduced once into
     * the tree gets a unique number. If the node is removed from the
     * tree during a reduction or with the help of one of the functions
     * that is provided by the class template PQtree, its number <b>is not reused</b>.
     * This always allows exact identification of nodes
     * during any process that is envoked on the PQ-tree. We strongly
     * recommend users who construct the tree with the help of the
     * construction functions and who instantiate the nodes by them selves
     * to do the same.
     */
    int m_identificationNumber;

    //! Stores the type of the parent which can be either a P- or Q-node.
    int m_parentType;

    //! Stores the number of pertinent children of the node.
    int m_pertChildCount;

    //! Stores the number of pertinent leaves in the frontier of the node.
    int m_pertLeafCount;

    //! Stores a pointer to the first full child of a Q-node.
    PQNode<T, X, Y>* m_firstFull;

    PQNode<T, X, Y>* m_leftEndmost;

    /**
     * Is a pointer to the parent. Observe that this
     * pointer may not be up to date after a few applications of the
     * reduction.
     */
    PQNode<T, X, Y>* m_parent;

    /**
     * Stores a pointer to one child, the <b>reference child</b> of the
     * doubly linked cirkular list of children of a
     * P-node. With the help of this pointer, it is possible to access
     * the children of the P-node
     */
    PQNode<T, X, Y>* m_referenceChild;

    /**
     * Is a pointer to the parent, in case that the
     * parent is a P-node and the node itself is its reference child.
     * The pointer is needed in order to identify the reference child
     * among all children of a P-node.
     */
    PQNode<T, X, Y>* m_referenceParent;

    //! Stores the right endmost child of a Q-node.
    PQNode<T, X, Y>* m_rightEndmost;

    /**
     * Stores a pointer ot the left sibling of PQNode.
     * If PQNode is child of a Q-node and has no left sibling,
     * \a m_sibLeft is set to 0. If PQNode is child of a P-node,
     * all children of the P-node are linked in a cirkular list. In the
     * latter case, \a m_sibLeft is never 0.
     */
    PQNode<T, X, Y>* m_sibLeft;

    /**
     * Stores a pointer ot the right sibling of PQNode.
     * If PQNode is child of a Q-node and has no right sibling,
     * \ m_sibRight is set to 0. If PQNode is child of a P-node,
     * all children of the P-node are linked in a cirkular list. In the
     * latter case, \a m_sibRight is never 0.
     */
    PQNode<T, X, Y>* m_sibRight;

    //! Stores a pointer to the corresponding information of the node.
    PQNodeKey<T, X, Y>* m_pointerToInfo;


    //! Stores all full children of a node during a reduction.
    List<PQNode<T, X, Y>*>* fullChildren;

    //! Stores all partial children of a node during a reduction.
    List<PQNode<T, X, Y>*>* partialChildren;

};


/*
The function [[changeEndmost]] replaces the old endmost child [[oldEnd]] of
the node by a new child [[newEnd]].
If the node is a $Q$-node, then it must have two valid
pointers to its endmost children. If one of the endmost children is [[oldEnd]],
it is replaced by [[newEnd]].
The function [[changeEndmost]] returns [[1]] if it succeeded in
replacing [[oldEnd]] by [[newEnd]]. Otherwise the function returns
[[0]], leaving with an error message.
*/
template<class T, class X, class Y>
bool PQNode<T, X, Y>::changeEndmost(PQNode<T, X, Y>* oldEnd, PQNode<T, X, Y>* newEnd)
{
    if(m_leftEndmost == oldEnd)
    {
        m_leftEndmost = newEnd;
        return true;
    }
    else if(m_rightEndmost == oldEnd)
    {
        m_rightEndmost = newEnd;
        return true;
    }
    return false;
}

/*
The function [[changeSiblings]] replaces the old sibling [[oldSib]] of the
node by a new sibling [[newSib]].

If the node has [[oldSib]] as sibling, then it changes the
sibling pointer that references to [[oldSib]] and places [[newSib]]
at its position.

The function [[changeSiblings]] returns [[1]] if it succeeded in replacing
[[oldSib]] by [[newSib]]. Otherwise the function returns
[[0]], leaving with an error message.
*/
template<class T, class X, class Y>
bool PQNode<T, X, Y>::changeSiblings(PQNode<T, X, Y>* oldSib, PQNode<T, X, Y>* newSib)
{
    if(m_sibLeft == oldSib)
    {
        m_sibLeft = newSib;
        return true;
    }
    else if(m_sibRight == oldSib)
    {
        m_sibRight = newSib;
        return true;
    }
    return false;
}


/*
The (first) constructor combines the node with its information and
will automatically set the [[m_nodePointer]] (see \ref{basicKey}) of
the element of type [[PQNodeKey]] (see \ref{PQNodeKey}).
*/
template<class T, class X, class Y>
PQNode<T, X, Y>::PQNode(int count, PQNodeKey<T, X, Y>* infoPtr)
{
    m_identificationNumber = count;
    m_childCount = 0;
    m_pertChildCount = 0;
    m_pertLeafCount = 0;
    m_debugTreeNumber = 0;
    m_parentType = 0;

    m_parent = 0;
    m_firstFull = 0;
    m_sibLeft = 0;
    m_sibRight = 0;
    m_referenceChild = 0;
    m_referenceParent = 0;
    m_leftEndmost = 0;
    m_rightEndmost = 0;

    fullChildren = OGDF_NEW List<PQNode<T, X, Y>*>;
    partialChildren = OGDF_NEW List<PQNode<T, X, Y>*>;

    m_pointerToInfo = infoPtr;
    infoPtr->setNodePointer(this);
}


/*
The (second) constructor is called,
if no information is available or neccessary.
*/
template<class T, class X, class Y>
PQNode<T, X, Y>::PQNode(int count)
{
    m_identificationNumber = count;
    m_childCount = 0;
    m_pertChildCount = 0;
    m_pertLeafCount = 0;
    m_debugTreeNumber = 0;
    m_parentType = 0;

    m_parent = 0;
    m_firstFull = 0;
    m_sibLeft = 0;
    m_sibRight = 0;
    m_referenceChild = 0;
    m_referenceParent = 0;
    m_leftEndmost = 0;
    m_rightEndmost = 0;

    fullChildren = OGDF_NEW List<PQNode<T, X, Y>*>;
    partialChildren = OGDF_NEW List<PQNode<T, X, Y>*>;

    m_pointerToInfo = 0;
}

}

#endif

