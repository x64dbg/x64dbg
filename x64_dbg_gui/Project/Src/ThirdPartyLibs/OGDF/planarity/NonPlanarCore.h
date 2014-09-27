/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class NonPlanarCore which represents the
 *        non-planar core reduction for biconnected graphs.
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


#ifndef OGDF_NON_PLANAR_CORE_H
#define OGDF_NON_PLANAR_CORE_H


#include <ogdf/basic/Graph.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/EdgeArray.h>


namespace ogdf
{

class OGDF_EXPORT SPQRTree;
class OGDF_EXPORT Skeleton;


//---------------------------------------------------------
// NonPlanarCore
//---------------------------------------------------------
class OGDF_EXPORT NonPlanarCore
{
public:
    NonPlanarCore(const Graph & G);

    const Graph & core() const
    {
        return m_graph;
    }
    const Graph & originalGraph() const
    {
        return *m_pOriginal;
    }

    node original(node v) const
    {
        return m_orig[v];
    }

    bool isVirtual(edge e) const
    {
        return m_real[e] == 0;
    }
    edge realEdge(edge e) const
    {
        return m_real[e];
    }

    const EdgeArray<int> & cost() const
    {
        return m_cost;
    }
    int cost(edge e) const
    {
        return m_cost[e];
    }
    const List<edge> & mincut(edge e) const
    {
        return m_mincut[e];
    }

protected:
    void markCore(const SPQRTree & T, NodeArray<bool> & mark);
    void traversingPath(Skeleton & S, edge eS, List<edge> & path, NodeArray<node> & mapV);

    Graph m_graph;
    const Graph* m_pOriginal;

    NodeArray<node> m_orig;  // corresp. original node
    EdgeArray<edge> m_real;  // corresp. original edge (0 if virtual)
    EdgeArray<List<edge> > m_mincut;  // traversing path for an edge in the core
    EdgeArray<int> m_cost;
}; // class NonPlanarCore


} // end namespace ogdf


#endif
