/*
 * $Revision: 2584 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-12 02:38:07 +0200 (Do, 12. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of ClusterArray class.
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

#ifndef OGDF_CLUSTER_ARRAY_H
#define OGDF_CLUSTER_ARRAY_H


#include <ogdf/basic/Array.h>
#include <ogdf/cluster/ClusterGraph.h>


namespace ogdf
{


//---------------------------------------------------------
// ClusterArrayBase
// base class for ClusterArray<T>, defines interface for event handling
// used by Graph
//---------------------------------------------------------
//! Abstract base class for cluster arrays.
/**
 * Defines the interface for event handling used by the ClusterGraph class.
 * Use the paramiterized class ClusterArray for creating edge arrays.
 */
class ClusterArrayBase
{
    /**
     * Pointer to list element in the list of all registered cluster
     * arrays which references this array.
     */
    ListIterator<ClusterArrayBase*> m_it;

public:
    const ClusterGraph* m_pClusterGraph; //!< The associated cluster graph.

    //! Initializes a cluster array not associated with a cluster graph.
    ClusterArrayBase() : m_pClusterGraph(0) { }
    //! Initializes a cluster array associated with \a pC.
    ClusterArrayBase(const ClusterGraph* pC) : m_pClusterGraph(pC)
    {
        if(pC) m_it = pC->registerArray(this);
    }

    // destructor, unregisters the array
    virtual ~ClusterArrayBase()
    {
        if(m_pClusterGraph) m_pClusterGraph->unregisterArray(m_it);
    }

    // event interface used by Graph
    //! Virtual function called when table size has to be enlarged.
    virtual void enlargeTable(int newTableSize) = 0;
    //! Virtual function called when table has to be reinitialized.
    virtual void reinit(int initTableSize) = 0;
    //! Virtual function called when array is disconnected from the cluster graph.
    virtual void disconnect() = 0;

    //! Associates the array with a new cluster graph.
    void reregister(const ClusterGraph* pC)
    {
        if(m_pClusterGraph) m_pClusterGraph->unregisterArray(m_it);
        if((m_pClusterGraph = pC) != 0) m_it = pC->registerArray(this);
    }
}; // class ClusterArrayBase


//! Dynamic arrays indexed with clusters.
/**
 * Cluster arrays adjust their table size automatically
 * when the cluster graph grows.
 */
template<class T> class ClusterArray : private Array<T>, protected ClusterArrayBase
{
    T m_x; //!< The default value for array elements.

public:
    //! Constructs an empty cluster array associated with no graph.
    ClusterArray() : Array<T>(), ClusterArrayBase() { }
    //! Constructs a cluster array associated with \a C.
    ClusterArray(const ClusterGraph & C) :
        Array<T>(C.clusterArrayTableSize()),
        ClusterArrayBase(&C) { }
    //! Constructs a cluster array associated with \a C.
    /**
     * @param C is the associated cluster graph.
     * @param x is the default value for all array elements.
     */
    ClusterArray(const ClusterGraph & C, const T & x) :
        Array<T>(0, C.clusterArrayTableSize() - 1, x),
        ClusterArrayBase(&C), m_x(x) { }
    //! Constructs a cluster array associated with \a C and a given
    //! size (for static use).
    /**
     * @param C is the associated cluster graph.
     * @param x is the default value for all array elements.
     * @param size is the size of the array.
     */
    ClusterArray(const ClusterGraph & C, const T & x, int size) :
        Array<T>(0, size - 1, x),
        ClusterArrayBase(&C), m_x(x) { }

    //! Constructs a cluster array that is a copy of \a A.
    /**
     * Associates the array with the same cluster graph as \a A and copies all elements.
     */
    ClusterArray(const ClusterArray<T> & A) :
        Array<T>(A),
        ClusterArrayBase(A.m_pClusterGraph), m_x(A.m_x) { }

    //! Returns true iff the array is associated with a graph.
    bool valid() const
    {
        return (Array<T>::low() <= Array<T>::high());
    }

    //! Returns a pointer to the associated cluster graph.
    const ClusterGraph* graphOf() const
    {
        return m_pClusterGraph;
    }

    //! Returns a reference to the element with index \a c.
    const T & operator[](cluster c) const
    {
        OGDF_ASSERT(c != 0 && c->graphOf() == m_pClusterGraph)
        return Array<T>::operator [](c->index());
    }

    //! Returns a reference to the element with index \a c.
    T & operator[](cluster c)
    {
        OGDF_ASSERT(c != 0 && c->graphOf() == m_pClusterGraph)
        return Array<T>::operator [](c->index());
    }

    //! Returns a reference to the element with index \a index.
    /**
     * \attention Make sure that \a index is a valid index for a cluster
     * in the associated cluster graph!
     */
    const T & operator[](int index) const
    {
        return Array<T>::operator [](index);
    }

    //! Returns a reference to the element with index \a index.
    /**
     * \attention Make sure that \a index is a valid index for a cluster
     * in the associated cluster graph!
     */
    T & operator[](int index)
    {
        return Array<T>::operator [](index);
    }

    //! Assignment operator.
    ClusterArray<T> & operator=(const ClusterArray<T> & a)
    {
        Array<T>::operator =(a);
        m_x = a.m_x;
        reregister(a.m_pClusterGraph);
        return *this;
    }

    //! Reinitializes the array. Associates the array with no cluster graph.
    void init()
    {
        Array<T>::init();
        reregister(0);
    }

    //! Reinitializes the array. Associates the array with \a C.
    void init(const ClusterGraph & C)
    {
        Array<T>::init(C.clusterArrayTableSize());
        reregister(&C);
    }

    //! Reinitializes the array. Associates the array with \a C.
    /**
     * @param C is the associated cluster graph.
     * @param x is the default value.
     */
    void init(const ClusterGraph & C, const T & x)
    {
        Array<T>::init(0, C.clusterArrayTableSize() - 1, m_x = x);
        reregister(&C);
    }

    //! Sets all array elements to \a x.
    void fill(const T & x)
    {
        int high = m_pClusterGraph->maxClusterIndex();
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
        m_pClusterGraph = 0;
    }

    OGDF_NEW_DELETE

}; // class ClusterArray<T>


} // end namespace ogdf


#endif
