/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Extends the GraphCopy concept to weighted graphs
 *
 * \author Matthias Woste
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

#ifndef OGDF_EDGE_WEIGHTED_GRAPH_COPY_H_
#define OGDF_EDGE_WEIGHTED_GRAPH_COPY_H_

#include <ogdf/basic/GraphCopy.h>
#include <ogdf/internal/steinertree/EdgeWeightedGraph.h>

namespace ogdf
{

template<typename T>
class EdgeWeightedGraphCopy: public GraphCopy
{
public:
    EdgeWeightedGraphCopy() :
        GraphCopy()
    {
    }
    EdgeWeightedGraphCopy(const EdgeWeightedGraph<T> & wC);
    EdgeWeightedGraphCopy(const EdgeWeightedGraphCopy & wGC);
    EdgeWeightedGraphCopy & operator=(const EdgeWeightedGraphCopy & wGC);
    virtual ~EdgeWeightedGraphCopy()
    {
    }
    ;
    void createEmpty(const EdgeWeightedGraph<T> & wG);
    edge newEdge(node u, node v, T weight);
    T weight(edge e) const
    {
        return m_edgeWeight[e];
    }
    void setWeight(edge e, T v)
    {
        m_edgeWeight[e] = v;
    }
    EdgeArray<T> edgeWeights() const
    {
        return m_edgeWeight;
    }

protected:
    EdgeArray<T> m_edgeWeight;

private:
    void initWGC(const EdgeWeightedGraphCopy & wGC, NodeArray<node> & vCopy, EdgeArray<edge> & eCopy);
};

}

// Implementation

namespace ogdf
{

template<typename T>
void EdgeWeightedGraphCopy<T>::initWGC(const EdgeWeightedGraphCopy & wGC, NodeArray<node> & vCopy, EdgeArray<edge> & eCopy)
{
    m_pGraph = wGC.m_pGraph;

    m_vOrig.init(*this, 0);
    m_eOrig.init(*this, 0);
    m_vCopy.init(*m_pGraph, 0);
    m_eCopy.init(*m_pGraph);
    m_eIterator.init(*this, 0);

    node v, w;
    forall_nodes(v, wGC)
    m_vOrig[vCopy[v]] = wGC.original(v);

    edge e;
    forall_edges(e, wGC)
    m_eOrig[eCopy[e]] = wGC.original(e);

    forall_nodes(v, *this)
    if((w = m_vOrig[v]) != 0)
        m_vCopy[w] = v;

    forall_edges(e, *m_pGraph)
    {
        ListConstIterator<edge> it;
        for(it = wGC.m_eCopy[e].begin(); it.valid(); ++it)
            m_eIterator[eCopy[*it]] = m_eCopy[e].pushBack(eCopy[*it]);
    }

    m_edgeWeight = EdgeArray<T>((*this));

    forall_edges(e, wGC)
    {
        m_edgeWeight[eCopy[e]] = wGC.weight(e);
    }
}

template<typename T>
EdgeWeightedGraphCopy<T> & EdgeWeightedGraphCopy<T>::operator=(const EdgeWeightedGraphCopy<T> & wGC)
{

    GraphCopy::operator =(wGC);

    m_edgeWeight = EdgeArray<T>((*this));

    edge e, f;
    forall_edges(e, wGC)
    {
        f = wGC.original(e);
        m_edgeWeight[copy(f)] = wGC.weight(e);
    }

}

template<typename T>
EdgeWeightedGraphCopy<T>::EdgeWeightedGraphCopy(const EdgeWeightedGraphCopy<T> & wGC) :
    GraphCopy(wGC)
{
    m_edgeWeight = EdgeArray<T>((*this));

    edge e, f;
    forall_edges(e, wGC)
    {
        f = wGC.original(e);
        m_edgeWeight[copy(f)] = wGC.weight(e);
    }
}

template<typename T>
EdgeWeightedGraphCopy<T>::EdgeWeightedGraphCopy(const EdgeWeightedGraph<T> & wG) :
    GraphCopy(wG)
{
    m_edgeWeight = EdgeArray<T>((*this));

    edge e;
    forall_edges(e, (*this))
    {
        m_edgeWeight[e] = wG.weight(original(e));
    }
}

template<typename T>
void EdgeWeightedGraphCopy<T>::createEmpty(const EdgeWeightedGraph<T> & wG)
{
    GraphCopy::createEmpty(wG);
    m_pGraph = &wG;
    m_edgeWeight = EdgeArray<T>(*this);
}

template<typename T>
edge EdgeWeightedGraphCopy<T>::newEdge(node u, node v, T weight)
{
    edge e = GraphCopy::newEdge(u, v);
    m_edgeWeight[e] = weight;
    return e;
}

}

#endif /* OGDF_EDGE_WEIGHTED_GRAPH_COPY_H_ */
