/*
 * $Revision: 2593 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-15 15:33:53 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of simple graph algorithms.
 *
 * \author Carsten Gutwenger and Sebastian Leipert
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

#ifndef OGDF_SIMPLE_GRAPH_ALG_H
#define OGDF_SIMPLE_GRAPH_ALG_H


#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/SList.h>
#include <ogdf/basic/BoundedStack.h>

namespace ogdf
{


//---------------------------------------------------------
// Methods for loops
//---------------------------------------------------------

//! Returns true iff \a G contains no self-loop.
/**
 * @param G is the input graph.
 * @return true if \a G contains no self-loops (edges whose two endpoints are the same), false otherwise.
 */
OGDF_EXPORT bool isLoopFree(const Graph & G);

//! Removes all self-loops from \a G and returns all nodes with self-loops in \a L.
/**
 * @tparam NODELIST is the type of the node list for returning the nodes with self-loops.
 * @param  G is the input graph.
 * @param  L is assigned the list of nodes with self-loops.
 */
template<class NODELIST>
void makeLoopFree(Graph & G, NODELIST & L)
{
    L.clear();

    edge e, eNext;
    for(e = G.firstEdge(); e; e = eNext)
    {
        eNext = e->succ();
        if(e->isSelfLoop())
        {
            L.pushBack(e->source());
            G.delEdge(e);
        }
    }
}


//! Removes all self-loops from \a G.
/**
 * @param  G is the input graph.
 */
OGDF_EXPORT void makeLoopFree(Graph & G);


//---------------------------------------------------------
// Methods for parallel edges
//---------------------------------------------------------

//! Sorts the edges of \a G such that parallel edges come after each other in the list.
/**
 * @param G is the input graph.
 * @param edges is assigned the list of sorted edges.
 */
OGDF_EXPORT void parallelFreeSort(const Graph & G, SListPure<edge> & edges);


//! Returns true iff \a G contains no paralle edges.
/**
 * A parallel edge is an edge e1=(v,w) such that there exists another edge e2=(v,w) in
 * the graph. Reversal edges (e.g. (v,w) and (w,v)) are not parallel edges. If you want to
 * test if a graph contains no undirected parallel edges, use isParallelFreeUndirected().
 *
 * @param G is the input graph.
 * @return true if \a G contains no multi-edges (edges with the same source and target).
 */
OGDF_EXPORT bool isParallelFree(const Graph & G);


//! Returns the number of parallel edges in \a G.
/**
 * A parallel edge is an edge e1=(v,w) such that there exists another edge e2=(v,w) in
 * the graph. Reversal edges (e.g. (v,w) and (w,v)) are not parallel edges. If you want to
 * also take reversal edges into account, use numParallelEdgesUndirected().
 *
 * @param G is the input graph.
 * @return is the number of parallel edges: for each bundle of parallel edges between two nodes
 *         v and w, all but one are counted.
 */
OGDF_EXPORT int numParallelEdges(const Graph & G);


//! Removes all but one of each bundle of parallel edges.
/**
 * A parallel edge is an edge e1=(v,w) such that there exists another edge e2=(v,w) in
 * the graph. Reversal edges (e.g. (v,w) and (w,v)) are not multi-edges. If you want to
 * remove parallel and reversal edges, use makeParallelFreeUndirected(Graph&,EDGELIST&).
 *
 * @tparam EDGELIST      is the type of edge list that will be assigned the list of parallel edges.
 * @param  G             is the input graph.
 * @param  parallelEdges is assigned the list of remaining edges in \a G that were part of a
 *                       bundle of parallel edges in the input graph.
 */
template <class EDGELIST>
void makeParallelFree(Graph & G, EDGELIST & parallelEdges)
{
    parallelEdges.clear();
    if(G.numberOfEdges() <= 1) return;

    SListPure<edge> edges;
    parallelFreeSort(G, edges);

    SListConstIterator<edge> it = edges.begin();
    edge ePrev = *it++, e;
    bool bAppend = true;
    while(it.valid())
    {
        e = *it++;
        if(ePrev->source() == e->source() && ePrev->target() == e->target())
        {
            G.delEdge(e);
            if(bAppend)
            {
                parallelEdges.pushBack(ePrev);
                bAppend = false;
            }
        }
        else
        {
            ePrev = e;
            bAppend = true;
        }
    }
}


