/*
 * $Revision: 2615 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 14:23:36 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of extended graph algorithms
 *
 * \author Sebastian Leipert, Karsten Klein, Markus Chimani
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

#ifndef OGDF_EXTENDED_GRAPH_ALG_H
#define OGDF_EXTENDED_GRAPH_ALG_H


#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/basic/BinaryHeap2.h>
#include <ogdf/planarity/BoyerMyrvold.h>
#include <limits>

namespace ogdf
{


//---------------------------------------------------------
// Methods for induced subgraphs
//---------------------------------------------------------

//! Computes the subgraph induced by a list of nodes.
/**
 * @tparam NODELISTITERATOR is the type of iterators for the input list of nodes.
 * @param G        is the input graph.
 * @param start    is a list iterator pointing to the first element in a list of nodes, for which
 *                 an induced subgraph shall be computed.
 * @param subGraph is assigned the computed subgraph.
 */
template<class LISTITERATOR>
void inducedSubGraph(const Graph & G, LISTITERATOR start, Graph & subGraph)
{
    NodeArray<node> nodeTableOrig2New;
    inducedSubGraph(G, start, subGraph, nodeTableOrig2New);
}

//! Computes the subgraph induced by a list of nodes (plus a mapping from original nodes to new copies).
/**
 * @tparam NODELISTITERATOR is the type of iterators for the input list of nodes.
 * @param G        is the input graph.
 * @param start    is a list iterator pointing to the first element in a list of nodes, for which
 *                 an induced subgraph shall be computed.
 * @param subGraph is assigned the computed subgraph.
 * @param nodeTableOrig2New is assigned a mapping from the nodes in \a G to the nodes in \a subGraph.
 */
template<class LISTITERATOR>
void inducedSubGraph(
    const Graph & G,
    LISTITERATOR start,
    Graph & subGraph,
    NodeArray<node> & nodeTableOrig2New)
{
    subGraph.clear();
    nodeTableOrig2New.init(G, 0);

    EdgeArray<bool> mark(G, false);

    LISTITERATOR its;
    for(its = start; its.valid(); its++)
    {
        node w = (*its);
        OGDF_ASSERT(w != 0 && w->graphOf() == &G);
        nodeTableOrig2New[w] = subGraph.newNode();

        adjEntry adj = w->firstAdj();
        forall_adj(adj, w)
        {
            edge e = adj->theEdge();
            if(nodeTableOrig2New[e->source()] && nodeTableOrig2New[e->target()] && !mark[e])
            {
                subGraph.newEdge(nodeTableOrig2New[e->source()], nodeTableOrig2New[e->target()]);
                mark[e] = true;
            }
        }
    }
}


//! Computes the subgraph induced by a list of nodes (plus mappings from original nodes and edges to new copies).
/**
 * @tparam NODELISTITERATOR is the type of iterators for the input list of nodes.
 * @param G        is the input graph.
 * @param start    is a list iterator pointing to the first element in a list of nodes, for which
 *                 an induced subgraph shall be computed.
 * @param subGraph is assigned the computed subgraph.
 * @param nodeTableOrig2New is assigned a mapping from the nodes in \a G to the nodes in \a subGraph.
 * @param edgeTableOrig2New is assigned a mapping from the edges in \a G to the egdes in \a subGraph.
 */
template<class LISTITERATOR>
void inducedSubGraph(
    const Graph & G,
    LISTITERATOR start,
    Graph & subGraph,
    NodeArray<node> & nodeTableOrig2New,
    EdgeArray<edge> & edgeTableOrig2New)
{
    subGraph.clear();
    nodeTableOrig2New.init(G, 0);
    edgeTableOrig2New.init(G, 0);

    EdgeArray<bool> mark(G, false);

    LISTITERATOR its;
    for(its = start; its.valid(); its++)
    {
        node w = (*its);
        OGDF_ASSERT(w != 0 && w->graphOf() == &G);
        nodeTableOrig2New[w] = subGraph.newNode();

        adjEntry adj = w->firstAdj();
        forall_adj(adj, w)
        {
            edge e = adj->theEdge();
            if(nodeTableOrig2New[e->source()] &&
                    nodeTableOrig2New[e->target()] &&
                    !mark[e])
            {
                edgeTableOrig2New[e] =
                    subGraph.newEdge(
                        nodeTableOrig2New[e->source()],
                        nodeTableOrig2New[e->target()]);
                mark[e] = true;
            }
        }
    }
}


