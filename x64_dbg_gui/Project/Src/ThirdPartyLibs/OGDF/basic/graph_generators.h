/*
 * $Revision: 2583 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-12 01:02:21 +0200 (Do, 12. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of graph generators.
 *
 * \author Carsten Gutwenger, Markus Chimani
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

#ifndef OGDF_GRAPH_GENERATORS_H
#define OGDF_GRAPH_GENERATORS_H


#include <ogdf/basic/Graph.h>
#include <ogdf/cluster/ClusterGraph.h>

namespace ogdf
{

//! Creates a random graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 * @param m is the number of edges of the generated graph.
 */
OGDF_EXPORT void randomGraph(Graph & G, int n, int m);

//! Creates a random simple graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 * @param m is the number of edges of the generated graph.
 */
OGDF_EXPORT bool randomSimpleGraph(Graph & G, int n, int m);

//! Creates a random biconnected graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 * @param m is the number of edges of the generated graph.
 */
OGDF_EXPORT void randomBiconnectedGraph(Graph & G, int n, int m);

//! Creates a connected (simple) planar (embedded) graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 * @param m is the number of edges of the generated graph.
 */
OGDF_EXPORT void planarConnectedGraph(Graph & G, int n, int m);

//! Creates a planar biconnected (embedded) graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 * @param m is the number of edges of the generated graph.
 * @param multiEdges determines if the generated graph may contain
 *        multi-edges.
 */
OGDF_EXPORT void planarBiconnectedGraph(Graph & G, int n, int m, bool multiEdges = false);

//! Creates a planar graph, that is connected, but not biconnected.
/*   @param n is the max. number of nodes in each biconencted component
 *   @param m is the max. number of edges in each biconnected component
 *   @param b is the number of biconnected components
 */
OGDF_EXPORT void planarCNBGraph(Graph & G, int n, int m, int b);

//! Creates a random triconnected (and simple) graph.
/**
 * The graph generator proceeds as follows. It starts with a \f$K_4\f$ and performs
 * then \a n-4 split node operations on randomly selected nodes of the graph
 * constructed so far. Each such operation splits a node \a v into two nodes
 * \a x and \a y and distributes \a v's neighbors to the two nodes such that each
 * node gets at least two neighbors. Additionally, the edge (\a x,\a y) is inserted.
 *
 * The neighbors are distributed such that a neighbor of \a v becomes
 *   - only a neighbor of \a x with probability \a p1;
 *   - only a neighbor of \a y with probability \a p1;
 *   - a neighbor of both \a x and \a y with probability 1.0 - \a p1 - \a p2.
 *
 * @param G is assigned the generated graph.
 * @param n is the number of nodes in the generated graph.
 * @param p1 is the probability that an edge is moved only to the left
 *        node after splitting a node.
 * @param p2 is the probability that an edge is moved only to the right
 *        node after splitting a node.
 *
 * The probability for a neighbor to be moved to both split nodes is
 * 1.0 - \a p1 - \a p2. The higher this probability, the higher the density
 * of the resulting graph.
 *
 * \pre The probabilities \a p1 and \a p2 must lie between 0.0 and 1.0, and
 *      \a p1 + \a p2 \f$\leq\f$ 1.0.
 */
OGDF_EXPORT void randomTriconnectedGraph(Graph & G, int n, double p1, double p2);

//! Creates a planar triconnected (and simple) graph.
/**
 * This graph generator works in two steps.
 *   -# A planar triconnected 3-regular graph is constructed using successive
 *      splitting of pairs of nodes. The constructed graph has \a n nodes and
 *      1.5\a n edges.
 *   -# The remaining edges are inserted by successive splitting of faces
 *      with degree four or greater.
 * The resulting graph also represents a combinatorial embedding.
 *
 * @param G is assigned the generated graph.
 * @param n is the number of nodes in the generated graph.
 * @param m is the number of edges in the generated graph.
 *
 * \pre
 *   - \a n \f$\geq\f$ 4 and \a n must be even; otherwise, \a n is adjusted
 *     to the next feasible integer.
 *   - 1.5\a n \f$\leq\f$ \a m \f$\leq\f$ 3\a n-6; otherwise, \a m is adjusted
 *     to a feasible value.
 */
OGDF_EXPORT void planarTriconnectedGraph(Graph & G, int n, int m);

//! Creates a planar triconnected (and simple) graph.
/**
 * This graph generator creates a planar triconnected graph by successive
 * node splitting. It starts with the \f$K_4\f$ and performs \a n-4 node
 * splits. Each such split operation distributes a node's neighbors to the
 * two nodes resulting from the split. Aftewards, two further edges can be
 * added; the probability for adding these edges is given by \a p1 and \a p2.
 * The higher these probabilities, the denser the resulting graph. Note that
 * a simple planar triconnected graph has between 1.5\a n and 3\a n-6 edges.
 *
 * \pre 0.0 \f$\le\f$ \a p1, \a p2 \f$\le\f$ 1.0.
 *
 * @param G is assigned the generated graph.
 * @param n is the number of nodes in the generated graph.
 * @param p1 is the probability for the first additional edge to be added.
 * @param p2 is the probability for the second additional edge to be added.
 */
OGDF_EXPORT void planarTriconnectedGraph(Graph & G, int n, double p1, double p2);

