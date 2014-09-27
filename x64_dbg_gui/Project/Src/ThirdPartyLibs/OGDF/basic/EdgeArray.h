/*
 * $Revision: 2615 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 14:23:36 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of EdgeArray class.
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

#ifndef OGDF_EDGE_ARRAY_H
#define OGDF_EDGE_ARRAY_H


#include <ogdf/basic/Graph_d.h>


namespace ogdf
{


//! Abstract base class for edge arrays.
/**
 * Defines the interface for event handling used by the Graph class.
 * Use the parameterized class EdgeArray for creating edge arrays.
 */
class EdgeArrayBase
{
    /**
     * Pointer to list element in the list of all registered edge
     * arrays which references this array.
     */
    ListIterator<EdgeArrayBase*> m_it;

public:
    const Graph* m_pGraph; //!< The associated graph.

    //! Initializes an edge array not associated with a graph.
    EdgeArrayBase() : m_pGraph(0) { }
    //! Initializes an edge array associated with \a pG.
    EdgeArrayBase(const Graph* pG) : m_pGraph(pG)
    {
        if(pG) m_it = pG->registerArray(this);
    }

    // destructor, unregisters the array
    virtual ~EdgeArrayBase()
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
}; // class EdgeArrayBase


//! Dynamic arrays indexed with edges.
/**
 * Edge arrays represent a mapping from edges to data of type \a T.
 * They adjust their table size automatically when the graph grows.
 *
 * @tparam T is the element type.
 */
template<class T> class EdgeArray : private Array<T>, protected EdgeArrayBase
{
    T m_x; //!< The default value for array elements.

public:
    //! Constructs an empty edge array associated with no graph.
    EdgeArray() : Array<T>(), EdgeArrayBase() { }
    //! Constructs an edge array associated with \a G.
    EdgeArray(const Graph & G) : Array<T>(G.edgeArrayTableSize()), EdgeArrayBase(&G) { }
    //! Constructs an edge array associated with \a G.
    /**
     * @param G is the associated graph.
     * @param x is the default value for all array elements.
     */
    EdgeArray(const Graph & G, const T & x) :
        Array<T>(0, G.edgeArrayTableSize() - 1, x), EdgeArrayBase(&G), m_x(x) { }
    //! Constructs an edge array that is a copy of \a A.
    /**
     * Associates the array with the same graph as \a A and copies all elements.
     */
    EdgeArray(const EdgeArray<T> & A) : Array<T>(A), EdgeArrayBase(A.m_pGraph), m_x(A.m_x) { }

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

    //! Returns a reference to the element with index \a e.
    const T & operator[](edge e) const
    {
        OGDF_ASSERT(e != 0 && e->graphOf() == m_pGraph)
        return Array<T>::operator [](e->index());
    }

    //! Returns a reference to the element with index \a e.
    T & operator[](edge e)
    {
        OGDF_ASSERT(e != 0 && e->graphOf() == m_pGraph)
        return Array<T>::operator [](e->index());
    }

    //! Returns a reference to the element with index edge of \a adj.
    const T & operator[](adjEntry adj) const
    {
        OGDF_ASSERT(adj != 0)
        return Array<T>::operator [](adj->index() >> 1);
    }

    //! Returns a reference to the element with index edge of \a adj.
    T & operator[](adjEntry adj)
    {
        OGDF_ASSERT(adj != 0)
        return Array<T>::operator [](adj->index() >> 1);
    }

    //! Returns a reference to the element with index \a index.
    /**
     * \attention Make sure that \a index is a valid index for an edge
     * in the associated graph!
     */
    const T & operator[](int index) const
    {
        return Array<T>::operator [](index);
    }

    //! Returns a reference to the element with index \a index.
    /**
     * \attention Make sure that \a index is a valid index for an edge
     * in the associated graph!
     */
    T & operator[](int index)
    {
        return Array<T>::operator [](index);
    }

    //! Assignment operator.
    EdgeArray<T> & operator=(const EdgeArray<T> & a)
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
        Array<T>::init(G.edgeArrayTableSize());
        reregister(&G);
    }

    //! Reinitializes the array. Associates the array with \a G.
    /**
     * @param G is the associated graph.
     * @param x is the default value.
     */
    void init(const Graph & G, const T & x)
    {
        Array<T>::init(0, G.edgeArrayTableSize() - 1, m_x = x);
        reregister(&G);
    }

    //! Sets all array elements to \a x.
    void fill(const T & x)
    {
        int high = m_pGraph->maxEdgeIndex();
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

}; // class EdgeArray<T>


//! Bucket function for edges.
/**
 * The bucket of an edge is stored in an edge array which is passed
 * by the user at construction; only a pointer is stored to that array.
 */
class OGDF_EXPORT BucketEdgeArray : public BucketFunc<edge>
{
    const EdgeArray<int>* m_pEdgeArray; //!< Pointer to edge array.

public:
    //! Constructs a bucket function.
    /**
     * @param edgeArray contains the buckets for the edges. May not be deleted
     *        as long as the bucket function is used.
     */
    BucketEdgeArray(const EdgeArray<int> & edgeArray) : m_pEdgeArray(&edgeArray) { }

    //! Returns bucket of edge \a e.
    int getBucket(const edge & e)
    {
        return (*m_pEdgeArray)[e];
    }
};


} // end namespace ogdf
#include <ogdf/basic/Graph.h>

#endif
