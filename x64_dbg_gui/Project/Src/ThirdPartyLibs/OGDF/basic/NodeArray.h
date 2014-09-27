/*
 * $Revision: 2615 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 14:23:36 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of NodeArray class
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

#ifndef OGDF_NODE_ARRAY_H
#define OGDF_NODE_ARRAY_H


#include <ogdf/basic/Graph_d.h>


namespace ogdf
{


//! Abstract base class for node arrays.
/**
 * Defines the interface for event handling used by the Graph class.
 * Use the parameterized class NodeArray for creating node arrays.
 */
class NodeArrayBase
{
    /**
     * Pointer to list element in the list of all registered node
     * arrays which references this array.
     */
    ListIterator<NodeArrayBase*> m_it;

public:
    const Graph* m_pGraph; //!< The associated graph.

    //! Initializes an node array not associated with a graph.
    NodeArrayBase() : m_pGraph(0) { }
    //! Initializes an node array associated with \a pG.
    NodeArrayBase(const Graph* pG) : m_pGraph(pG)
    {
        if(pG) m_it = pG->registerArray(this);
    }

    // destructor, unregisters the array
    virtual ~NodeArrayBase()
    {
        if(m_pGraph) m_pGraph->unregisterArray(m_it);
    }

    // event interface used by Graph
    //! Virtual function called when table size has to be enlarged.
    virtual void enlargeTable(int newTableSize) = 0;
    //! Virtual function called when table has to be reinitialized.
    virtual void reinit(int initTableSize) = 0;
    //! Virtual function called when array is disconnected from the graph.
    virtual void disconnect() = 0;

    //! Associates the array with a new graph.
    void reregister(const Graph* pG)
    {
        if(m_pGraph) m_pGraph->unregisterArray(m_it);
        if((m_pGraph = pG) != 0) m_it = pG->registerArray(this);
    }
}; // class NodeArrayBase


//! Dynamic arrays indexed with nodes.
/**
 * Node arrays represent a mapping from nodes to data of type \a T.
 * They adjust their table size automatically when the graph grows.
 *
 * @tparam T is the element type.
 */
template<class T> class NodeArray : private Array<T>, protected NodeArrayBase
{
    T m_x; //!< The default value for array elements.

public:
    //! Constructs an empty node array associated with no graph.
    NodeArray() : Array<T>(), NodeArrayBase() { }
    //! Constructs a node array associated with \a G.
    NodeArray(const Graph & G) : Array<T>(G.nodeArrayTableSize()), NodeArrayBase(&G) { }
    //! Constructs a node array associated with \a G.
    /**
     * @param G is the associated graph.
     * @param x is the default value for all array elements.
     */
    NodeArray(const Graph & G, const T & x) :
        Array<T>(0, G.nodeArrayTableSize() - 1, x), NodeArrayBase(&G), m_x(x) { }
    //! Constructs a node array that is a copy of \a A.
    /**
     * Associates the array with the same graph as \a A and copies all elements.
     */
    NodeArray(const NodeArray<T> & A) : Array<T>(A), NodeArrayBase(A.m_pGraph), m_x(A.m_x) { }

    //! Returns true iff the array is associated with a graph.
    bool valid() const
    {
        return (Array<T>::low() <= Array<T>::high());
    }

    //! Returns a pointer to the associated graph.
    const Graph* graphOf() const
    {
        return m_pGraph;
    }

    //! Returns a reference to the element with index \a v.
    const T & operator[](node v) const
    {
        OGDF_ASSERT(v != 0 && v->graphOf() == m_pGraph)
        return Array<T>::operator [](v->index());
    }

    //! Returns a reference to the element with index \a v.
    T & operator[](node v)
    {
        OGDF_ASSERT(v != 0 && v->graphOf() == m_pGraph)
        return Array<T>::operator [](v->index());
    }

    //! Returns a reference to the element with index \a index.
    /**
     * \attention Make sure that \a index is a valid index for a node
     * in the associated graph!
     */
    const T & operator[](int index) const
    {
        return Array<T>::operator [](index);
    }

    //! Returns a reference to the element with index \a index.
    /**
     * \attention Make sure that \a index is a valid index for a node
     * in the associated graph!
     */
    T & operator[](int index)
    {
        return Array<T>::operator [](index);
    }

    //! Assignment operator.
    NodeArray<T> & operator=(const NodeArray<T> & a)
    {
        Array<T>::operator =(a);
        m_x = a.m_x;
        reregister(a.m_pGraph);
        return *this;
    }

    //! Reinitializes the array. Associates the array with no graph.
    void init()
    {
        Array<T>::init();
        reregister(0);
    }

    //! Reinitializes the array. Associates the array with \a G.
    void init(const Graph & G)
    {
        Array<T>::init(G.nodeArrayTableSize());
        reregister(&G);
    }

    //! Reinitializes the array. Associates the array with \a G.
    /**
     * @param G is the associated graph.
     * @param x is the default value.
     */
    void init(const Graph & G, const T & x)
    {
        Array<T>::init(0, G.nodeArrayTableSize() - 1, m_x = x);
        reregister(&G);
    }

    //! Sets all array elements to \a x.
    void fill(const T & x)
    {
        int high = m_pGraph->maxNodeIndex();
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

    virtual void disconnect()
    {
        Array<T>::init();
        m_pGraph = 0;
    }

    OGDF_NEW_DELETE

}; // class NodeArray<T>


} // end namespace ogdf

#include <ogdf/basic/Graph.h>

#endif
