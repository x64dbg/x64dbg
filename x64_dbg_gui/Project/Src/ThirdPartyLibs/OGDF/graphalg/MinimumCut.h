/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declares & implements a minimum-cut algorithm according
 * to an approach of Stoer and Wagner 1997. However, no Priority Queues
 * are used as suggested in the approach. Should be adapted to improve
 * performance.
 *
 * \author Mathias Jansen
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

#ifndef OGDF_MINIMUM_CUT_H
#define OGDF_MINIMUM_CUT_H

#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/basic/simple_graph_alg.h>

namespace ogdf
{


class OGDF_EXPORT MinCut
{

public:
    //Todo: Shift parameters to the call!
    //m_minCut is only initialized once!!!
    MinCut(Graph & G, EdgeArray<double> & w);
    ~MinCut();

    // implements the main loop that computes the minimum cut by invoking function
    // minimumCutPhase() in each iteration. Returns the mincut value.
    double minimumCut();

    // returns the edges defining the computed mincut in list \a edges.
    void cutEdges(List<edge> & edges, Graph & G);

    // returns list of nodes belonging to one side of the bipartition in list \a nodes.
    void partition(List<node> & nodes);

    double minCutValue() const
    {
        return m_minCut;
    }

private:

    // stores the value of the minimum cut
    double m_minCut;

    // GraphCopy of the corresponding Graph. Used for the computation in order not
    // to destroy the original Graph.
    GraphCopy m_GC;

    // an EdgeArray containing the corresponding edge weights.
    EdgeArray<double> m_w;

    // the two node lists corresponding to the node contraction
    List<node> m_contraction1, m_contraction2;

    // store one side of the computed bipartition.
    List<node> m_partition;

    // the list of edges defining the cut
    List<edge> m_cutEdges;

    // each node has a list containing the nodes with which it has been contracted.
    // Because the GraphCopy \a m_GC is destroyed during the algorithm, this is
    // necessary to be able to determine the original nodes in the end.
    NodeArray<List<node> > m_contractedNodes;

    // computes and returns the value of the minimum cut of the current phase (itertion).
    double minimumCutPhase();

    // Contracts the nodes \a s and \a t, i.e \a s is collapsed to \a t.
    // The edge (if existing) between \a s and \t s is deleted. Edges incident to \a s are redirected to \t.
    // If parallel edges occur, one of them is deleted and its weight is added to the other one.
    void contraction(node t, node s);

};

}// end namespace

#endif