//! Removes all but one edge of each bundle of parallel edges in \a G.
/**
 * A parallel edge is an edge e1=(v,w) such that there exists another edge e2=(v,w) in
 * the graph. Reversal edges (e.g. (v,w) and (w,v)) are not parallel edges. If you want to
 * remove parallel and reversal edges, use makeParallelFreeUndirected(Graph&).
 *
 * @param G is the input graph.
 */
inline void makeParallelFree(Graph & G)
{
    List<edge> parallelEdges;
    makeParallelFree(G, parallelEdges);
}



//! Sorts the edges of \a G such that undirected parallel edges come after each other in the list.
/**
 * An undirected parallel edges is an edge e1=(v,w) such that there exists another edge e2=(v,w) or (w,v)
 * in the graph.
 *
 * @param G is the input graph.
 * @param edges is assigned the list of sorted edges.
 * @param minIndex is assigned for each edge (v,w) the index min(index(v),index(w)).
 * @param maxIndex is assigned for each edge (v,w) the index max(index(v),index(w)).
 */
OGDF_EXPORT void parallelFreeSortUndirected(
    const Graph & G,
    SListPure<edge> & edges,
    EdgeArray<int> & minIndex,
    EdgeArray<int> & maxIndex);


//! Returns true iff \a G contains no undirected parallel edges.
/**
 * An undirected parallel edges is an edge e1=(v,w) such that there exists another edge e2=(v,w) or (w,v)
 * in the graph.
 *
 * @param G is the input graph.
 * @return true if \a G contains no undirected parallel edges.
 */
OGDF_EXPORT bool isParallelFreeUndirected(const Graph & G);


//! Returns the number of undirected parallel edges in \a G.
/**
 * An undirected parallel edges is an edge e1=(v,w) such that there exists another edge e2=(v,w) or (w,v)
 * in the graph.
 *
 * @param G is the input graph.
 * @return the number of undirected parallel edges; for each unordered pair {v,w} of nodes, all
 *         but one of the edges with endpoints v and w (in any order) are counted.
 */
OGDF_EXPORT int numParallelEdgesUndirected(const Graph & G);


//! Removes all but one of each bundle of undirected parallel edges.
/**
 * An undirected parallel edges is an edge e1=(v,w) such that there exists another edge e2=(v,w) or (w,v)
 * in the graph. The function removes unordered pair {v,w} of nodes all but one of the edges with
 * endpoints v and w (in any order).
 *
 * @tparam EDGELIST      is the type of edge list that will be assigned the list of edges.
 * @param  G             is the input graph.
 * @param  parallelEdges is assigned the list of remaining edges that were part of a bundle
 *                       of undirected parallel edges in the input graph.
 */
template <class EDGELIST>
void makeParallelFreeUndirected(Graph & G, EDGELIST & parallelEdges)
{
    parallelEdges.clear();
    if(G.numberOfEdges() <= 1) return;

    SListPure<edge> edges;
    EdgeArray<int> minIndex(G), maxIndex(G);
    parallelFreeSortUndirected(G, edges, minIndex, maxIndex);

    SListConstIterator<edge> it = edges.begin();
    edge ePrev = *it++, e;
    bool bAppend = true;
    while(it.valid())
    {
        e = *it++;
        if(minIndex[ePrev] == minIndex[e] && maxIndex[ePrev] == maxIndex[e])
        {
            G.delEdge(e);
            if(bAppend)
            {
                parallelEdges.pushBack(ePrev);
                bAppend = false;
            }
        }
        else
        {
            ePrev = e;
            bAppend = true;
        }
    }
}


//! Removes all but one of each bundle of undirected parallel edges.
/**
 * An undirected parallel edges is an edge e1=(v,w) such that there exists another edge e2=(v,w) or (w,v)
 * in the graph. The function removes unordered pair {v,w} of nodes all but one of the edges with
 * endpoints v and w (in any order).
 *
 * @param G is the input graph.
 */
inline void makeParallelFreeUndirected(Graph & G)
{
    List<edge> parallelEdges;
    makeParallelFreeUndirected(G, parallelEdges);
}