//! Creates a random tree (simpler version.
/**
 * @param G is assigned the tree.
 * @param n is the number of nodes of the tree.
 */
OGDF_EXPORT void randomTree(Graph & G, int n);

//! Creates a random tree.
/**
 * @param G is assigned the tree.
 * @param n is the number of nodes of the tree.
 * @param maxDeg is the maximal allowed node degree; 0 means no restriction.
 * @param maxWidth is the maximal allowed width of a level; 0 means no restriction.
 */
OGDF_EXPORT void randomTree(Graph & G, int n, int maxDeg, int maxWidth);

//! Creates a regular tree.
/**
 * @param G is assigned the tree.
 * @param n is the number of nodes of the tree.
 * @param children is the number of children per node. root has index 0, the next level has
 * indizes 1...children, the children of node 1 have indizes children+1...2*children, etc.
 * if number of nodes does not allow a regular node, the "last" node will have fewer children.
 */
void regularTree(Graph & G, int n, int children);



//! Creates a random hierarchical graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes.
 * @param m is the number of edges.
 * @param planar determines if the resulting graph is (level-)planar.
 * @param singleSource determines if the graph is a single-source graph.
 * @param longEdges determines if the graph has long edges (spanning 2 layers
 *        or more); otherwise the graph is proper.
 */
OGDF_EXPORT void randomHierarchy(
    Graph & G,
    int n,
    int m,
    bool planar,
    bool singleSource,
    bool longEdges);

//! Assigns random clusters to a given graph \a G.
/**
 * This function is called with a graph \a G and creates randomly clusters.
 * The resulting cluster graph is always c-connected and,
 * if G is planar, also c-planar.
 * @param G is the input graph.
 * @param C is a cluster graph for \a G.
 * @param cNum is the maximal number of Clusters introduced.
 * \pre \a G is connected and not empty and \a C is initialized with \a G.
 */
OGDF_EXPORT void randomClusterPlanarGraph(ClusterGraph & C, Graph & G, int cNum);

//! Assigns random clusters to a given graph \a G.
/**
 * This function is called with a graph \a G and creates randomly clusters.
 * @param G is the input graph.
 * @param C is a cluster graph for \a G.
 * @param cNum is the maximal number of clusters introduced.
 * \pre \a G is connected and not empty and \a C is initialized with \a G.
 */
OGDF_EXPORT void randomClusterGraph(ClusterGraph & C, Graph & G, int cNum);


//! Assigns a specified cluster-structure to a given graph \a G, and assigns vertices to clusters.
/**
 * This function is called with a graph \a G and the root of a second graph, resembling a tree,
 * that gives the cluster structure. Then, the vertices of G are randomly assigned to the clusters,
 * where we can guarantee that any leaf-cluster has (on average) <i>moreInLeaves</i>-times more vertices
 * than a non-leaf cluster. (E.g. if \a moreInLeaves = 5, any leaf will contain roughly 5 times more vertices than
 * an inner cluster)
 * @param C is a cluster graph for \a G, to be assigned the solution.
 * @param G is the input graph.
 * @param root is a node in some other graph (say \a T). \a T is a tree that we will consider rooted at \a root.
 *        \a T is the pattern for the cluster hierarchy.
 * @param moreInLeaves is a factor such that leaf-clusters have on average <i>moreInLeaves</i>-times more
 *        vertices than inner clusters
 * \pre \a G contains at least twice as many nodes as \a T has leaves.
 */
OGDF_EXPORT void randomClusterGraph(ClusterGraph & C, const Graph & G, const node root, int moreInLeaves);


//! Creates the complete graph \f$K_n\f$.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the generated graph.
 */
OGDF_EXPORT void completeGraph(Graph & G, int n);

//! Creates t complete bipartite graph \f$K_{n,m}\f$.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes of the first partition set.
 * @param m is the number of nodes of the second partition set.
 */
OGDF_EXPORT void completeBipartiteGraph(Graph & G, int n, int m);

//! Creates the graph \f$W_n^{(d)}\f$: A wheel graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes on the rim of the wheel (W_n).
 */
OGDF_EXPORT void wheelGraph(Graph & G, int n);

//! Creates the graph \f$Q^n\f$: A <i>n</i>-cube graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of the cube's dimensions (n>=0).
 */
OGDF_EXPORT void cubeGraph(Graph & G, int n);

//! Modifies \a G by adding its <i>n</i>-th suspension.
/**
 * @param G is the graph to extend.
 * @param s is the suspension.
 */
OGDF_EXPORT void suspension(Graph & G, int s);

//! Creates a (toroidal) grid graph on \a n x \a m nodes.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes on first axis.
 * @param m is the number of nodes on second axis.
 * @param loopN if the grid is cyclic on first axis
 * @param loopM if the grid is cyclic on second axis
 */
OGDF_EXPORT void gridGraph(Graph & G, int n, int m, bool loopN, bool loopM);

//! Creates a generalized Petersen graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes on outer cycle.
 * @param m is the number of jumps.
 */
OGDF_EXPORT void petersenGraph(Graph & G, int n, int m);

//! Creates a random (simple) directed graph.
/**
 * @param G is assigned the generated graph.
 * @param n is the number of nodes in the generated graph.
 * @param p is the probability that an edge is created (for each node pair)
 */
OGDF_EXPORT void randomDiGraph(Graph & G, int n, double p);



}


#endif