//! Computes the edges in a node-induced subgraph.
/**
 * @tparam NODELISTITERATOR is the type of iterators for the input list of nodes.
 * @tparam EDGELIST         is the type of the returned edge list.
 * @param  G  is the input graph.
 * @param  it is a list iterator pointing to the first element in a list of nodes, whose
 *            induced subgraph is considered.
 * @param  E  is assigned the list of edges in the node-induced subgraph.
 */
template<class NODELISTITERATOR, class EDGELIST>
void inducedSubgraph(Graph & G, NODELISTITERATOR & it, EDGELIST & E)
{
    NODELISTITERATOR itBegin = it;
    NodeArray<bool>  mark(G, false);

    for(; it.valid(); it++)
        mark[(*it)] = true;
    it = itBegin;
    for(; it.valid(); it++)
    {
        node v = (*it);
        adjEntry adj;
        forall_adj(adj, v)
        {
            edge e = adj->theEdge();
            if(mark[e->source()] && mark[e->target()])
                E.pushBack(e);
        }
    }
}


//---------------------------------------------------------
// Methods for clustered graphs
//---------------------------------------------------------


//! Returns true iff cluster graph \a C is c-connected.
OGDF_EXPORT bool isCConnected(const ClusterGraph & C);

//! Makes a cluster graph c-connected by adding edges.
/**
 * @param C is the input cluster graph.
 * @param G is the graph associated with the cluster graph \a C; the function adds new edges to this graph.
 * @param addedEdges is assigned the list of newly created edges.
 * @param simple selects the method used: If set to true, a simple variant that does not guarantee to preserve
 *        planarity is used.
 */
OGDF_EXPORT void makeCConnected(
    ClusterGraph & C,
    Graph & G,
    List<edge> & addedEdges,
    bool simple = true);




//---------------------------------------------------------
// Methods for st-numbering
//---------------------------------------------------------


//! Computes an st-Numbering of \a G.
/**
 * \pre \a G must be biconnected and simple, with the exception that
 * the graph is allowed to have isolated nodes. If both \a s and \a t
 * are set to nodes (both are not 0), they must be adjacent.
 *
 * @param G is the input graph.
 * @param numbering is assigned the st-number for each node.
 * @param s is the source node for the st-numbering.
 * @param t is the target node for the st-numbering.
 * @param randomized is only used when both \a s and \a t are not set (both are 0);
 *        in this case a random edge (s,t) is chosen; otherwise the first node s with degree
 *        > 0 is chosen and its first neighbor is used as t.
 * @return the number assigned to \a t, or 0 if no st-numbering could be computed.
 */
OGDF_EXPORT int stNumber(const Graph & G,
                         NodeArray<int> & numbering,
                         node s = 0,
                         node t = 0,
                         bool randomized = false);

//! Tests, whether a numbering of the nodes is an st-numbering.
/**
 * \pre \a G must be biconnected and simple, with the exception that
 * the graph is allowed to have isolated nodes.
 */
OGDF_EXPORT bool testSTnumber(const Graph & G, NodeArray<int> & st_no, int max);


//---------------------------------------------------------
// Methods for minimum spanning tree computation
//---------------------------------------------------------

//! Computes a minimum spanning tree using Prim's algorithm
/**
 * @tparam T        is the numeric type for edge weights.
 * @param  G        is the input graph.
 * @param  weight   is an edge array with the edge weights.
 * @param  isInTree is assigned the result, i.e. \a isInTree[\a e] is true iff edge \a e is in the computed MST.
 * @return the sum of the edge weights in the computed tree.
 **/
template<typename T>
T computeMinST(const Graph & G, const EdgeArray<T> & weight, EdgeArray<bool> & isInTree)
{
    NodeArray<edge> pred(G, 0);
    return computeMinST(G.firstNode(), G, weight, pred, isInTree);
}


//! Computes a minimum spanning tree (MST) using Prim's algorithm
/**
 * @tparam T        is the numeric type for edge weights.
 * @param  G        is the input graph.
 * @param  weight   is an edge array with the edge weights.
 * @param  isInTree is assigned the result, i.e. \a isInTree[\a e] is true iff edge \a e is in the computed MST.
 * @param  pred     is assigned for each node the edge from its parent in the MST.
 * @return the sum of the edge weights in the computed tree.
 **/