//! Removes all but one of each bundle of undirected parallel edges.
/**
 * An undirected parallel edges is an edge e1=(v,w) such that there exists another edge e2=(v,w) or (w,v)
 * in the graph. The function removes unordered pair {v,w} of nodes all but one of the edges with
 * endpoints v and w (in any order).
 *
 * @tparam EDGELIST      is the type of edge list that is assigned the list of edges.
 * @param  G             is the input graph.
 * @param  parallelEdges is assigned the list of remaining edges that were
 *                       part of a bundle of undirected parallel edges in the input graph.
 * @param  cardPositive  contains for each edge the number of removed undirected parallel edges
 *                       pointing in the same direction.
 * @param  cardNegative  contains for each edge the number of removed undirected parallel edges
 *                       pointing in the opposite direction.
 */
template <class EDGELIST>
void makeParallelFreeUndirected(
    Graph & G,
    EDGELIST & parallelEdges,
    EdgeArray<int> & cardPositive,
    EdgeArray<int> & cardNegative)
{
    parallelEdges.clear();
    cardPositive.fill(0);
    cardNegative.fill(0);
    if(G.numberOfEdges() <= 1) return;

    SListPure<edge> edges;
    EdgeArray<int> minIndex(G), maxIndex(G);
    parallelFreeSortUndirected(G, edges, minIndex, maxIndex);

    SListConstIterator<edge> it = edges.begin();
    edge ePrev = *it++, e;
    bool bAppend = true;
    int  counter = 0;
    while(it.valid())
    {
        e = *it++;
        if(minIndex[ePrev] == minIndex[e] && maxIndex[ePrev] == maxIndex[e])
        {
            if(ePrev->source() == e->source() && ePrev->target() == e->target())
                cardPositive[ePrev]++;
            else if(ePrev->source() == e->target() && ePrev->target() == e->source())
                cardNegative[ePrev]++;
            G.delEdge(e);
            if(bAppend)
            {
                parallelEdges.pushBack(ePrev);
                bAppend = false;
            }
        }
        else
        {
            ePrev = e;
            bAppend = true;
        }
    }
}


//! Computes the bundles of undirected parallel edges in \a G.
/**
 * Stores for one (arbitrarily chosen) reference edge all edges belonging to the same bundle of
 * undirected parallel edges; no edge is removed from the graph.
 *
 * @tparam EDGELIST      is the type of edge list that is assigned the list of edges.
 * @param  G             is the input graph.
 * @param  parallelEdges is assigned for each reference edge the list of edges belonging to the
 *                       bundle of undirected parallel edges.
 */
template <class EDGELIST>
void getParallelFreeUndirected(const Graph & G, EdgeArray<EDGELIST> & parallelEdges)
{
    if(G.numberOfEdges() <= 1) return;

    SListPure<edge> edges;
    EdgeArray<int> minIndex(G), maxIndex(G);
    parallelFreeSortUndirected(G, edges, minIndex, maxIndex);

    SListConstIterator<edge> it = edges.begin();
    edge ePrev = *it++, e;
    while(it.valid())
    {
        e = *it++;
        if(minIndex[ePrev] == minIndex[e] && maxIndex[ePrev] == maxIndex[e])
            parallelEdges[ePrev].pushBack(e);
        else
            ePrev = e;
    }
}


//---------------------------------------------------------
// Methods for simple graphs
//---------------------------------------------------------


//! Returns true iff \a G contains neither self-loops nor parallel edges.
/**
 * @param G is the input graph.
 * @return true if \a G is simple, i.e. contains neither self-loops nor parallel edges, false otherwise.
 */
inline bool isSimple(const Graph & G)
{
    return isLoopFree(G) && isParallelFree(G);
}


//! Removes all self-loops and all but one edge of each bundle of parallel edges.
/**
 * @param G is the input graph.
 */
inline void makeSimple(Graph & G)
{
    makeLoopFree(G);
    makeParallelFree(G);
}


//! Returns true iff \a G contains neither self-loops nor undirected parallel edges.
/**
 * @param G is the input graph.
 * @return true if \a G is (undirected) simple, i.e. contains neither self-loops
 *         nor undirected parallel edges, false otherwise.
 */
