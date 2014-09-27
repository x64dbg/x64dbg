/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of Dijkstra's single source shortest path algorithm
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

#ifndef OGDF_DIJKSTRA_H_
#define OGDF_DIJKSTRA_H_

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/BinaryHeap2.h>
#include <limits>

namespace ogdf
{

/*!
 * \brief Dijkstra's single source shortest path algorithm.
 *
 * This class implements Dijkstra's algorithm for computing single source shortest path.
 * It requires a graph with proper, positive edge weights and returns a predecessor array
 * as well as the shortest distances from the source node to all others.
 */
template<typename T>
class Dijkstra
{
public:

    /*!
     * \brief Calculates, based on the graph G with corresponding edge costs and a source node s,
     * the shortest paths and distances to all other nodes by Dijkstra's algorithm.
     * @param G The original input graph
     * @param weight The edge weights
     * @param s The source node
     * @param predecessor The resulting predecessor relation
     * @param distance The resulting distances to all other nodes
     */
    void call(const Graph & G, const EdgeArray<T> & weight, node s, NodeArray<edge> & predecessor,
              NodeArray<T> & distance)
    {
        BinaryHeap2<T, node> queue(G.numberOfNodes());
        int* qpos = new int[G.numberOfNodes()];
        NodeArray<int> vIndex(G);
        T maxEdgeWeight = 0;
        int i = 0;
        node v;
        edge e;

        // determining maximum edge weight
        forall_edges(e, G)
        {
            if(maxEdgeWeight < weight[e])
            {
                maxEdgeWeight = weight[e];
            }
        }

        // setting distances to "infinity"
        forall_nodes(v, G)
        {
            vIndex[v] = i;
            distance[v] = std::numeric_limits<T>::max() - maxEdgeWeight - 1;
            predecessor[v] = 0;
            queue.insert(v, distance[v], &qpos[i++]);
        }

        distance[s] = 0;
        queue.decreaseKey(qpos[vIndex[s]], 0);

        while(!queue.empty())
        {
            v = queue.extractMin();
            forall_adj_edges(e, v)
            {
                node w = e->opposite(v);
                if(distance[w] > distance[v] + weight[e])
                {
                    distance[w] = distance[v] + weight[e];
                    queue.decreaseKey(qpos[vIndex[w]], distance[w]);
                    predecessor[w] = e;
                }
            }
        }
        delete[] qpos;
    }

};

} // end namespace ogdf

#endif /* OGDF_DIJKSTRA_H_ */