template<typename T>
T computeMinST(const Graph & G, const EdgeArray<T> & weight, NodeArray<edge> & pred, EdgeArray<bool> & isInTree)
{
    return computeMinST(G.firstNode(), G, weight, pred, isInTree);
}


//! Computes a minimum spanning tree (MST) using Prim's algorithm
/**
 * @tparam T        is the numeric type for edge weights.
 * @param  s        is the start node for Prim's algorithm and will be the root of the MST.
 * @param  G        is the input graph.
 * @param  weight   is an edge array with the edge weights.
 * @param  isInTree is assigned the result, i.e. \a isInTree[\a e] is true iff edge \a e is in the computed MST.
 * @param  pred     is assigned for each node the edge from its parent in the MST.
 * @return the sum of the edge weights in the computed tree.
 **/
template<typename T>
T computeMinST(node s, const Graph & G, const EdgeArray<T> & weight, NodeArray<edge> & pred, EdgeArray<bool> & isInTree)
{
    //our priority queue storing the front vertices
    BinaryHeap2<T, node> pq;

    //initialize tree flag for edges
    edge e;
    forall_edges(e, G)
    {
        isInTree[e] = false;
    }

    //array to save the position information from pq
    int* pqpos = new int[G.numberOfNodes()];

    int i = 0;
    NodeArray<int> vIndex(G);
    //array that tells us if node is already processed
    NodeArray<bool> processed(G);
    //predecessor of node v is given by an edge (w,v)

    //insert nodes into pr
    node v = G.firstNode();
    T prio = std::numeric_limits<T>::max();;
    while(v != 0)
    {
        vIndex[v] = i;
        pq.insert(v, prio, &(pqpos[i++]));
        processed[v] = false;
        pred[v] = 0;
        v = v->succ();
    }//while all nodes
    // decrease start node
    pq.decreaseKey(pqpos[vIndex[s]], 0.0);

    //extract the nodes again along a minimum ST
    while(!pq.empty())
    {
        v = pq.extractMin();
        processed[v] = true;
        forall_adj_edges(e, v)
        {
            node w = e->opposite(v);

            int posofw = pqpos[vIndex[w]];
            if((!processed[w]) && (weight[e] < pq.getPriority(posofw)))
            {
                pq.decreaseKey(posofw, weight[e]);
                pred[w] = e;
            }//if improvement
        }
    }//while pq

    //only for connected graphs
    int rootcount = 0;
    T treeWeight = 0.0;
    forall_nodes(v, G)
    {
        if(pred[v] == 0) rootcount++;
        else
        {
            isInTree[pred[v]] = true;
            treeWeight += weight[pred[v]];
        }
    }
    OGDF_ASSERT(rootcount == 1);

    delete[] pqpos;
    return treeWeight;
}//computeMinST


//! Returns true, if G is planar, false otherwise.
/**
 * This is a shortcut for BoyerMyrvold::isPlanar().
 *
 * @param G is the input graph.
 * @return true if \a G is planar, false otherwise.
 */
inline bool isPlanar(const Graph & G)
{
    return BoyerMyrvold().isPlanar(G);
}


//! Returns true, if G is planar, false otherwise. If true is returned, G will be planarly embedded.
/**
 * This is a shortcut for BoyerMyrvold::planarEmbed
 *
 * @param G is the input graph.
 * @return true if \a G is planar, false otherwise.
 */
inline bool planarEmbed(Graph & G)
{
    return BoyerMyrvold().planarEmbed(G);
}


//! Constructs a planar embedding of G. It assumes that \a G is planar!
/**
 * This routine is slightly faster than planarEmbed(), but requires \a G to be planar.
 * If \a G is not planar, the graph will be destroyed while trying to embed it!
 *
 * This is a shortcut for BoyerMyrvold::planarEmbedPlanarGraph().
 *
 * @param G is the input graph.
 * @return true if the embedding was successful; false, if the given graph was non-planar (in this case
 *         the graph will be left in an at least partially deleted state).
 *
 */
inline bool planarEmbedPlanarGraph(Graph & G)
{
    return BoyerMyrvold().planarEmbedPlanarGraph(G);
}

} // end namespace ogdf


#endif