inline bool isSimpleUndirected(const Graph & G)
{
    return isLoopFree(G) && isParallelFreeUndirected(G);
}


//! Removes all self-loops and all but one edge of each bundle of undirected parallel edges.
/**
 * @param G is the input graph.
 */
inline void makeSimpleUndirected(Graph & G)
{
    makeLoopFree(G);
    makeParallelFreeUndirected(G);
}



//---------------------------------------------------------
// Methods for connectivity
//---------------------------------------------------------

//! Returns true iff \a G is connected.
/**
 * @param G is the input graph.
 * @return true if \a G is connected, false otherwise.
 */
OGDF_EXPORT bool isConnected(const Graph & G);


//! Makes \a G connected by adding a minimum number of edges.
/**
 * @param G     is the input graph.
 * @param added is assigned the added edges.
 */
OGDF_EXPORT void makeConnected(Graph & G, List<edge> & added);


//! makes \a G connected by adding a minimum number of edges.
/**
 * @param G is the input graph.
 */
inline void makeConnected(Graph & G)
{
    List<edge> added;
    makeConnected(G, added);
}


//! Computes the connected components of \a G.
/**
 * Assigns component numbers (0, 1, ...) to the nodes of \a G. The component number of each
 * node is stored in the node array \a component.
 *
 * @param G         is the input graph.
 * @param component is assigned a mapping from nodes to component numbers.
 * @return the number of connected components.
 */
OGDF_EXPORT int connectedComponents(const Graph & G, NodeArray<int> & component);


//! Computes the connected components of \a G and returns the list of isolated nodes.
/**
 * Assigns component numbers (0, 1, ...) to the nodes of \a G. The component number of each
 * node is stored in the node array \a component.
 *
 * @param G         is the input graph.
 * @param isolated  is assigned the list of isolated nodes. An isolated
 *                  node is a node without incident edges.
 * @param component is assigned a mapping from nodes to component numbers.
 * @return the number of connected components.
 */
OGDF_EXPORT int connectedIsolatedComponents(
    const Graph & G,
    List<node> & isolated,
    NodeArray<int> & component);


//! Returns true iff \a G is biconnected.
/**
 * @param G is the input graph.
 * @param cutVertex If false is returned, \a cutVertex is assigned either 0 if \a G is not connected,
 *                  or a cut vertex in \a G.
 */
OGDF_EXPORT bool isBiconnected(const Graph & G, node & cutVertex);


//! Returns true iff \a G is biconnected.
/**
 * @param G is the input graph.
 */
inline bool isBiconnected(const Graph & G)
{
    node cutVertex;
    return isBiconnected(G, cutVertex);
}


//! Makes \a G biconnected by adding edges.
/**
 * @param G     is the input graph.
 * @param added is assigned the list of inserted edges.
 */
OGDF_EXPORT void makeBiconnected(Graph & G, List<edge> & added);


//! Makes \a G biconnected by adding edges.
/**
 * @param G is the input graph.
 */
inline void makeBiconnected(Graph & G)
{
    List<edge> added;
    makeBiconnected(G, added);
}


//! Computes the biconnected components of \a G.
/**
 * Assigns component numbers (0, 1, ...) to the edges of \ G. The component number of each edge
 * is stored in the edge array \a component.
 *
 * @param G         is the input graph.
 * @param component is assigned a mapping from edges to component numbers.
 * @return the number of biconnected components (including isolated nodes).
 */
OGDF_EXPORT int biconnectedComponents(const Graph & G, EdgeArray<int> & component);


//! Returns true iff \a G is triconnected.
/**
 * If true is returned, then either
 *   - \a s1 and \a s2 are either both 0 if \a G is not connected; or
 *   - \a s1 is a cut vertex and \a s2 = 0 if \a G is not biconnected; or
 *   - \a s1 and \a s2 are a separation pair otherwise.
 *
 * @param G is the input graph.
 * @param s1 is assigned a cut vertex of one node of a separation pair, if \a G is not triconnected (see above).
 * @param s2 is assigned one node of a separation pair, if \a G is not triconnected (see above).
 * @return true if \a G is triconnected, false otherwise.
 */
