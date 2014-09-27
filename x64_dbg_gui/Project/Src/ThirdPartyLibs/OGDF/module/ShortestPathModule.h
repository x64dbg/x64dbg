/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of base class of shortest path algorithms
 *        including some useful functions dealing with
 *        shortest paths flow (generater, checker).
 *
 * \author Gunnar W. Klau
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

#ifndef OGDF_SHORTEST_PATH_MODULE_H
#define OGDF_SHORTEST_PATH_MODULE_H


#include <ogdf/basic/Graph.h>


namespace ogdf
{


class OGDF_EXPORT ShortestPathModule
{
public:
    ShortestPathModule() { }
    virtual ~ShortestPathModule() {}

    // computes shortest paths
    // Precond.:
    // returns true iff a feasible min-cost flow exists
    virtual bool call(
        const Graph & G,                  // directed graph
        const node s,                     // source node
        const EdgeArray<int> & length,    // length of an edge
        NodeArray<int> & d,               // contains shortest path distances after call
        NodeArray<edge> & pi
    ) = 0;



protected:
    //
    // static functions
    //

    // generates a shortest path problem instance with n nodes and m+n edges
    /*
    static void generateProblem(
        Graph &G,
        int n,
        int m,
        EdgeArray<int> &lowerBound,
        EdgeArray<int> &upperBound,
        EdgeArray<int> &cost,
        NodeArray<int> &supply);


    // checks if a given min-cost flow problem instance satisfies
    // the preconditions
    //
    //    lowerBound[e] <= upperBound[e] for all edges e
    //    cost[e] >= 0 for all edges e
    //    sum over all supply[v] = 0
    static bool checkProblem(
        const Graph &G,
        const EdgeArray<int> &lowerBound,
        const EdgeArray<int> &upperBound,
        const EdgeArray<int> &cost,
        const NodeArray<int> &supply);



    // checks if a computed flow is a feasible solution to the given problem
    // instance, i.e., checks if
    //    lowerBound[e] <= flow[e] <= upperBound[e]
    //    sum flow[e], e is outgoing edge of v -
    //      sum flow[e], e is incoming edge of v = supply[v] for each v
    // returns true iff the solution is feasible and in value the value of
    //   the computed flow
    static bool checkComputedFlow(
        Graph &G,
        EdgeArray<int> &lowerBound,
        EdgeArray<int> &upperBound,
        EdgeArray<int> &cost,
        NodeArray<int> &supply,
        EdgeArray<int> &flow,
        int &value);

    static bool checkComputedFlow(
        Graph &G,
        EdgeArray<int> &lowerBound,
        EdgeArray<int> &upperBound,
        EdgeArray<int> &cost,
        NodeArray<int> &supply,
        EdgeArray<int> &flow)
    {
        int value;
        return checkComputedFlow(
            G,lowerBound,upperBound,cost,supply,flow,value);
    }
    */
};


} // end namespace ogdf


#endif
