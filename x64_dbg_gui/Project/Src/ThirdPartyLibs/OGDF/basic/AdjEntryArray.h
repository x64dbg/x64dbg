/*
 * $Revision: 2615 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 14:23:36 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of AdjEntryArray class.
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

#ifndef OGDF_ADJ_ENTRY_ARRAY_H
#define OGDF_ADJ_ENTRY_ARRAY_H


#include <ogdf/basic/Graph.h>


namespace ogdf
{


//! Abstract base class for adjacency entry arrays.
/**
 * Defines the interface for event handling used by the Graph class.
 * Use the parameterized class AdjEntryArray for creating adjacency arrays.
 */
class AdjEntryArrayBase
{
    /**
     * Pointer to list element in the list of all registered adjacency
     * entry arrays which references this array.
     */
    ListIterator<AdjEntryArrayBase*> m_it;

public:
    const Graph* m_pGraph; //!< The associated graph.

    //! Initializes an adjacency entry array not associated with a graph.
    AdjEntryArrayBase() : m_pGraph(0) { }
    //! Initializes an adjacency entry array associated with \a pG.
    AdjEntryArrayBase(const Graph* pG) : m_pGraph(pG)
    {
        if(pG) m_it = pG->registerArray(this);
    }

    // destructor, unregisters the array
    virtual ~AdjEntryArrayBase()
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
    //! Virtual function called when the index of an adjacency entry is changed.
    virtual void resetIndex(int newIndex, int oldIndex) = 0;

    //! Associates the array with a new graph.
    void reregister(const Graph* pG)
    {
        if(m_pGraph) m_pGraph->unregisterArray(m_it);
        if((m_pGraph = pG) != 0) m_it = pG->registerArray(this);
    }
}; // class AdjEntryArrayBase


//! Dynamic arrays indexed with adjacency entries.
/**
 * Adjacency entry arrays represent a mapping from adjacency entries to data of type \a T.
 * They adjust their table size automatically when the graph grows.
 *
 * @tparam T is the element type.
 */
template<class T> class AdjEntryArray : private Array<T>, protected AdjEntryArrayBase
{
    T m_x; //!< The default value for array elements.

public:
    //! Constructs an empty adjacency entry array associated with no graph.
    AdjEntryArray() : Array<T>(), AdjEntryArrayBase() { }
    //! Constructs an adjacency entry array associated with \a G.
    AdjEntryArray(const Graph & G) : Array<T>(G.adjEntryArrayTableSize()), AdjEntryArrayBase(&G) { }
    //! Constructs an adjacency entry array associated with \a G.
    /**
     * @param G is the associated graph.
     * @param x is the default value for all array elements.
     */
    AdjEntryArray(const Graph & G, const T & x) :
        Array<T>(0, G.adjEntryArrayTableSize() - 1, x), AdjEntryArrayBase(&G), m_x(x) { }
    //! Constructs an adjacency entry array that is a copy of \a A.
    /**
     * Associates the array with the same graph as \a A and copies all elements.
     */
    AdjEntryArray(const AdjEntryArray<T> & A) : Array<T>(A), AdjEntryArrayBase(A.m_pGraph), m_x(A.m_x) { }

    //! Returns true iff the array is associated with a graph.
    bool valid() const
    {
        return (Array<T>::low() <= Array<T>::high());
    }

    //! Returns a reference to the element with index \a adj.
    const T & operator[](adjEntry adj) const
    {
        OGDF_ASSERT(adj != 0 && adj->graphOf() == m_pGraph)
        return Array<T>::operator [](adj->index());
    }

    //! Returns a reference to the element with index \a adj.
    T & operator[](adjEntry adj)
    {
        OGDF_ASSERT(adj != 0 && adj->graphOf() == m_pGraph)
        return Array<T>::operator [](adj->index());
    }

    //! Returns a reference the element with index \a index.
    /**
     * \attention Make sure that \a index is a valid index for an adjacency
     * entry in the associated graph!
     */
    const T & operator[](int index) const
    {
        return Array<T>::operator [](index);
    }

    //! Returns a reference the element with index \a index.
    /**
     * \attention Make sure that \a index is a valid index for an adjacency
     * entry in the associated graph!
     */
    T & operator[](int index)
    {
        return Array<T>::operator [](index);
    }

    //! Assignment operator.
    AdjEntryArray<T> & operator=(const AdjEntryArray<T> & A)
    {
        Array<T>::operator =(A);
        m_x = A.m_x;
        reregister(A.m_pGraph);
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
        Array<T>::init(G.adjEntryArrayTableSize());
        reregister(&G);
    }

    //! Reinitializes the array. Associates the array with \a G.
    /**
     * @param G is the associated graph.
     * @param x is the default value.
     */
    void init(const Graph & G, const T & x)
    {
        Array<T>::init(0, G.adjEntryArrayTableSize() - 1, m_x = x);
        reregister(&G);
    }

    //! Sets all array elements to \a x.
    void fill(const T & x)
    {
        int high = m_pGraph->maxAdjEntryIndex();
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

    virtual void resetIndex(int newIndex, int oldIndex)
    {
        Array<T>::operator [](newIndex) = Array<T>::operator [](oldIndex);
    }

    virtual void disconnect()
    {
        Array<T>::init();
        m_pGraph = 0;
    }

    OGDF_NEW_DELETE

}; // class AdjEntryArray<T>


} // end namespace ogdf


#endif