OGDF_EXPORT bool isTriconnected(const Graph & G, node & s1, node & s2);


//! Returns true iff \a G is triconnected.
/**
 * @param G is the input graph.
 * @return true if \a G is triconnected, false otherwise.
 */
inline bool isTriconnected(const Graph & G)
{
    node s1, s2;
    return isTriconnected(G, s1, s2);
}


//! Returns true iff \a G is triconnected (using a quadratic time algorithm!).
/**
 * If true is returned, then either
 *   - \a s1 and \a s2 are either both 0 if \a G is not connected; or
 *   - \a s1 is a cut vertex and \a s2 = 0 if \a G is not biconnected; or
 *   - \a s1 and \a s2 are a separation pair otherwise.
 *
 * \warning This method has quadratic running time. An efficient linear time
 *          version is provided by isTriconnected().
 *
 * @param G is the input graph.
 * @param s1 is assigned a cut vertex of one node of a separation pair, if \a G is not triconnected (see above).
 * @param s2 is assigned one node of a separation pair, if \a G is not triconnected (see above).
 * @return true if \a G is triconnected, false otherwise.
 */
OGDF_EXPORT bool isTriconnectedPrimitive(const Graph & G, node & s1, node & s2);


//! Returns true iff \a G is triconnected (using a quadratic time algorithm!).
/**
 * \warning This method has quadratic running time. An efficient linear time
 *          version is provided by isTriconnected().
 *
 * @param G is the input graph.
 * @return true if \a G is triconnected, false otherwise.
 */
inline bool isTriconnectedPrimitive(const Graph & G)
{
    node s1, s2;
    return isTriconnectedPrimitive(G, s1, s2);
}


//! Triangulates a planarly embedded graph \a G by adding edges.
/**
 * The result of this function is that \a G is made maximally planar by adding new edges.
 * \a G will also be planarly embedded such that each face is a triangle.
 *
 * \pre \a G is planar, simple and represents a combinatorial embedding (i.e. \a G is planarly embedded).
 *
 * @param G is the input graph to which edges will be added.
 */
void triangulate(Graph & G);


//---------------------------------------------------------
// Methods for directed graphs
//---------------------------------------------------------

//! Returns true iff the digraph \a G is acyclic.
/**
 * @param G         is the input graph
 * @param backedges is assigned the backedges of a DFS-tree.
 * @return true if \a G contains no directed cycle, false otherwise.
 */
OGDF_EXPORT bool isAcyclic(const Graph & G, List<edge> & backedges);


//! Returns true iff the digraph \a G is acyclic.
/**
 * @param G is the input graph
 * @return true if \a G contains no directed cycle, false otherwise.
 */
inline bool isAcyclic(const Graph & G)
{
    List<edge> backedges;
    return isAcyclic(G, backedges);
}


//! Returns true iff the undirected graph \a G is acyclic.
/**
 * @param G         is the input graph
 * @param backedges is assigned the backedges of a DFS-tree.
 * @return true if \a G contains no undirected cycle, false otherwise.
 */
OGDF_EXPORT bool isAcyclicUndirected(const Graph & G, List<edge> & backedges);


//! Returns true iff the undirected graph \a G is acyclic.
/**
 * @param G is the input graph
 * @return true if \a G contains no undirected cycle, false otherwise.
 */
inline bool isAcyclicUndirected(const Graph & G)
{
    List<edge> backedges;
    return isAcyclicUndirected(G, backedges);
}


//! Makes the digraph \a G acyclic by removing edges.
/**
 * The implementation removes all backedges of a DFS tree.
 *
 * @param G is the input graph
 */
OGDF_EXPORT void makeAcyclic(Graph & G);


//! Makes the digraph G acyclic by reversing edges.
/**
 * \remark The implementation ignores self-loops and reverses
 * the backedges of a DFS-tree.
 *
 * @param G is the input graph
 */
OGDF_EXPORT void makeAcyclicByReverse(Graph & G);


//! Returns true iff the digraph \a G contains exactly one source node (or is empty).
/**
 * @param G      is the input graph.
 * @param source is assigned the single source if true is returned, or 0 otherwise.
 * @return true if \a G has a single source, false otherwise.
 */
OGDF_EXPORT bool hasSingleSource(const Graph & G, node & source);


//! Returns true iff the digraph \a G contains exactly one source node (or is empty).
/**
 * @param G is the input graph.
 * @return true if \a G has a single source, false otherwise.
 */
inline bool hasSingleSource(const Graph & G)
{
    node source;
    return hasSingleSource(G, source);
}


//! Returns true iff the digraph \a G contains exactly one sink node (or is empty).
/**
 * @param G is the input graph.
 * @param sink is assigned the single sink if true is returned, or 0 otherwise.
 * @return true if \a G has a single sink, false otherwise.
 */
OGDF_EXPORT bool hasSingleSink(const Graph & G, node & sink);


//! Returns true iff the digraph \a G contains exactly one sink node (or is empty).
/**
 * @param G is the input graph.
 * @return true if \a G has a single sink, false otherwise.
 */
inline bool hasSingleSink(const Graph & G)
{
    node sink;
    return hasSingleSink(G, sink);
}


//! Returns true iff \a G is an st-digraph.
/**
 * A directed graph is an st-digraph if it is acyclic, contains exactly one source s
 * and one sink t, and the edge (s,t).
 *
 * @param G  is the input graph.
 * @param s  is assigned the single source (if true is returned).
 * @param t  is assigned the single sink (if true is returned).
 * @param st is assigned the edge (s,t) (if true is returned).
 * @return true if \a G is an st-digraph, false otherwise.
 */
OGDF_EXPORT bool isStGraph(const Graph & G, node & s, node & t, edge & st);


//! Returns true if \a G is an st-digraph.
/**
 * A directed graph is an st-digraph if it is acyclic, contains exactly one source s
 * and one sink t, and the edge (s,t).
 * @param G  is the input graph.
 * @return true if \a G is an st-digraph, false otherwise.
 */
inline bool isStGraph(const Graph & G)
{
    node s, t;
    edge st;
    return isStGraph(G, s, t, st);
}


//! Computes a topological numbering of an acyclic digraph \a G.
/**
 * \pre \a G is an acyclic directed graph.
 *
 * @param G   is the input graph.
 * @param num is assigned the topological numbering (0, 1, ...).
 */
OGDF_EXPORT void topologicalNumbering(const Graph & G, NodeArray<int> & num);


//! Computes the strongly connected components of the digraph \a G.
/**
 * The function implements the algorithm by Tarjan.
 *
 * @param G         is the input graph.
 * @param component is assigned a mapping from nodes to component numbers (0, 1, ...).
 * @return the number of strongly connected components.
 */
OGDF_EXPORT int strongComponents(const Graph & G, NodeArray<int> & component);



//---------------------------------------------------------
// Methods for trees and forests
//---------------------------------------------------------

//! Returns true iff \a G is a free forest, i.e. contains no undirected cycle.
/**
 * @param G is the input graph.
 * @return true if \ G is contains no undirected cycle, false otherwise.
 */
OGDF_EXPORT bool isFreeForest(const Graph & G);


//! Returns true iff \a G represents a forest, i.e., a collection of rooted trees.
/**
 * @param G     is the input graph.
 * @param roots is assigned the list of root nodes of the trees in the forest.
 * @return true if \a G represents a forest, false otherwise.
 */
OGDF_EXPORT bool isForest(const Graph & G, List<node> & roots);


//! Returns true iff \a G represents a forest, i.e. a collection of rooted trees.
/**
 * @param G is the input graph.
 * @return true if \a G represents a forest, false otherwise.
 */
inline bool isForest(const Graph & G)
{
    List<node> roots;
    return isForest(G, roots);
}


//! Returns true iff \a G represents a tree
/**
 * @param G    is the input graph.
 * @param root is assigned the root node (if true is returned).
 * @return true if \a G represents a tree, false otherwise.
 */
OGDF_EXPORT bool isTree(const Graph & G, node & root);


//! Returns true iff \a G represents a tree
/**
 * @param G    is the input graph.
 * @return true if \a G represents a tree, false otherwise.
 */
inline bool isTree(const Graph & G)
{
    node root;
    return isTree(G, root);
}


} // end namespace ogdf

#endif
