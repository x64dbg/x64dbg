/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Computes an embedding of a biconnected graph with maximum
 * external face.
 *
 * \author Thorsten Kerkhof
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

#ifndef OGDF_EMBEDDER_MAX_FACE_BICONNECTED_GRAPHS_Layers_H
#define OGDF_EMBEDDER_MAX_FACE_BICONNECTED_GRAPHS_Layers_H

#include <ogdf/decomposition/StaticSPQRTree.h>
#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/graphalg/ShortestPathWithBFM.h>


namespace ogdf
{

//! Computes an embedding of a biconnected graph with maximum external face (plus layers approach).
/**
 * See the paper "Graph Embedding with Minimum Depth and
 * Maximum External Face" by C. Gutwenger and P. Mutzel (2004) for
 * details.
 * The algorithm for maximum external face is combined with the
 * algorithm for maximum external layers which defines how to embed
 * blocks into inner faces. See diploma thesis "Algorithmen zur
 * Bestimmung von guten Graph-Einbettungen f&uuml;r orthogonale
 * Zeichnungen" (in german) by Thorsten Kerkhof (2007) for details.
 */
template<class T>
class EmbedderMaxFaceBiconnectedGraphsLayers
{
public:
    //! Creates an embedder.
    EmbedderMaxFaceBiconnectedGraphsLayers() { }

    /**
     * \brief Embeds \a G by computing and extending a maximum face in \a G
     *   containg \a n.
     * \param G is the original graph.
     * \param adjExternal is assigned an adjacency entry of the external face.
     * \param nodeLength stores for each vertex in \a G its length.
     * \param edgeLength stores for each edge in \a G its length.
     * \param n is a node of the original graph. If n is given, a maximum face
     *   containing n is computed, otherwise any maximum face.
     */
    static void embed(
        Graph & G,
        adjEntry & adjExternal,
        const NodeArray<T> & nodeLength,
        const EdgeArray<T> & edgeLength,
        const node & n = 0);

    /**
     * \brief Computes the component lengths of all virtual edges in spqrTree.
     * \param G is the original graph.
     * \param nodeLength is saving for each vertex in \a G its length.
     * \param edgeLength is saving for each edge in \a G its length.
     * \param spqrTree is the SPQR-tree of \a G.
     * \param edgeLengthSkel is saving for each skeleton graph of the SPQR-tree
     *   all edge lengths.
     */
    static void compute(
        const Graph & G,
        const NodeArray<T> & nodeLength,
        const EdgeArray<T> & edgeLength,
        StaticSPQRTree & spqrTree,
        NodeArray< EdgeArray<T> > & edgeLengthSkel);

    /**
     * \brief Returns the size of a maximum external face in \a G containing the node \a n.
     * \param G is the original graph.
     * \param n is a node of the original graph.
     * \param nodeLength is saving for each vertex in \a G its length.
     * \param edgeLength is saving for each edge in \a G its length.
     * \return The size of a maximum external face in \a G containing the node \a n.
     */
    static T computeSize(
        const Graph & G,
        const node & n,
        const NodeArray<T> & nodeLength,
        const EdgeArray<T> & edgeLength);

    /**
     * \brief Returns the size of a maximum external face in \a G containing
     *   the node \a n.
     *
     * \param G is the original graph.
     * \param n is a node of the original graph.
     * \param nodeLength is saving for each vertex in \a G its length.
     * \param edgeLength is saving for each edge in \a G its length.
     * \param spqrTree is the SPQR-tree of G.
     * \return The size of a maximum external face in \a G containing the node \a n.
     */
    static T computeSize(
        const Graph & G,
        const node & n,
        const NodeArray<T> & nodeLength,
        const EdgeArray<T> & edgeLength,
        StaticSPQRTree & spqrTree);

    /**
     * \brief Returns the size of a maximum external face in \a G containing
     *   the node \a n.
     *
     * \param G is the original graph.
     * \param n is a node of the original graph.
     * \param nodeLength is saving for each vertex in \a G its length.
     * \param edgeLength is saving for each edge in \a G its length.
     * \param spqrTree is the SPQR-tree of G.
     * \param edgeLengthSkel is saving for each skeleton graph the length
     *   of each edge.
     * \return The size of a maximum external face in \a G containing the node \a n.
     */
    static T computeSize(
        const Graph & G,
        const node & n,
        const NodeArray<T> & nodeLength,
        const EdgeArray<T> & edgeLength,
        StaticSPQRTree & spqrTree,
        const NodeArray< EdgeArray<T> > & edgeLengthSkel);

    /**
     * \brief Returns the size of a maximum external face in \a G.
     * \param G is the original graph.
     * \param nodeLength is saving for each vertex in \a G its length.
     * \param edgeLength is saving for each edge in \a G its length.
     * \return The size of a maximum external face in \a G.
     */
    static T computeSize(
        const Graph & G,
        const NodeArray<T> & nodeLength,
        const EdgeArray<T> & edgeLength);

    /**
     * \brief Returns the size of a maximum external face in \a G.
     *   The SPQR-tree is given. The computed component lengths are
     *   computed and returned.
     *
     * \param G is the original graph.
     * \param nodeLength is saving for each vertex in \a G its length.
     * \param edgeLength is saving for each edge in \a G its length.
     * \param spqrTree is the SPQR-tree of G.
     * \param edgeLengthSkel is saving for each skeleton graph the length
     *   of each edge.
     * \return The size of a maximum external face in \a G.
     */
    static T computeSize(
        const Graph & G,
        const NodeArray<T> & nodeLength,
        const EdgeArray<T> & edgeLength,
        StaticSPQRTree & spqrTree,
        NodeArray< EdgeArray<T> > & edgeLengthSkel);

private:
    /**
     * \brief Bottom up traversal of SPQR-tree computing the component length of
     *   all non-reference edges.
     * \param spqrTree is the SPQR-tree of \a G.
     * \param mu is the SPQR-tree node treated in this function call.
     * \param nodeLength is saving for each node of the original graph \a G its
     *   length.
     * \param edgeLength is saving for each skeleton graph the length of each
     *   edge.
     */
    static void bottomUpTraversal(
        StaticSPQRTree & spqrTree,
        const node & mu,
        const NodeArray<T> & nodeLength,
        NodeArray< EdgeArray<T> > & edgeLength);

    /**
     * \brief Top down traversal of SPQR-tree computing the component length of
     *   all reference edges.
     * \param spqrTree is the SPQR-tree of \a G.
     * \param mu is the SPQR-tree node treated in this function call.
     * \param nodeLength is saving for each node of the original graph \a G its
     *   length.
     * \param edgeLength is saving for each skeleton graph the length of each
     *   edge.
     */
    static void topDownTraversal(
        StaticSPQRTree & spqrTree,
        const node & mu,
        const NodeArray<T> & nodeLength,
        NodeArray< EdgeArray<T> > & edgeLength);

    /**
     * \brief Computes the size of a maximum face in the skeleton graph of \a mu
     *   containing \a n.
     * \param spqrTree is the SPQR-tree of \a G.
     * \param mu is the SPQR-tree node treated in this function call.
     * \param n is a node of the original graph \a G.
     * \param nodeLength is saving for each node of the original graph \a G its
     *   length.
     * \param edgeLength is saving for each skeleton graph the length of each
     *   edge.
     */
    static T largestFaceContainingNode(
        const StaticSPQRTree & spqrTree,
        const node & mu,
        const node & n,
        const NodeArray<T> & nodeLength,
        const NodeArray< EdgeArray<T> > & edgeLength);

    /**
     * \brief Computes the size of a maximum face in the skeleton graph of \a mu.
     * \param spqrTree is the SPQR-tree of \a G.
     * \param mu is the SPQR-tree node treated in this function call.
     * \param nodeLength is saving for each node of the original graph \a G its
     *   length.
     * \param edgeLength is saving for each skeleton graph the length of each
     *   edge.
     */
    static T largestFaceInSkeleton(
        const StaticSPQRTree & spqrTree,
        const node & mu,
        const NodeArray<T> & nodeLength,
        const NodeArray< EdgeArray<T> > & edgeLength);

    /* \brief Computes recursively the thickness of the skeleton graph of all
     *   nodes in the SPQR-tree.
     *
     * \param spqrTree The SPQR-tree of the treated graph.
     * \param mu a node in the SPQR-tree.
     * \param thickness saving the computed results - the thickness of each
     *   skeleton graph.
     * \param nodeLength is saving for each node of the original graph \a G its
     *   length.
     * \param edgeLength is saving the edge lengths of all edges in each skeleton
     *   graph of all tree nodes.
     */
    static void bottomUpThickness(
        const StaticSPQRTree & spqrTree,
        const node & mu,
        NodeArray<T> & thickness,
        const NodeArray<T> & nodeLength,
        const NodeArray< EdgeArray<T> > & edgeLength);

    /* \brief ExpandEdge embeds all edges in the skeleton graph \a S into an
     *   existing embedding and calls recursively itself for all virtual edges
     *   in S.
     *
     * \param spqrTree The SPQR-tree of the treated graph.
     * \param treeNodeTreated is an array saving for each SPQR-tree node \a mu
     *   whether it was already treated by any call of ExpandEdge or not. Every
     *   \a mu should only be treated once.
     * \param mu is a node in the SPQR-tree.
     * \param leftNode is the node adjacent to referenceEdge, which should be "left"
     *   in the embedding
     * \param nodeLength is an array saving the lengths of the nodes of \a G.
     * \param edgeLength is saving the edge lengths of all edges in each skeleton
     *   graph of all tree nodes.
     * \param thickness of each skeleton graph.
     * \param newOrder is saving for each node \a n in \a G the new adjacency
     *   list. This is an output parameter.
     * \param adjBeforeNodeArraySource is saving for the source of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
     * \param adjBeforeNodeArrayTarget is saving for the target of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
     * \param delta_u the distance from the second adjacent face of the reference edge
     *   except the external face to the external face of G.
     * \param delta_d the distance from the external face to the external face of G.
     * \param adjExternal is an adjacency entry in the external face.
     * \param n is only set, if ExpandEdge is called for the first time, because
     *   then there is no virtual edge which has to be expanded, but the max face
     *   has to contain a certain node \a n.
     */
    static void expandEdge(
        const StaticSPQRTree & spqrTree,
        NodeArray<bool> & treeNodeTreated,
        const node & mu,
        const node & leftNode,
        const NodeArray<T> & nodeLength,
        const NodeArray< EdgeArray<T> > & edgeLength,
        const NodeArray<T> & thickness,
        NodeArray< List<adjEntry> > & newOrder,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
        const T & delta_u,
        const T & delta_d,
        adjEntry & adjExternal,
        const node & n = 0);

    /* \brief Embeds all edges in the skeleton graph \a S of an S-node of the
     *   SPQR-tree into an existing embedding and calls recursively itself for
     *   all virtual edges in S.
     *
     * \param spqrTree The SPQR-tree of the treated graph.
     * \param treeNodeTreated is an array saving for each SPQR-tree node \a mu
     *   whether it was already treated by any call of ExpandEdge or not. Every
     *   \a mu should only be treated once.
     * \param mu is a node in the SPQR-tree.
     * \param leftNode is the node adjacent to referenceEdge, which should be "left"
     *   in the embedding
     * \param nodeLength is an array saving the lengths of the nodes of \a G.
     * \param edgeLength is saving the edge lengths of all edges in each skeleton
     *   graph of all tree nodes.
     * \param thickness of each skeleton graph.
     * \param newOrder is saving for each node \a n in \a G the new adjacency
     *   list. This is an output parameter.
     * \param adjBeforeNodeArraySource is saving for the source of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
     * \param adjBeforeNodeArrayTarget is saving for the target of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
     * \param delta_u the distance from the second adjacent face of the reference edge
     *   except the external face to the external face of G.
     * \param delta_d the distance from the external face to the external face of G.
     * \param adjExternal is an adjacency entry in the external face.
     */
    static void expandEdgeSNode(
        const StaticSPQRTree & spqrTree,
        NodeArray<bool> & treeNodeTreated,
        const node & mu,
        const node & leftNode,
        const NodeArray<T> & nodeLength,
        const NodeArray< EdgeArray<T> > & edgeLength,
        const NodeArray<T> & thickness,
        NodeArray< List<adjEntry> > & newOrder,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
        const T & delta_u,
        const T & delta_d,
        adjEntry & adjExternal);

    /* \brief Embeds all edges in the skeleton graph \a S of an P-node of the
     *   SPQR-tree into an existing embedding and calls recursively itself for
     *   all virtual edges in S.
     *
     * \param spqrTree The SPQR-tree of the treated graph.
     * \param treeNodeTreated is an array saving for each SPQR-tree node \a mu
     *   whether it was already treated by any call of ExpandEdge or not. Every
     *   \a mu should only be treated once.
     * \param mu is a node in the SPQR-tree.
     * \param leftNode is the node adjacent to referenceEdge, which should be "left"
     *   in the embedding
     * \param nodeLength is an array saving the lengths of the nodes of \a G.
     * \param edgeLength is saving the edge lengths of all edges in each skeleton
     *   graph of all tree nodes.
     * \param thickness of each skeleton graph.
     * \param newOrder is saving for each node \a n in \a G the new adjacency
     *   list. This is an output parameter.
     * \param adjBeforeNodeArraySource is saving for the source of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
     * \param adjBeforeNodeArrayTarget is saving for the target of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
     * \param delta_u the distance from the second adjacent face of the reference edge
     *   except the external face to the external face of G.
     * \param delta_d the distance from the external face to the external face of G.
     * \param adjExternal is an adjacency entry in the external face.
     */
    static void expandEdgePNode(
        const StaticSPQRTree & spqrTree,
        NodeArray<bool> & treeNodeTreated,
        const node & mu,
        const node & leftNode,
        const NodeArray<T> & nodeLength,
        const NodeArray< EdgeArray<T> > & edgeLength,
        const NodeArray<T> & thickness,
        NodeArray< List<adjEntry> > & newOrder,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
        const T & delta_u,
        const T & delta_d,
        adjEntry & adjExternal);

    /* \brief Embeds all edges in the skeleton graph \a S of an R-node of the
     *   SPQR-tree into an existing embedding and calls recursively itself for
     *   all virtual edges in S.
     *
     * \param spqrTree The SPQR-tree of the treated graph.
     * \param treeNodeTreated is an array saving for each SPQR-tree node \a mu
     *   whether it was already treated by any call of ExpandEdge or not. Every
     *   \a mu should only be treated once.
     * \param mu is a node in the SPQR-tree.
     * \param leftNode is the node adjacent to referenceEdge, which should be "left"
     *   in the embedding
     * \param nodeLength is an array saving the lengths of the nodes of \a G.
     * \param edgeLength is saving the edge lengths of all edges in each skeleton
     *   graph of all tree nodes.
     * \param thickness of each skeleton graph.
     * \param newOrder is saving for each node \a n in \a G the new adjacency
     *   list. This is an output parameter.
     * \param adjBeforeNodeArraySource is saving for the source of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
     * \param adjBeforeNodeArrayTarget is saving for the target of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
     * \param delta_u the distance from the second adjacent face of the reference edge
     *   except the external face to the external face of G.
     * \param delta_d the distance from the external face to the external face of G.
     * \param adjExternal is an adjacency entry in the external face.
     * \param n is only set, if ExpandEdge is called for the first time, because
     *   then there is no virtual edge which has to be expanded, but the max face
     *   has to contain a certain node \a n.
     */
    static void expandEdgeRNode(
        const StaticSPQRTree & spqrTree,
        NodeArray<bool> & treeNodeTreated,
        const node & mu,
        const node & leftNode,
        const NodeArray<T> & nodeLength,
        const NodeArray< EdgeArray<T> > & edgeLength,
        const NodeArray<T> & thickness,
        NodeArray< List<adjEntry> > & newOrder,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
        const T & delta_u,
        const T & delta_d,
        adjEntry & adjExternal,
        const node & n = 0);

    /* \brief Writes a given adjacency entry into the newOrder. If the edge
     *   belonging to ae is a virtual edge, it is expanded.
     *
     * \param ae is the adjacency entry which has to be expanded.
     * \param before is the adjacency entry of the node in \a G, before
     *   which ae has to be inserted.
     * \param spqrTree The SPQR-tree of the treated graph.
     * \param treeNodeTreated is an array saving for each SPQR-tree node \a mu
     *   whether it was already treated by any call of ExpandEdge or not. Every
     *   \a mu should only be treated once.
     * \param mu is a node in the SPQR-tree.
     * \param leftNode is the node adjacent to referenceEdge, which should be "left"
     *   in the embedding
     * \param nodeLength is an array saving the lengths of the nodes of \a G.
     * \param edgeLength is saving the edge lengths of all edges in each skeleton
     *   graph of all tree nodes.
     * \param thickness of each skeleton graph.
     * \param newOrder is saving for each node \a n in \a G the new adjacency
     *   list. This is an output parameter.
     * \param adjBeforeNodeArraySource is saving for the source of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
     * \param adjBeforeNodeArrayTarget is saving for the target of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
     * \param delta_u the distance from the second adjacent face of the reference edge
     *   except the external face to the external face of G.
     * \param delta_d the distance from the external face to the external face of G.
     * \param adjExternal is an adjacency entry in the external face.
     */
    static void adjEntryForNode(
        adjEntry & ae,
        ListIterator<adjEntry> & before,
        const StaticSPQRTree & spqrTree,
        NodeArray<bool> & treeNodeTreated,
        const node & mu,
        const node & leftNode,
        const NodeArray<T> & nodeLength,
        const NodeArray< EdgeArray<T> > & edgeLength,
        const NodeArray<T> & thickness,
        NodeArray< List<adjEntry> > & newOrder,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
        const T & delta_u,
        const T & delta_d,
        adjEntry & adjExternal);

    /* \brief Single source shortest path.
     *
     * \param G directed graph
     * \param s source node
     * \param length length of an edge
     * \param d contains shortest path distances after call
     */
    static bool sssp(
        const Graph & G,
        const node & s,
        const EdgeArray<T> & length,
        NodeArray<T> & d);
};


template<class T>
void EmbedderMaxFaceBiconnectedGraphsLayers<T>::embed(
    Graph & G,
    adjEntry & adjExternal,
    const NodeArray<T> & nodeLength,
    const EdgeArray<T> & edgeLength,
    const node & n /* = 0*/)
{
    //Base cases (SPQR-Tree-implementatioin would crash with these inputs):
    OGDF_ASSERT(G.numberOfNodes() >= 2)
    if(G.numberOfEdges() <= 2)
    {
        edge e = G.firstEdge();
        adjExternal = e->adjSource();
        return;
    }

    //****************************************************************************
    //First step: calculate maximum face and edge lengths for virtual edges
    //****************************************************************************
    StaticSPQRTree spqrTree(G);
    NodeArray< EdgeArray<T> > edgeLengthSkel;
    compute(G, nodeLength, edgeLength, spqrTree, edgeLengthSkel);

    //****************************************************************************
    //Second step: Embed G
    //****************************************************************************
    T biggestFace = -1;
    node bigFaceMu;
    if(n == 0)
    {
        node mu;
        forall_nodes(mu, spqrTree.tree())
        {
            //Expand all faces in skeleton(mu) and get size of the largest of them:
            T sizeMu = largestFaceInSkeleton(spqrTree, mu, nodeLength, edgeLengthSkel);
            if(sizeMu > biggestFace)
            {
                biggestFace = sizeMu;
                bigFaceMu = mu;
            }
        }
    }
    else
    {
        edge nAdjEdge;
        node* mus = new node[n->degree()];
        int i = 0;
        forall_adj_edges(nAdjEdge, n)
        {
            mus[i] = spqrTree.skeletonOfReal(nAdjEdge).treeNode();
            bool alreadySeenMu = false;
            for(int j = 0; j < i && !alreadySeenMu; j++)
            {
                if(mus[i] == mus[j])
                    alreadySeenMu = true;
            }
            if(alreadySeenMu)
            {
                i++;
                continue;
            }
            else
            {
                //Expand all faces in skeleton(mu) containing n and get size of
                //the largest of them:
                T sizeInMu = largestFaceContainingNode(spqrTree, mus[i], n, nodeLength, edgeLengthSkel);
                if(sizeInMu > biggestFace)
                {
                    biggestFace = sizeInMu;
                    bigFaceMu = mus[i];
                }

                i++;
            }
        }
        delete mus;
    }

    bigFaceMu = spqrTree.rootTreeAt(bigFaceMu);

    NodeArray<T> thickness(spqrTree.tree());
    bottomUpThickness(spqrTree, bigFaceMu, thickness, nodeLength, edgeLengthSkel);

    NodeArray< List<adjEntry> > newOrder(G);
    NodeArray<bool> treeNodeTreated(spqrTree.tree(), false);
    ListIterator<adjEntry> it;
    adjExternal = 0;
    NodeArray< ListIterator<adjEntry> > adjBeforeNodeArraySource(spqrTree.tree());
    NodeArray< ListIterator<adjEntry> > adjBeforeNodeArrayTarget(spqrTree.tree());
    expandEdge(spqrTree, treeNodeTreated, bigFaceMu, 0, nodeLength,
               edgeLengthSkel, thickness, newOrder, adjBeforeNodeArraySource,
               adjBeforeNodeArrayTarget, 0, 0, adjExternal, n);

    node v;
    forall_nodes(v, G)
    G.sort(v, newOrder[v]);
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphsLayers<T>::adjEntryForNode(
    adjEntry & ae,
    ListIterator<adjEntry> & before,
    const StaticSPQRTree & spqrTree,
    NodeArray<bool> & treeNodeTreated,
    const node & mu,
    const node & leftNode,
    const NodeArray<T> & nodeLength,
    const NodeArray< EdgeArray<T> > & edgeLength,
    const NodeArray<T> & thickness,
    NodeArray< List<adjEntry> > & newOrder,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
    const T & delta_u,
    const T & delta_d,
    adjEntry & adjExternal)
{
    Skeleton & S = spqrTree.skeleton(mu);
    edge referenceEdge = S.referenceEdge();
    if(S.isVirtual(ae->theEdge()))
    {
        edge twinE = S.twinEdge(ae->theEdge());
        node twinNT = S.twinTreeNode(ae->theEdge());
        //Skeleton& twinS = spqrTree.skeleton(twinNT);

        if(!treeNodeTreated[twinNT])
        {
            node m_leftNode;
            if(ae->theEdge()->source() == leftNode)
                m_leftNode = twinE->source();
            else
                m_leftNode = twinE->target();

            if(ae->theEdge()->source() == ae->theNode())
                adjBeforeNodeArraySource[twinNT] = before;
            else
                adjBeforeNodeArrayTarget[twinNT] = before;

            //recursion call:
            expandEdge(spqrTree, treeNodeTreated, twinNT, m_leftNode,
                       nodeLength, edgeLength, thickness, newOrder,
                       adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                       delta_u, delta_d, adjExternal);
        } //if (!treeNodeTreated[twinNT])

        if(ae->theEdge() == referenceEdge)
        {
            if(ae->theNode() == ae->theEdge()->source())
            {
                ListIterator<adjEntry> tmpBefore = adjBeforeNodeArraySource[mu];
                adjBeforeNodeArraySource[mu] = before;
                before = tmpBefore;
            }
            else
            {
                ListIterator<adjEntry> tmpBefore = adjBeforeNodeArrayTarget[mu];
                adjBeforeNodeArrayTarget[mu] = before;
                before = tmpBefore;
            }
        }
        else //!(ae->theEdge() == referenceEdge)
        {
            if(ae->theNode() == ae->theEdge()->source())
                before = adjBeforeNodeArraySource[twinNT];
            else
                before = adjBeforeNodeArrayTarget[twinNT];
        }
    }
    else //!(S.isVirtual(ae->theEdge()))
    {
        node origNode = S.original(ae->theNode());
        edge origEdge = S.realEdge(ae->theEdge());

        if(origNode == origEdge->source())
        {
            if(!before.valid())
                before = newOrder[origNode].pushBack(origEdge->adjSource());
            else
                before = newOrder[origNode].insertBefore(origEdge->adjSource(), before);
        }
        else
        {
            if(!before.valid())
                before = newOrder[origNode].pushBack(origEdge->adjTarget());
            else
                before = newOrder[origNode].insertBefore(origEdge->adjTarget(), before);
        }
    } //else //!(S.isVirtual(ae->theEdge()))
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphsLayers<T>::expandEdge(
    const StaticSPQRTree & spqrTree,
    NodeArray<bool> & treeNodeTreated,
    const node & mu,
    const node & leftNode,
    const NodeArray<T> & nodeLength,
    const NodeArray< EdgeArray<T> > & edgeLength,
    const NodeArray<T> & thickness,
    NodeArray< List<adjEntry> > & newOrder,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
    const T & delta_u,
    const T & delta_d,
    adjEntry & adjExternal,
    const node & n /*= 0*/)
{
    treeNodeTreated[mu] = true;

    switch(spqrTree.typeOf(mu))
    {
    case SPQRTree::SNode:
        expandEdgeSNode(spqrTree, treeNodeTreated, mu, leftNode,
                        nodeLength, edgeLength, thickness, newOrder, adjBeforeNodeArraySource,
                        adjBeforeNodeArrayTarget, delta_u, delta_d, adjExternal);
        break;
    case SPQRTree::PNode:
        expandEdgePNode(spqrTree, treeNodeTreated, mu, leftNode,
                        nodeLength, edgeLength, thickness, newOrder, adjBeforeNodeArraySource,
                        adjBeforeNodeArrayTarget, delta_u, delta_d, adjExternal);
        break;
    case SPQRTree::RNode:
        expandEdgeRNode(spqrTree, treeNodeTreated, mu, leftNode,
                        nodeLength, edgeLength, thickness, newOrder, adjBeforeNodeArraySource,
                        adjBeforeNodeArrayTarget, delta_u, delta_d, adjExternal, n);
        break;
        OGDF_NODEFAULT
    }
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphsLayers<T>::expandEdgeSNode(
    const StaticSPQRTree & spqrTree,
    NodeArray<bool> & treeNodeTreated,
    const node & mu,
    const node & leftNode,
    const NodeArray<T> & nodeLength,
    const NodeArray< EdgeArray<T> > & edgeLength,
    const NodeArray<T> & thickness,
    NodeArray< List<adjEntry> > & newOrder,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
    const T & delta_u,
    const T & delta_d,
    adjEntry & adjExternal)
{
    Skeleton & S = spqrTree.skeleton(mu);
    edge referenceEdge = S.referenceEdge();
    adjEntry startAdjEntry;
    if(leftNode == 0)
    {
        edge e;
        forall_edges(e, S.getGraph())
        {
            if(!S.isVirtual(e))
            {
                startAdjEntry = e->adjSource();
                break;
            }
        }
    }
    else if(leftNode->firstAdj()->theEdge() == referenceEdge)
        startAdjEntry = leftNode->lastAdj();
    else
        startAdjEntry = leftNode->firstAdj();

    adjEntry ae = startAdjEntry;
    if(adjExternal == 0)
    {
        edge orgEdge = S.realEdge(ae->theEdge());
        if(orgEdge->source() == S.original(ae->theNode()))
            adjExternal = orgEdge->adjSource()->twin();
        else
            adjExternal = orgEdge->adjTarget()->twin();
    }

    ListIterator<adjEntry> before;
    if(!(referenceEdge == 0) && leftNode == referenceEdge->source())
        before = adjBeforeNodeArraySource[mu];
    else if(!(referenceEdge == 0))
        before = adjBeforeNodeArrayTarget[mu];
    ListIterator<adjEntry> beforeSource;

    bool firstStep = true;
    while(firstStep || ae != startAdjEntry)
    {
        //first treat ae with ae->theNode() is left node, then treat its twin:
        node m_leftNode = ae->theNode();

        if(ae->theEdge() == referenceEdge)
        {
            if(ae->theNode() == referenceEdge->source())
                adjBeforeNodeArraySource[mu] = before;
            else
                adjBeforeNodeArrayTarget[mu] = before;
        }
        else
        {
            if(S.isVirtual(ae->theEdge()) && !(referenceEdge == 0))
            {
                node twinTN = S.twinTreeNode(ae->theEdge());
                if(ae->theEdge()->source() == ae->theNode())
                {
                    if(ae->theEdge()->target() == referenceEdge->source())
                        adjBeforeNodeArrayTarget[twinTN] = adjBeforeNodeArraySource[mu];
                    else if(ae->theEdge()->target() == referenceEdge->target())
                        adjBeforeNodeArrayTarget[twinTN] = adjBeforeNodeArrayTarget[mu];
                }
                else
                {
                    if(ae->theEdge()->source() == referenceEdge->source())
                        adjBeforeNodeArraySource[twinTN] = adjBeforeNodeArraySource[mu];
                    else if(ae->theEdge()->source() == referenceEdge->target())
                        adjBeforeNodeArraySource[twinTN] = adjBeforeNodeArrayTarget[mu];
                }
            }

            adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
                            m_leftNode, nodeLength, edgeLength, thickness, newOrder,
                            adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                            delta_u, delta_d, adjExternal);
        }

        if(firstStep)
        {
            beforeSource = before;
            firstStep = false;
        }

        ae = ae->twin();
        if(referenceEdge == 0)
            before = 0;
        else if(ae->theNode() == referenceEdge->source())
            before = adjBeforeNodeArraySource[mu];
        else if(ae->theNode() == referenceEdge->target())
            before = adjBeforeNodeArrayTarget[mu];
        else
            before = 0;
        if(ae->theEdge() == referenceEdge)
        {
            if(ae->theNode() == referenceEdge->source())
                adjBeforeNodeArraySource[mu] = beforeSource;
            else
                adjBeforeNodeArrayTarget[mu] = beforeSource;
        }
        else
            adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
                            m_leftNode, nodeLength, edgeLength, thickness, newOrder,
                            adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                            delta_u, delta_d, adjExternal);

        //set new adjacency entry pair (ae and its twin):
        if(ae->theNode()->firstAdj() == ae)
            ae = ae->theNode()->lastAdj();
        else
            ae = ae->theNode()->firstAdj();
    }
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphsLayers<T>::expandEdgePNode(
    const StaticSPQRTree & spqrTree,
    NodeArray<bool> & treeNodeTreated,
    const node & mu,
    const node & leftNode,
    const NodeArray<T> & nodeLength,
    const NodeArray< EdgeArray<T> > & edgeLength,
    const NodeArray<T> & thickness,
    NodeArray< List<adjEntry> > & newOrder,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
    const T & delta_u,
    const T & delta_d,
    adjEntry & adjExternal)
{
    //Choose face defined by virtual edge and the longest edge different from it.
    Skeleton & S = spqrTree.skeleton(mu);
    edge referenceEdge = S.referenceEdge();
    edge altReferenceEdge = 0;

    node m_leftNode = leftNode;
    if(m_leftNode == 0)
    {
        List<node> nodeList;
        S.getGraph().allNodes(nodeList);
        m_leftNode = *(nodeList.begin());
    }
    node m_rightNode = m_leftNode->firstAdj()->twinNode();

    edge e;
    if(referenceEdge == 0)
    {
        forall_edges(e, S.getGraph())
        {
            if(!S.isVirtual(e))
            {
                altReferenceEdge = e;
                edge orgEdge = S.realEdge(e);
                if(orgEdge->source() == S.original(m_leftNode))
                    adjExternal = orgEdge->adjSource();
                else
                    adjExternal = orgEdge->adjTarget();
                break;
            }
        }
    }

    //sort edges by length:
    List<edge> graphEdges;
    forall_edges(e, S.getGraph())
    {
        if(e == referenceEdge || e == altReferenceEdge)
            continue;

        if(!graphEdges.begin().valid())
            graphEdges.pushBack(e);
        else
        {
            for(ListIterator<edge> it = graphEdges.begin(); it.valid(); it++)
            {
                if(edgeLength[mu][e] > edgeLength[mu][*it])
                {
                    graphEdges.insertBefore(e, it);
                    break;
                }
                ListIterator<edge> next = it;
                next++;
                if(!next.valid())
                {
                    graphEdges.pushBack(e);
                    break;
                }
            }
        }
    }

    List<edge> rightEdgeOrder;
    ListIterator<adjEntry> beforeAltRefEdge;
    ListIterator<adjEntry> beforeLeft;

    //begin with left node and longest edge:
    for(int i = 0; i < 2; i++)
    {
        ListIterator<adjEntry> before;
        node n;
        if(i == 0)
            n = m_leftNode;
        else
        {
            n = m_rightNode;
            before = beforeAltRefEdge;
        }

        if(!(referenceEdge == 0))
        {
            if(n == referenceEdge->source())
                before = adjBeforeNodeArraySource[mu];
            else
                before = adjBeforeNodeArrayTarget[mu];
        }

        adjEntry ae;

        //all edges except reference edge:
        if(i == 0)
        {
            ListIterator<edge> lastPos;
            ListIterator<adjEntry> beforeRight;
            if(!(referenceEdge == 0))
            {
                if(referenceEdge->source() == m_rightNode)
                    beforeRight = adjBeforeNodeArraySource[mu];
                else //referenceEdge->target() == rightnode
                    beforeRight = adjBeforeNodeArrayTarget[mu];
            }
            bool insertBeforeLast = false;
            bool oneEdgeInE_a = false;
            T sum_E_a = 0;
            T sum_E_b = 0;

            for(int looper = 0; looper < graphEdges.size(); looper++)
            {
                edge e = *(graphEdges.get(looper));

                if(!lastPos.valid())
                    lastPos = rightEdgeOrder.pushBack(e);
                else if(insertBeforeLast)
                    lastPos = rightEdgeOrder.insertBefore(e, lastPos);
                else
                    lastPos = rightEdgeOrder.insertAfter(e, lastPos);

                //decide whether to choose face f_a or f_b as f_{mu, mu'}:
                if(delta_u + sum_E_a < delta_d + sum_E_b)
                {
                    ListIterator<adjEntry> beforeU = before;

                    if(e->source() == n)
                        ae = e->adjSource();
                    else
                        ae = e->adjTarget();

                    if(S.isVirtual(e))
                    {
                        node nu = S.twinTreeNode(e);

                        T delta_u_nu = delta_u + sum_E_a;
                        T delta_d_nu = delta_d + sum_E_b;

                        //buffer computed embedding:
                        NodeArray< List<adjEntry> > tmp_newOrder(spqrTree.originalGraph());
                        ListIterator<adjEntry> tmp_before;

                        adjEntryForNode(ae, tmp_before, spqrTree, treeNodeTreated, mu,
                                        m_leftNode, nodeLength, edgeLength, thickness,
                                        tmp_newOrder, adjBeforeNodeArraySource,
                                        adjBeforeNodeArrayTarget, delta_d_nu, delta_u_nu, adjExternal);

                        //copy buffered embedding reversed into newOrder:
                        node leftOrg = S.original(m_leftNode);
                        node rightOrg = S.original(m_rightNode);
                        node nOG;
                        forall_nodes(nOG, spqrTree.originalGraph())
                        {
                            List<adjEntry> nOG_tmp_adjList = tmp_newOrder[nOG];
                            if(nOG_tmp_adjList.size() == 0)
                                continue;

                            ListIterator<adjEntry>* m_before;
                            if(nOG == leftOrg)
                                m_before = &beforeU;
                            else if(nOG == rightOrg && !(referenceEdge == 0))
                                m_before = &beforeRight;
                            else
                                m_before = OGDF_NEW ListIterator<adjEntry>();

                            for(ListIterator<adjEntry> ae_it = nOG_tmp_adjList.begin(); ae_it.valid(); ae_it++)
                            {
                                adjEntry adjaEnt = *ae_it;
                                if(!m_before->valid())
                                    *m_before = newOrder[nOG].pushBack(adjaEnt);
                                else
                                    *m_before = newOrder[nOG].insertBefore(adjaEnt, *m_before);

                                if(nOG == leftOrg || nOG == rightOrg)
                                {
                                    if(S.original(e->source()) == nOG)
                                        adjBeforeNodeArraySource[nu] = *m_before;
                                    else
                                        adjBeforeNodeArrayTarget[nu] = *m_before;
                                }
                            }

                            if(nOG != leftOrg && (nOG != rightOrg || referenceEdge == 0))
                                delete m_before;
                        } //forall_nodes(nOG, spqrTree.originalGraph())

                        sum_E_a += thickness[nu];
                    }
                    else //!(S.isVirtual(e))
                    {
                        adjEntryForNode(ae, beforeU, spqrTree, treeNodeTreated, mu,
                                        m_leftNode, nodeLength, edgeLength, thickness,
                                        newOrder, adjBeforeNodeArraySource,
                                        adjBeforeNodeArrayTarget, 0, 0, adjExternal);

                        sum_E_a += 1;
                    }

                    insertBeforeLast = false;
                    if(!oneEdgeInE_a)
                    {
                        beforeLeft = beforeU;
                        oneEdgeInE_a = true;
                    }
                }
                else //!(delta_u + sum_E_a <= delta_d + sum_E_b)
                {
                    if(S.isVirtual(e))
                    {
                        node nu = S.twinTreeNode(e);
                        if(!(referenceEdge == 0))
                        {
                            if(e->source() == n)
                                adjBeforeNodeArrayTarget[nu] = beforeRight;
                            else
                                adjBeforeNodeArraySource[nu] = beforeRight;
                        }
                    }

                    T delta_u_nu = delta_u + sum_E_a;
                    T delta_d_nu = delta_d + sum_E_b;

                    if(e->source() == n)
                        ae = e->adjSource();
                    else
                        ae = e->adjTarget();

                    adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
                                    m_leftNode, nodeLength, edgeLength, thickness, newOrder,
                                    adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                                    delta_u_nu, delta_d_nu, adjExternal);

                    if(S.isVirtual(e))
                        sum_E_b += thickness[S.twinTreeNode(e)];
                    else
                        sum_E_b += 1;

                    insertBeforeLast = true;
                    if(!oneEdgeInE_a)
                        beforeLeft = before;
                }
            }
        }
        else
        {
            for(ListIterator<edge> it = rightEdgeOrder.begin(); it.valid(); it++)
            {
                if((*it)->source() == n)
                    ae = (*it)->adjSource();
                else
                    ae = (*it)->adjTarget();

                adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
                                m_leftNode, nodeLength, edgeLength, thickness, newOrder,
                                adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                                0, 0, adjExternal);
            }
        }

        //(alternative) reference edge at last:
        if(!(referenceEdge == 0))
        {
            if(i == 0)
            {
                if(n == referenceEdge->source())
                    adjBeforeNodeArraySource[mu] = beforeLeft;
                else
                    adjBeforeNodeArrayTarget[mu] = beforeLeft;
            }
            else
            {
                if(n == referenceEdge->source())
                    adjBeforeNodeArraySource[mu] = before;
                else
                    adjBeforeNodeArrayTarget[mu] = before;
            }
        }
        else
        {
            if(altReferenceEdge->source() == n)
                ae = altReferenceEdge->adjSource();
            else
                ae = altReferenceEdge->adjTarget();

            adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
                            m_leftNode, nodeLength, edgeLength, thickness, newOrder,
                            adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                            0, 0, adjExternal);
        }
    }
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphsLayers<T>::expandEdgeRNode(
    const StaticSPQRTree & spqrTree,
    NodeArray<bool> & treeNodeTreated,
    const node & mu,
    const node & leftNode,
    const NodeArray<T> & nodeLength,
    const NodeArray< EdgeArray<T> > & edgeLength,
    const NodeArray<T> & thickness,
    NodeArray< List<adjEntry> > & newOrder,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
    const T & delta_u,
    const T & delta_d,
    adjEntry & adjExternal,
    const node & n /* = 0 */)
{
    Skeleton & S = spqrTree.skeleton(mu);
    edge referenceEdge = S.referenceEdge();

    //compute biggest face containing the reference edge:
    face maxFaceContEdge;
    List<node> maxFaceNodes;
    planarEmbed(S.getGraph());
    CombinatorialEmbedding combinatorialEmbedding(S.getGraph());
    T bigFaceSize = -1;
    adjEntry m_adjExternal = 0;
    face f;
    forall_faces(f, combinatorialEmbedding)
    {
        bool containsVirtualEdgeOrN = false;
        adjEntry this_m_adjExternal = 0;
        T sizeOfFace = 0;
        List<node> faceNodes;
        adjEntry ae;
        forall_face_adj(ae, f)
        {
            faceNodes.pushBack(ae->theNode());
            if((n == 0 && (ae->theEdge() == referenceEdge || referenceEdge == 0))
                    || S.original(ae->theNode()) == n)
            {
                containsVirtualEdgeOrN = true;
                if(!(referenceEdge == 0))
                    this_m_adjExternal = ae;
            }

            if(referenceEdge == 0 && !S.isVirtual(ae->theEdge()))
                this_m_adjExternal = ae;

            sizeOfFace += edgeLength[mu][ae->theEdge()]
                          +  nodeLength[S.original(ae->theNode())];
        }

        if(containsVirtualEdgeOrN && !(this_m_adjExternal == 0) && sizeOfFace > bigFaceSize)
        {
            maxFaceNodes = faceNodes;
            bigFaceSize = sizeOfFace;
            maxFaceContEdge = f;
            m_adjExternal = this_m_adjExternal;
        }
    }

    if(adjExternal == 0)
    {
        edge orgEdge = S.realEdge(m_adjExternal->theEdge());
        if(orgEdge->source() == S.original(m_adjExternal->theNode()))
            adjExternal = orgEdge->adjSource();
        else
            adjExternal = orgEdge->adjTarget();
    }

    adjEntry adjMaxFace = m_adjExternal;

    //if embedding is mirror symmetrical embedding of desired embedding,
    //invert adjacency list of all nodes:
    if(!(referenceEdge == 0))
    {
        //successor of adjEntry of virtual edge in adjacency list of leftNode:
        adjEntry succ_virtualEdge_leftNode;
        if(leftNode == referenceEdge->source())
            succ_virtualEdge_leftNode = referenceEdge->adjSource()->succ();
        else
            succ_virtualEdge_leftNode = referenceEdge->adjTarget()->succ();
        if(!succ_virtualEdge_leftNode)
            succ_virtualEdge_leftNode = leftNode->firstAdj();

        bool succVELNAEInExtFace = false;
        adjEntry aeExtFace;
        forall_face_adj(aeExtFace, maxFaceContEdge)
        {
            if(aeExtFace->theEdge() == succ_virtualEdge_leftNode->theEdge())
            {
                succVELNAEInExtFace = true;
                break;
            }
        }

        if(!succVELNAEInExtFace)
        {
            node v;
            forall_nodes(v, S.getGraph())
            {
                List<adjEntry> newAdjOrder;
                for(adjEntry ae = v->firstAdj(); ae; ae = ae->succ())
                    newAdjOrder.pushFront(ae);
                S.getGraph().sort(v, newAdjOrder);
            }
            adjMaxFace = adjMaxFace->twin();
        }
    }

    NodeArray<bool> nodeTreated(S.getGraph(), false);
    adjEntry start_ae;
    if(!(referenceEdge == 0))
    {
        start_ae = adjMaxFace;
        do
        {
            if(start_ae->theEdge() == referenceEdge)
            {
                start_ae = start_ae->faceCycleSucc();
                break;
            }
            start_ae = start_ae->faceCycleSucc();
        }
        while(start_ae != adjMaxFace);
    }
    else
        start_ae = adjMaxFace;

    //For every edge a buffer saving adjacency entries written in embedding step
    //for nodes on the maximum face, needed in step for other nodes.
    EdgeArray< List<adjEntry> > buffer(S.getGraph());

    bool firstStep = true;
    bool after_start_ae = true;
    for(adjEntry ae = start_ae;
            firstStep || ae != start_ae;
            after_start_ae = (!after_start_ae || !ae->succ()) ? false : true,
            ae = after_start_ae ? ae->faceCycleSucc()
                 : (!ae->faceCycleSucc() ? adjMaxFace : ae->faceCycleSucc()))
    {
        firstStep = false;
        //node nodeG = S.original(ae->theNode());
        nodeTreated[ae->theNode()] = true;

        //copy adjacency list of nodes into newOrder:
        ListIterator<adjEntry> before;
        edge vE = (ae->theEdge() == referenceEdge) ? referenceEdge : ae->theEdge();
        node nu = (ae->theEdge() == referenceEdge) ? mu : S.twinTreeNode(ae->theEdge());
        if(S.isVirtual(vE))
        {
            if(ae->theNode() == vE->source())
                before = adjBeforeNodeArraySource[nu];
            else
                before = adjBeforeNodeArrayTarget[nu];
        }

        bool after_ae = true;
        adjEntry m_start_ae;
        if(!(referenceEdge == 0))
        {
            if(ae->theNode() == referenceEdge->source())
                m_start_ae = referenceEdge->adjSource();
            else if(ae->theNode() == referenceEdge->target())
                m_start_ae = referenceEdge->adjTarget();
            else
                m_start_ae = ae;
        }
        else
            m_start_ae = ae;

        adjEntry m_stop_ae;
        bool hit_stop_twice = false;
        int numOfHits = 0;
        if(!(referenceEdge == 0)
                && (ae->theNode() == referenceEdge->source()
                    || ae->theNode() == referenceEdge->target()))
        {
            if(m_start_ae->succ())
                m_stop_ae = m_start_ae->succ();
            else
            {
                m_stop_ae = m_start_ae->theNode()->firstAdj();
                hit_stop_twice = true;
            }
        }
        else
            m_stop_ae = m_start_ae;

        for(adjEntry aeN = m_start_ae;
                after_ae || (hit_stop_twice && numOfHits != 2) || aeN != m_stop_ae;
                after_ae = (!after_ae || !aeN->succ()) ? false : true,
                aeN = after_ae ? aeN->succ()
                      : (!aeN->succ() ? ae->theNode()->firstAdj() : aeN->succ()),
                numOfHits = (aeN == m_stop_ae) ? numOfHits + 1 : numOfHits)
        {
            node m_leftNode = 0;
            if(S.isVirtual(aeN->theEdge()) && aeN->theEdge() != referenceEdge)
            {
                //Compute left node of aeN->theNode(). First get adjacency entry in ext. face
                //(if edge is in ext. face) and compare face cycle successor with successor
                //in node adjacency list. If it is the same, it is the right node, otherwise
                //the left.
                adjEntry aeExtFace = 0;
                bool succInExtFace = false;
                bool aeNInExtFace = false;
                adjEntry aeNSucc = (aeN->succ()) ? aeN->succ() : ae->theNode()->firstAdj();
                aeExtFace = adjMaxFace;
                do
                {
                    if(aeExtFace->theEdge() == aeNSucc->theEdge())
                    {
                        succInExtFace = true;
                        if(aeNInExtFace)
                            break;
                    }
                    if(aeExtFace->theEdge() == aeN->theEdge())
                    {
                        aeNInExtFace = true;
                        if(succInExtFace)
                            break;
                    }
                    aeExtFace = aeExtFace->faceCycleSucc();
                }
                while(aeExtFace != adjMaxFace);
                if(aeNInExtFace && succInExtFace)
                    m_leftNode = aeN->twinNode();
                else
                    m_leftNode = aeN->theNode();

                node twinTN = S.twinTreeNode(aeN->theEdge());
                if(!(referenceEdge == 0))
                {
                    if(aeN->theEdge()->source() == aeN->theNode())
                    {
                        if(aeN->theEdge()->target() == referenceEdge->source())
                            adjBeforeNodeArrayTarget[twinTN] = adjBeforeNodeArraySource[mu];
                        else if(aeN->theEdge()->target() == referenceEdge->target())
                            adjBeforeNodeArrayTarget[twinTN] = adjBeforeNodeArrayTarget[mu];
                    }
                    else
                    {
                        if(aeN->theEdge()->source() == referenceEdge->source())
                            adjBeforeNodeArraySource[twinTN] = adjBeforeNodeArraySource[mu];
                        else if(aeN->theEdge()->source() == referenceEdge->target())
                            adjBeforeNodeArraySource[twinTN] = adjBeforeNodeArrayTarget[mu];
                    }
                }
            }

            adjEntryForNode(aeN, before, spqrTree, treeNodeTreated, mu,
                            m_leftNode, nodeLength, edgeLength, thickness, newOrder,
                            adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                            0, 0, adjExternal);

            //if the other node adjacent to the current treated edge is not in the
            //max face, put written edges into an buffer and clear the adjacency
            //list of that node.
            if(maxFaceNodes.search(aeN->twinNode()) == -1)
            {
                node orig_aeN_twin_theNode = S.original(aeN->twinNode());
                buffer[aeN->theEdge()] = newOrder[orig_aeN_twin_theNode];
                newOrder[orig_aeN_twin_theNode].clear();
            }
        } //for (adjEntry aeN = m_start_ae; [...]
    } //for (adjEntry ae = start_ae; [...]

    //Copy of not treated node's adjacency lists (internal nodes). Setting
    //of left node depending on minimal distance to external face of the
    //face defined by left node.
    bool DGcomputed = false;
    int f_ext_id = 0;
    int f_ref_id = 0;
    Graph* p_DG;
    List<node>* p_fPG_to_nDG;
    NodeArray<int>* p_nDG_to_fPG;
    NodeArray< List<adjEntry> >* p_adjacencyList;
    List< List<adjEntry> >* p_faces;
    NodeArray<T>* p_dist_f_ref;
    NodeArray<T>* p_dist_f_ext;
    AdjEntryArray<int>* p_ae_to_face;

    node v;
    forall_nodes(v, S.getGraph())
    {
        if(nodeTreated[v])
            continue;

        node v_original = S.original(v);
        nodeTreated[v] = true;
        ListIterator<adjEntry> before;
        for(adjEntry ae = v->firstAdj(); ae; ae = ae->succ())
        {
            if(buffer[ae->theEdge()].empty())
            {
                T delta_u_nu = 0;
                T delta_d_nu = 0;
                bool frauenlinks = false;
                if(S.isVirtual(ae->theEdge()))
                {
                    if(!DGcomputed)
                    {
                        p_DG = new Graph();
                        p_fPG_to_nDG = OGDF_NEW List<node>();
                        p_nDG_to_fPG = OGDF_NEW NodeArray<int>();
                        p_adjacencyList = OGDF_NEW NodeArray< List<adjEntry> >();
                        p_faces = OGDF_NEW List< List<adjEntry> >();
                        p_dist_f_ref = OGDF_NEW NodeArray<T>();
                        p_dist_f_ext = OGDF_NEW NodeArray<T>();
                        p_ae_to_face = OGDF_NEW AdjEntryArray<int>(S.getGraph());
                        EdgeArray<T> edgeLengthDG(*p_DG);
                        DGcomputed = true;

                        //compute dual graph of skeleton graph:
                        p_adjacencyList->init(S.getGraph());
                        node nBG;
                        forall_nodes(nBG, S.getGraph())
                        {
                            adjEntry ae_nBG;
                            forall_adj(ae_nBG, nBG)
                            (*p_adjacencyList)[nBG].pushBack(ae_nBG);
                        }

                        NodeArray< List<adjEntry> > adjEntryTreated(S.getGraph());
                        forall_nodes(nBG, S.getGraph())
                        {
                            adjEntry adj;
                            forall_adj(adj, nBG)
                            {
                                if(adjEntryTreated[nBG].search(adj) != -1)
                                    continue;

                                List<adjEntry> newFace;
                                adjEntry adj2 = adj;
                                do
                                {
                                    newFace.pushBack(adj2);
                                    (*p_ae_to_face)[adj2] = p_faces->size();
                                    adjEntryTreated[adj2->theNode()].pushBack(adj2);
                                    node tn = adj2->twinNode();
                                    int idx = (*p_adjacencyList)[tn].search(adj2->twin());
                                    if(idx - 1 < 0)
                                        idx = (*p_adjacencyList)[tn].size() - 1;
                                    else
                                        idx -= 1;
                                    adj2 = *((*p_adjacencyList)[tn].get(idx));
                                }
                                while(adj2 != adj);
                                p_faces->pushBack(newFace);
                            }
                        }

                        p_nDG_to_fPG->init(*p_DG);

                        for(ListIterator< List<adjEntry> > it = p_faces->begin(); it.valid(); it++)
                        {
                            node nn = p_DG->newNode();
                            (*p_nDG_to_fPG)[nn] = p_fPG_to_nDG->search(*(p_fPG_to_nDG->pushBack(nn)));
                        }

                        NodeArray< List<node> > adjFaces(*p_DG);
                        int i = 0;
                        for(ListIterator< List<adjEntry> > it = p_faces->begin(); it.valid(); it++)
                        {
                            int f1_id = i;
                            for(ListIterator<adjEntry> it2 = (*it).begin(); it2.valid(); it2++)
                            {
                                int f2_id = 0;
                                int j = 0;
                                for(ListIterator< List<adjEntry> > it3 = p_faces->begin(); it3.valid(); it3++)
                                {
                                    bool do_break = false;
                                    for(ListIterator<adjEntry> it4 = (*it3).begin(); it4.valid(); it4++)
                                    {
                                        if((*it4) == (*it2)->twin())
                                        {
                                            f2_id = j;
                                            do_break = true;
                                            break;
                                        }
                                    }
                                    if(do_break)
                                        break;
                                    j++;
                                }

                                if(f1_id != f2_id
                                        && adjFaces[*(p_fPG_to_nDG->get(f1_id))].search(*(p_fPG_to_nDG->get(f2_id))) == -1
                                        && adjFaces[*(p_fPG_to_nDG->get(f2_id))].search(*(p_fPG_to_nDG->get(f1_id))) == -1)
                                {
                                    adjFaces[*(p_fPG_to_nDG->get(f1_id))].pushBack(*(p_fPG_to_nDG->get(f2_id)));
                                    edge e1 = p_DG->newEdge(*(p_fPG_to_nDG->get(f1_id)), *(p_fPG_to_nDG->get(f2_id)));
                                    edge e2 = p_DG->newEdge(*(p_fPG_to_nDG->get(f2_id)), *(p_fPG_to_nDG->get(f1_id)));

                                    //set edge length:
                                    T e_length = -1;
                                    for(ListIterator<adjEntry> it_f1 = (*it).begin(); it_f1.valid(); it_f1++)
                                    {
                                        edge e = (*it_f1)->theEdge();
                                        for(ListIterator<adjEntry> it_f2 = (*(p_faces->get(f2_id))).begin();
                                                it_f2.valid();
                                                it_f2++)
                                        {
                                            if((*it_f2)->theEdge() == e)
                                            {
                                                if(e_length == -1 || edgeLength[mu][e] < e_length)
                                                    e_length = edgeLength[mu][e];
                                            }
                                        }
                                    }
                                    edgeLengthDG[e1] = e_length;
                                    edgeLengthDG[e2] = e_length;
                                }

                                if(*it2 == adjMaxFace)
                                    f_ext_id = f1_id;
                                if(!(referenceEdge == 0) && *it2 == adjMaxFace->twin())
                                    f_ref_id = f1_id;
                            }
                            i++;
                        } //for (ListIterator< List<adjEntry> > it = faces.begin(); it.valid(); it++)

                        //compute shortest path from every face to the external face:
                        node f_ext_DG = *(p_fPG_to_nDG->get(f_ext_id));
                        p_dist_f_ext->init(*p_DG);
                        sssp(*p_DG, f_ext_DG, edgeLengthDG, *p_dist_f_ext);
                        if(!(referenceEdge == 0))
                        {
                            node f_ref_DG = *(p_fPG_to_nDG->get(f_ref_id));
                            p_dist_f_ref->init(*p_DG);
                            sssp(*p_DG, f_ref_DG, edgeLengthDG, *p_dist_f_ref);
                        }
                    } //if (!DGcomputed)

                    //choose face with minimal shortest path:
                    T min1, min2;
                    T pi_f_0_f_ext = (*p_dist_f_ext)[*(p_fPG_to_nDG->get((*p_ae_to_face)[ae]))];
                    T pi_f_1_f_ext = (*p_dist_f_ext)[*(p_fPG_to_nDG->get((*p_ae_to_face)[ae->twin()]))];
                    if(!(referenceEdge == 0))
                    {
                        T pi_f_0_f_ref = (*p_dist_f_ref)[*(p_fPG_to_nDG->get((*p_ae_to_face)[ae]))];
                        T pi_f_1_f_ref = (*p_dist_f_ref)[*(p_fPG_to_nDG->get((*p_ae_to_face)[ae->twin()]))];

                        if(delta_u + pi_f_0_f_ref < delta_d + pi_f_0_f_ext)
                            min1 = delta_u + pi_f_0_f_ref;
                        else
                            min1 = delta_d + pi_f_0_f_ext;

                        if(delta_u + pi_f_1_f_ref < delta_d + pi_f_1_f_ext)
                            min2 = delta_u + pi_f_1_f_ref;
                        else
                            min2 = delta_d + pi_f_1_f_ext;

                        if(min1 > min2)
                        {
                            delta_u_nu = delta_u;
                            if(pi_f_0_f_ref < pi_f_0_f_ext)
                                delta_u_nu += pi_f_0_f_ref;
                            else
                                delta_u_nu += pi_f_0_f_ext;
                            delta_d_nu = delta_d;
                            if(pi_f_1_f_ref < pi_f_1_f_ext)
                                delta_d_nu += pi_f_1_f_ref;
                            else
                                delta_d_nu += pi_f_1_f_ext;
                        }
                        else
                        {
                            frauenlinks = true;
                            delta_u_nu = delta_u;
                            if(pi_f_1_f_ref < pi_f_1_f_ext)
                                delta_u_nu += pi_f_1_f_ref;
                            else
                                delta_u_nu += pi_f_1_f_ext;
                            delta_d_nu = delta_d;
                            if(pi_f_0_f_ref < pi_f_0_f_ext)
                                delta_d_nu += pi_f_0_f_ref;
                            else
                                delta_d_nu += pi_f_0_f_ext;
                        }
                    }
                    else
                    {
                        min1 = delta_d + pi_f_0_f_ext;
                        min2 = delta_d + pi_f_1_f_ext;

                        if(min1 > min2)
                        {
                            delta_u_nu = delta_u + pi_f_0_f_ext;
                            delta_d_nu = delta_d + pi_f_1_f_ext;
                        }
                        else
                        {
                            frauenlinks = true;
                            delta_u_nu = delta_u + pi_f_1_f_ext;
                            delta_d_nu = delta_d + pi_f_0_f_ext;
                        }
                    }
                }

                if(frauenlinks)
                {
                    node nu = S.twinTreeNode(ae->theEdge());

                    //buffer computed embedding:
                    NodeArray< List<adjEntry> > tmp_newOrder(spqrTree.originalGraph());
                    ListIterator<adjEntry> tmp_before;

                    adjEntryForNode(
                        ae, tmp_before, spqrTree, treeNodeTreated, mu,
                        v, nodeLength, edgeLength, thickness, tmp_newOrder,
                        adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                        delta_u_nu, delta_d_nu, adjExternal);

                    //copy buffered embedding reversed into newOrder:
                    //node m_leftNode = v;
                    node m_rightNode = ae->twinNode();
                    node leftOrg = v_original;
                    node rightOrg = S.original(m_rightNode);
                    node nOG;
                    forall_nodes(nOG, spqrTree.originalGraph())
                    {
                        List<adjEntry> nOG_tmp_adjList = tmp_newOrder[nOG];
                        if(nOG_tmp_adjList.size() == 0)
                            continue;

                        ListIterator<adjEntry>* m_before;
                        if(nOG == leftOrg)
                            m_before = &before;
                        else
                            m_before = OGDF_NEW ListIterator<adjEntry>();

                        for(ListIterator<adjEntry> ae_it = nOG_tmp_adjList.begin(); ae_it.valid(); ae_it++)
                        {
                            adjEntry adjaEnt = *ae_it;
                            if(!m_before->valid())
                                *m_before = newOrder[nOG].pushBack(adjaEnt);
                            else
                                *m_before = newOrder[nOG].insertBefore(adjaEnt, *m_before);

                            if(nOG == leftOrg || nOG == rightOrg)
                            {
                                if(S.original(ae->theEdge()->source()) == nOG)
                                    adjBeforeNodeArraySource[nu] = *m_before;
                                else
                                    adjBeforeNodeArrayTarget[nu] = *m_before;
                            }
                        }

                        if(nOG != leftOrg)
                            delete m_before;
                    } //forall_nodes(nOG, spqrTree.originalGraph())
                }
                else
                    adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
                                    v, nodeLength, edgeLength, thickness, newOrder,
                                    adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                                    delta_u_nu, delta_d_nu, adjExternal);

                if(!nodeTreated[ae->twinNode()])
                {
                    node orig_ae_twin_theNode = S.original(ae->twinNode());
                    buffer[ae->theEdge()] = newOrder[orig_ae_twin_theNode];
                    newOrder[orig_ae_twin_theNode].clear();
                }
            }
            else
            {
                buffer[ae->theEdge()].reverse();
                for(ListIterator<adjEntry> it = buffer[ae->theEdge()].begin(); it.valid(); it++)
                {
                    if(!before.valid())
                        before = newOrder[v_original].pushFront(*it);
                    else
                        before = newOrder[v_original].insertBefore(*it, before);
                }
            }
        }
    }

    if(DGcomputed)
    {
        delete p_DG;
        delete p_fPG_to_nDG;
        delete p_nDG_to_fPG;
        delete p_adjacencyList;
        delete p_faces;
        delete p_dist_f_ext;
        delete p_dist_f_ref;
        delete p_ae_to_face;
    }
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphsLayers<T>::compute(
    const Graph & G,
    const NodeArray<T> & nodeLength,
    const EdgeArray<T> & edgeLength,
    StaticSPQRTree & spqrTree,
    NodeArray< EdgeArray<T> > & edgeLengthSkel)
{
    //base cases (SPQR-tree implementation would crash for such graphs):
    if(G.numberOfNodes() <= 1 || G.numberOfEdges() <= 2)
        return;

    //set length for all real edges in skeletons to length of the original edge
    //and initialize edge lengths for virtual edges with 0:
    edgeLengthSkel.init(spqrTree.tree());
    node v;
    forall_nodes(v, spqrTree.tree())
    {
        edgeLengthSkel[v].init(spqrTree.skeleton(v).getGraph());
        edge e;
        forall_edges(e, spqrTree.skeleton(v).getGraph())
        {
            if(spqrTree.skeleton(v).isVirtual(e))
                edgeLengthSkel[v][e] = 0;
            else
                edgeLengthSkel[v][e] = edgeLength[spqrTree.skeleton(v).realEdge(e)];
        }
    }

    //set component-length for all non-reference edges:
    bottomUpTraversal(spqrTree, spqrTree.rootNode(), nodeLength, edgeLengthSkel);
    //set component length for all reference edges:
    topDownTraversal(spqrTree, spqrTree.rootNode(), nodeLength, edgeLengthSkel);
}

template<class T>
T EmbedderMaxFaceBiconnectedGraphsLayers<T>::computeSize(
    const Graph & G,
    const NodeArray<T> & nodeLength,
    const EdgeArray<T> & edgeLength)
{
    //base cases (SPQR-tree implementation would crash for such graphs):
    OGDF_ASSERT(G.numberOfNodes() >= 2)
    if(G.numberOfEdges() == 1)
    {
        edge e = G.firstEdge();
        return edgeLength[e] + nodeLength[e->source()] + nodeLength[e->target()];
    }
    else if(G.numberOfEdges() == 2)
    {
        edge e1 = G.firstEdge();
        edge e2 = e1->succ();
        return edgeLength[e1] + edgeLength[e2] + nodeLength[e1->source()] + \
               nodeLength[e1->target()];
    }
    StaticSPQRTree spqrTree(G);
    NodeArray< EdgeArray<T> > edgeLengthSkel;
    return computeSize(G, nodeLength, edgeLength, spqrTree, edgeLengthSkel);
}


template<class T>
T EmbedderMaxFaceBiconnectedGraphsLayers<T>::computeSize(
    const Graph & G,
    const NodeArray<T> & nodeLength,
    const EdgeArray<T> & edgeLength,
    StaticSPQRTree & spqrTree,
    NodeArray< EdgeArray<T> > & edgeLengthSkel)
{
    //base cases (SPQR-tree implementation would crash for such graphs):
    OGDF_ASSERT(G.numberOfNodes() >= 2)
    if(G.numberOfEdges() == 1)
    {
        edge e = G.firstEdge();
        return edgeLength[e] + nodeLength[e->source()] + nodeLength[e->target()];
    }
    else if(G.numberOfEdges() == 2)
    {
        edge e1 = G.firstEdge();
        edge e2 = e1->succ();
        return edgeLength[e1] + edgeLength[e2] + nodeLength[e1->source()] + \
               nodeLength[e1->target()];
    }

    //set length for all real edges in skeletons to length of the original edge
    //and initialize edge lengths for virtual edges with 0:
    edgeLengthSkel.init(spqrTree.tree());
    node v;
    forall_nodes(v, spqrTree.tree())
    {
        edgeLengthSkel[v].init(spqrTree.skeleton(v).getGraph());
        edge e;
        forall_edges(e, spqrTree.skeleton(v).getGraph())
        {
            if(spqrTree.skeleton(v).isVirtual(e))
                edgeLengthSkel[v][e] = 0;
            else
                edgeLengthSkel[v][e] = edgeLength[spqrTree.skeleton(v).realEdge(e)];
        }
    }

    //set component-length for all non-reference edges:
    bottomUpTraversal(spqrTree, spqrTree.rootNode(), nodeLength, edgeLengthSkel);
    //set component length for all reference edges:
    topDownTraversal(spqrTree, spqrTree.rootNode(), nodeLength, edgeLengthSkel);

    T biggestFace = -1;
    node mu;
    forall_nodes(mu, spqrTree.tree())
    {
        //Expand all faces in skeleton(mu) and get size of the largest of them:
        T sizeMu = largestFaceInSkeleton(spqrTree, mu, nodeLength, edgeLengthSkel);
        if(sizeMu > biggestFace)
            biggestFace = sizeMu;
    }

    return biggestFace;
}


template<class T>
T EmbedderMaxFaceBiconnectedGraphsLayers<T>::computeSize(
    const Graph & G,
    const node & n,
    const NodeArray<T> & nodeLength,
    const EdgeArray<T> & edgeLength)
{
    //base cases (SPQR-tree implementation would crash for such graphs):
    OGDF_ASSERT(G.numberOfNodes() >= 2)
    if(G.numberOfEdges() == 1)
    {
        edge e = G.firstEdge();
        return edgeLength[e] + nodeLength[e->source()] + nodeLength[e->target()];
    }
    else if(G.numberOfEdges() == 2)
    {
        edge e1 = G.firstEdge();
        edge e2 = e1->succ();
        return edgeLength[e1] + edgeLength[e2] + nodeLength[e1->source()] + \
               nodeLength[e1->target()];
    }

    StaticSPQRTree spqrTree(G);
    NodeArray< EdgeArray<T> > edgeLengthSkel;
    compute(G, nodeLength, edgeLength, spqrTree, edgeLengthSkel);
    return computeSize(G, n, nodeLength, edgeLength, spqrTree, edgeLengthSkel);
}


template<class T>
T EmbedderMaxFaceBiconnectedGraphsLayers<T>::computeSize(
    const Graph & G,
    const node & n,
    const NodeArray<T> & nodeLength,
    const EdgeArray<T> & edgeLength,
    StaticSPQRTree & spqrTree)
{
    NodeArray< EdgeArray<T> > edgeLengthSkel;
    compute(G, nodeLength, edgeLength, spqrTree, edgeLengthSkel);
    return computeSize(G, n, nodeLength, edgeLength, spqrTree, edgeLengthSkel);
}


template<class T>
T EmbedderMaxFaceBiconnectedGraphsLayers<T>::computeSize(
    const Graph & G,
    const node & n,
    const NodeArray<T> & nodeLength,
    const EdgeArray<T> & edgeLength,
    StaticSPQRTree & spqrTree,
    const NodeArray< EdgeArray<T> > & edgeLengthSkel)
{
    //base cases (SPQR-tree implementation would crash for such graphs):
    OGDF_ASSERT(G.numberOfNodes() >= 2)
    if(G.numberOfEdges() == 1)
    {
        edge e = G.firstEdge();
        return edgeLength[e] + nodeLength[e->source()] + nodeLength[e->target()];
    }
    else if(G.numberOfEdges() == 2)
    {
        edge e1 = G.firstEdge();
        edge e2 = e1->succ();
        return edgeLength[e1] + edgeLength[e2] + nodeLength[e1->source()] + \
               nodeLength[e1->target()];
    }

    edge nAdjEdges;
    node* mus = new node[n->degree()];
    int i = 0;
    T biggestFace = -1;
    forall_adj_edges(nAdjEdges, n)
    {
        mus[i] = spqrTree.skeletonOfReal(nAdjEdges).treeNode();
        bool alreadySeenMu = false;
        for(int j = 0; j < i && !alreadySeenMu; j++)
        {
            if(mus[i] == mus[j])
                alreadySeenMu = true;
        }
        if(alreadySeenMu)
        {
            i++;
            continue;
        }
        else
        {
            //Expand all faces in skeleton(mu) containing n and get size of the largest of them:
            T sizeInMu = largestFaceContainingNode(spqrTree, mus[i], n, nodeLength, edgeLengthSkel);
            if(sizeInMu > biggestFace)
                biggestFace = sizeInMu;

            i++;
        }
    }
    delete mus;

    return biggestFace;
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphsLayers<T>::bottomUpTraversal(
    StaticSPQRTree & spqrTree,
    const node & mu,
    const NodeArray<T> & nodeLength,
    NodeArray< EdgeArray<T> > & edgeLength)
{
    //Recursion:
    edge ed;
    forall_adj_edges(ed, mu)
    {
        if(ed->source() == mu)
            bottomUpTraversal(spqrTree, ed->target(), nodeLength, edgeLength);
    }

    edge e;
    forall_edges(e, spqrTree.skeleton(mu).getGraph())
    {
        //do not treat real edges here and do not treat reference edges:
        if(!spqrTree.skeleton(mu).isVirtual(e) || e == spqrTree.skeleton(mu).referenceEdge())
            continue;

        //pertinent node of e in the SPQR-tree:
        node nu = spqrTree.skeleton(mu).twinTreeNode(e);
        //reference edge of nu (virtual edge in nu associated with mu):
        edge er = spqrTree.skeleton(nu).referenceEdge();
        //sum of the lengths of the two poles of mu:
        node refEdgeSource = spqrTree.skeleton(nu).referenceEdge()->source();
        node origRefEdgeSource = spqrTree.skeleton(nu).original(refEdgeSource);
        node refEdgeTarget = spqrTree.skeleton(nu).referenceEdge()->target();
        node origRefEdgeTarget = spqrTree.skeleton(nu).original(refEdgeTarget);
        T ell = nodeLength[origRefEdgeSource] + nodeLength[origRefEdgeTarget];

        if(spqrTree.typeOf(nu) == SPQRTree::SNode)
        {
            //size of a face in skeleton(nu) minus ell
            T sizeOfFace = 0;
            node nS;
            forall_nodes(nS, spqrTree.skeleton(nu).getGraph())
            sizeOfFace += nodeLength[spqrTree.skeleton(nu).original(nS)];

            edge eS;
            forall_edges(eS, spqrTree.skeleton(nu).getGraph())
            sizeOfFace += edgeLength[nu][eS];

            edgeLength[mu][e] = sizeOfFace - ell;
        }
        else if(spqrTree.typeOf(nu) == SPQRTree::PNode)
        {
            //length of the longest edge different from er in skeleton(nu)
            edge longestEdge = 0;
            forall_edges(ed, spqrTree.skeleton(nu).getGraph())
            {
                if(!(ed == er) && (longestEdge == 0
                                   || edgeLength[nu][ed] > edgeLength[nu][longestEdge]))
                {
                    longestEdge = ed;
                }
            }
            edgeLength[mu][e] = edgeLength[nu][longestEdge];
        }
        else if(spqrTree.typeOf(nu) == SPQRTree::RNode)
        {
            //size of the largest face containing er in skeleton(nu) minus ell
            //Calculate an embedding of the graph (it exists only two which are
            //mirror-symmetrical):
            planarEmbed(spqrTree.skeleton(nu).getGraph());
            CombinatorialEmbedding combinatorialEmbedding(spqrTree.skeleton(nu).getGraph());
            T biggestFaceSize = -1;
            face f;
            forall_faces(f, combinatorialEmbedding)
            {
                T sizeOfFace = 0;
                bool containsEr = false;
                adjEntry ae;
                forall_face_adj(ae, f)
                {
                    if(ae->theEdge() == er)
                        containsEr = true;
                    sizeOfFace += edgeLength[nu][ae->theEdge()]
                                  +  nodeLength[spqrTree.skeleton(nu).original(ae->theNode())];
                }

                if(containsEr && sizeOfFace > biggestFaceSize)
                    biggestFaceSize = sizeOfFace;
            }

            edgeLength[mu][e] = biggestFaceSize - ell;
        }
        else //should never happen
            edgeLength[mu][e] = 1;
    }
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphsLayers<T>::topDownTraversal(
    StaticSPQRTree & spqrTree,
    const node & mu,
    const NodeArray<T> & nodeLength,
    NodeArray< EdgeArray<T> > & edgeLength)
{
    //S: skeleton of mu
    Skeleton & S = spqrTree.skeleton(mu);

    //Get all reference edges of the children nu of mu and set their component length:
    edge ed;
    forall_adj_edges(ed, mu)
    {
        if(ed->source() != mu)
            continue;

        node nu = ed->target();
        edge referenceEdgeOfNu = spqrTree.skeleton(nu).referenceEdge();
        edge eSnu = spqrTree.skeleton(nu).twinEdge(referenceEdgeOfNu);
        if(spqrTree.typeOf(mu) == SPQRTree::SNode)
        {
            //Let L be the sum of the length of all vertices and edges in S. The component
            //length of the reference edge of nu is L minus the length of e_{S, nu} minus
            //the lengths of the two vertices incident to e_{S, nu}.
            T L = 0;
            edge ed2;
            forall_edges(ed2, S.getGraph())
            L += edgeLength[mu][ed2];
            node no;
            forall_nodes(no, S.getGraph())
            L += nodeLength[S.original(no)];

            edgeLength[nu][referenceEdgeOfNu] =   L - edgeLength[mu][eSnu]
                                                  - nodeLength[S.original(eSnu->source())]
                                                  - nodeLength[S.original(eSnu->target())];
        }
        else if(spqrTree.typeOf(mu) == SPQRTree::PNode)
        {
            //The component length of the reference edge of nu is the length of the longest
            //edge in S different from e_{S, nu}.
            edge ed2;
            edge longestEdge = 0;
            forall_edges(ed2, S.getGraph())
            {
                if(ed2 != eSnu && (longestEdge == 0
                                   || edgeLength[mu][ed2] > edgeLength[mu][longestEdge]))
                {
                    longestEdge = ed2;
                }
            }
            edgeLength[nu][referenceEdgeOfNu] = edgeLength[mu][longestEdge];
        }
        else if(spqrTree.typeOf(mu) == SPQRTree::RNode)
        {
            //Let f be the largest face in S containing e_{S, nu}. The component length of
            //the reference edge of nu is the size of f minus the length of e_{S, nu} minus
            //the lengths of the two vertices incident to e_{S, nu}.

            //Calculate an embedding of the graph (it exists only two which are
            //mirror-symmetrical):
            planarEmbed(S.getGraph());
            CombinatorialEmbedding combinatorialEmbedding(S.getGraph());
            T biggestFaceSize = -1;
            face f;
            forall_faces(f, combinatorialEmbedding)
            {
                T sizeOfFace = 0;
                bool containsESnu = false;
                adjEntry ae;
                forall_face_adj(ae, f)
                {
                    if(ae->theEdge() == eSnu)
                        containsESnu = true;
                    sizeOfFace += edgeLength[mu][ae->theEdge()]
                                  +  nodeLength[S.original(ae->theNode())];
                }
                if(containsESnu && sizeOfFace > biggestFaceSize)
                    biggestFaceSize = sizeOfFace;
            }
            edgeLength[nu][referenceEdgeOfNu] = biggestFaceSize - edgeLength[mu][eSnu]
                                                - nodeLength[S.original(eSnu->source())]
                                                - nodeLength[S.original(eSnu->target())];
        }
        else //should never happen
            edgeLength[nu][referenceEdgeOfNu] = 0;

        //Recursion:
        topDownTraversal(spqrTree, ed->target(), nodeLength, edgeLength);
    }
}


template<class T>
T EmbedderMaxFaceBiconnectedGraphsLayers<T>::largestFaceContainingNode(
    const StaticSPQRTree & spqrTree,
    const node & mu,
    const node & n,
    const NodeArray<T> & nodeLength,
    const NodeArray< EdgeArray<T> > & edgeLength)
{
    bool containsARealEdge = false;
    if(spqrTree.typeOf(mu) == SPQRTree::RNode)
    {
        //The largest face containing n is the largest face containg n in any embedding of S.
        planarEmbed(spqrTree.skeleton(mu).getGraph());
        CombinatorialEmbedding combinatorialEmbedding(spqrTree.skeleton(mu).getGraph());
        T biggestFaceSize = -1;
        face f;
        forall_faces(f, combinatorialEmbedding)
        {
            T sizeOfFace = 0;
            bool containingN = false;
            bool m_containsARealEdge = false;
            adjEntry ae;
            forall_face_adj(ae, f)
            {
                if(spqrTree.skeleton(mu).original(ae->theNode()) == n)
                    containingN = true;
                if(!spqrTree.skeleton(mu).isVirtual(ae->theEdge()))
                    m_containsARealEdge = true;
                sizeOfFace += edgeLength[mu][ae->theEdge()];
                sizeOfFace += nodeLength[spqrTree.skeleton(mu).original(ae->theNode())];
            }
            if(containingN && sizeOfFace > biggestFaceSize)
            {
                biggestFaceSize = sizeOfFace;
                containsARealEdge = m_containsARealEdge;
            }
        }

        if(!containsARealEdge)
            return -1;
        return biggestFaceSize;
    }
    else if(spqrTree.typeOf(mu) == SPQRTree::PNode)
    {
        //Find the two longest edges, they define the largest face containg n.
        edge edgeWalker;
        edge longestEdges[2] = {0, 0};
        forall_edges(edgeWalker, spqrTree.skeleton(mu).getGraph())
        {
            if(longestEdges[1] == 0
                    || edgeLength[mu][edgeWalker] > edgeLength[mu][longestEdges[1]])
            {
                if(longestEdges[0] == 0
                        || edgeLength[mu][edgeWalker] > edgeLength[mu][longestEdges[0]])
                {
                    longestEdges[1] = longestEdges[0];
                    longestEdges[0] = edgeWalker;
                }
                else
                    longestEdges[1] = edgeWalker;
            }
        }

        if(!spqrTree.skeleton(mu).isVirtual(longestEdges[0])
                || !spqrTree.skeleton(mu).isVirtual(longestEdges[1]))
        {
            containsARealEdge = true;
        }

        if(!containsARealEdge)
            return -1;

        return edgeLength[mu][longestEdges[0]] + edgeLength[mu][longestEdges[1]];
    }
    else if(spqrTree.typeOf(mu) == SPQRTree::SNode)
    {
        //The largest face containing n is any face in the single existing embedding of S.
        T sizeOfFace = 0;
        node nS;
        forall_nodes(nS, spqrTree.skeleton(mu).getGraph())
        sizeOfFace += nodeLength[spqrTree.skeleton(mu).original(nS)];

        edge eS;
        forall_edges(eS, spqrTree.skeleton(mu).getGraph())
        {
            if(!spqrTree.skeleton(mu).isVirtual(eS))
                containsARealEdge = true;
            sizeOfFace += edgeLength[mu][eS];
        }

        if(!containsARealEdge)
            return -1;

        return sizeOfFace;
    }

    //should never end here...
    return 42;
}


template<class T>
T EmbedderMaxFaceBiconnectedGraphsLayers<T>::largestFaceInSkeleton
(const StaticSPQRTree & spqrTree,
 const node & mu,
 const NodeArray<T> & nodeLength,
 const NodeArray< EdgeArray<T> > & edgeLength)
{
    bool containsARealEdge = false;
    if(spqrTree.typeOf(mu) == SPQRTree::RNode)
    {
        //The largest face is a largest face in any embedding of S.
        planarEmbed(spqrTree.skeleton(mu).getGraph());
        CombinatorialEmbedding combinatorialEmbedding(spqrTree.skeleton(mu).getGraph());
        T biggestFaceSize = -1;
        face f;
        forall_faces(f, combinatorialEmbedding)
        {
            bool m_containsARealEdge = false;
            T sizeOfFace = 0;
            adjEntry ae;
            forall_face_adj(ae, f)
            {
                //node originalNode = spqrTree.skeleton(mu).original(ae->theNode());
                if(!spqrTree.skeleton(mu).isVirtual(ae->theEdge()))
                    m_containsARealEdge = true;
                sizeOfFace += edgeLength[mu][ae->theEdge()]
                              +  nodeLength[spqrTree.skeleton(mu).original(ae->theNode())];
            }

            if(sizeOfFace > biggestFaceSize)
            {
                biggestFaceSize = sizeOfFace;
                containsARealEdge = m_containsARealEdge;
            }
        }

        if(!containsARealEdge)
            return -1;

        return biggestFaceSize;
    }
    else if(spqrTree.typeOf(mu) == SPQRTree::PNode)
    {
        //Find the two longest edges, they define the largest face.
        edge edgeWalker;
        edge longestEdges[2] = {0, 0};
        forall_edges(edgeWalker, spqrTree.skeleton(mu).getGraph())
        {
            if(longestEdges[1] == 0
                    || edgeLength[mu][edgeWalker] > edgeLength[mu][longestEdges[1]])
            {
                if(longestEdges[0] == 0
                        || edgeLength[mu][edgeWalker] > edgeLength[mu][longestEdges[0]])
                {
                    longestEdges[1] = longestEdges[0];
                    longestEdges[0] = edgeWalker;
                }
                else
                    longestEdges[1] = edgeWalker;
            }
        }

        if(!spqrTree.skeleton(mu).isVirtual(longestEdges[0])
                || !spqrTree.skeleton(mu).isVirtual(longestEdges[1]))
        {
            containsARealEdge = true;
        }

        if(!containsARealEdge)
            return -1;

        return edgeLength[mu][longestEdges[0]] + edgeLength[mu][longestEdges[1]];
    }
    else if(spqrTree.typeOf(mu) == SPQRTree::SNode)
    {
        //The largest face is any face in the single existing embedding of S.
        T sizeOfFace = 0;
        node nS;
        forall_nodes(nS, spqrTree.skeleton(mu).getGraph())
        sizeOfFace += nodeLength[spqrTree.skeleton(mu).original(nS)];

        edge eS;
        forall_edges(eS, spqrTree.skeleton(mu).getGraph())
        {
            if(!spqrTree.skeleton(mu).isVirtual(eS))
                containsARealEdge = true;
            sizeOfFace += edgeLength[mu][eS];
        }

        if(!containsARealEdge)
            return -1;

        return sizeOfFace;
    }

    //should never end here...
    return 42;
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphsLayers<T>::bottomUpThickness(
    const StaticSPQRTree & spqrTree,
    const node & mu,
    NodeArray<T> & thickness,
    const NodeArray<T> & nodeLength,
    const NodeArray< EdgeArray<T> > & edgeLength)
{
    //recursion:
    edge e_mu_to_nu;
    forall_adj_edges(e_mu_to_nu, mu)
    {
        if(e_mu_to_nu->source() != mu)
            continue;
        else
        {
            node nu = e_mu_to_nu->target();
            bottomUpThickness(spqrTree, nu, thickness, nodeLength, edgeLength);
        }
    }

    Skeleton & S = spqrTree.skeleton(mu);
    edge referenceEdge = S.referenceEdge();

    if(referenceEdge == 0)  //root of SPQR-tree
    {
        thickness[mu] = 0;
        return;
    }

    //set dLengths for all edges in skeleton graph:
    EdgeArray<T> dLength(S.getGraph());
    edge eSG;
    forall_edges(eSG, S.getGraph())
    {
        if(eSG == referenceEdge)
            continue;

        if(S.isVirtual(eSG))
        {
            node twinTN = S.twinTreeNode(eSG);
            dLength[eSG] = thickness[twinTN];
        }
        else
            dLength[eSG] = edgeLength[mu][eSG];
    }

    //compute thickness of skeleton(mu):
    switch(spqrTree.typeOf(mu))
    {
    case SPQRTree::SNode:
    {
        //thickness(mu) = min_{edges e != referenceEdge} dLength(e)
        T min_dLength = -1;
        forall_edges(eSG, S.getGraph())
        {
            if(eSG == referenceEdge)
                continue;
            if(min_dLength == -1 || dLength[eSG] < min_dLength)
                min_dLength = dLength[eSG];
        }
        thickness[mu] = min_dLength;
    }
    break;
    case SPQRTree::PNode:
    {
        //thickness(mu) = sum_{edges e != referenceEdge} dLength(e)
        T sumof_dLength = 0;
        forall_edges(eSG, S.getGraph())
        {
            if(eSG == referenceEdge)
                continue;
            sumof_dLength += dLength[eSG];
        }
        thickness[mu] = sumof_dLength;
    }
    break;
    case SPQRTree::RNode:
    {
        /* Let f^ref_0, ... , f^ref_k be the faces sharing at least one edge with
         * f_ref - the face adjacent to the reference edge not equal to the
         * external face f_ext. Compute the dual graph and set edge lengths for
         * two faces f_i, f_j, i != j, with at least one shared edge, to the
         * minimal dLength of the shared edges of f_i and f_j. Remove the node
         * related to face f_ref. thickness(mu) is then the length of the shortest
         * path from any of the faces f^ref_0, ... , f^ref_k to f_ext plus 1.
         */
        CombinatorialEmbedding CE(S.getGraph());
        adjEntry ae_f_ext = referenceEdge->adjSource();
        adjEntry ae_f_ref = referenceEdge->adjTarget();
        T faceSize_f_ext = 0;
        adjEntry ae_f_ext_walker = ae_f_ext;
        do
        {
            faceSize_f_ext += nodeLength[S.original(ae_f_ext_walker->theNode())]
                              +  edgeLength[mu][ae_f_ext_walker->theEdge()];
            ae_f_ext_walker = ae_f_ext_walker->faceCycleSucc();
        }
        while(ae_f_ext_walker != ae_f_ext);
        T faceSize_f_ref = 0;
        adjEntry ae_f_ref_walker = ae_f_ref;
        do
        {
            faceSize_f_ref += nodeLength[S.original(ae_f_ref_walker->theNode())]
                              +  edgeLength[mu][ae_f_ref_walker->theEdge()];
            ae_f_ref_walker = ae_f_ref_walker->faceCycleSucc();
        }
        while(ae_f_ref_walker != ae_f_ref);
        if(faceSize_f_ext < faceSize_f_ref)
        {
            adjEntry ae_tmp = ae_f_ext;
            ae_f_ext = ae_f_ref;
            ae_f_ref = ae_tmp;
        }

        //compute dual graph:
        Graph DG;
        List<node> fPG_to_nDG;
        NodeArray<int> nDG_to_fPG(DG);
        NodeArray< List<adjEntry> > adjacencyList(S.getGraph());
        List< List<adjEntry> > faces;
        NodeArray<int> distances;
        EdgeArray<T> edgeLengthDG(DG);
        int f_ref_id = -1;
        int f_ext_id = -1;

        node nBG;
        forall_nodes(nBG, S.getGraph())
        {
            adjEntry ae_nBG;
            forall_adj(ae_nBG, nBG)
            adjacencyList[nBG].pushBack(ae_nBG);
        }

        NodeArray< List<adjEntry> > adjEntryTreated(S.getGraph());
        forall_nodes(nBG, S.getGraph())
        {
            adjEntry adj;
            forall_adj(adj, nBG)
            {
                if(adjEntryTreated[nBG].search(adj) != -1)
                    continue;

                List<adjEntry> newFace;
                adjEntry adj2 = adj;
                do
                {
                    newFace.pushBack(adj2);
                    adjEntryTreated[adj2->theNode()].pushBack(adj2);
                    node tn = adj2->twinNode();
                    int idx = adjacencyList[tn].search(adj2->twin());
                    if(idx - 1 < 0)
                        idx = adjacencyList[tn].size() - 1;
                    else
                        idx -= 1;
                    adj2 = *(adjacencyList[tn].get(idx));
                }
                while(adj2 != adj);
                faces.pushBack(newFace);
            }
        } //forall_nodes(nBG, blockG[bT])

        for(ListIterator< List<adjEntry> > it = faces.begin(); it.valid(); it++)
        {
            node nn = DG.newNode();
            nDG_to_fPG[nn] = fPG_to_nDG.search(*(fPG_to_nDG.pushBack(nn)));
        }

        NodeArray< List<node> > adjFaces(DG);
        int i = 0;
        for(ListIterator< List<adjEntry> > it = faces.begin(); it.valid(); it++)
        {
            int f1_id = i;
            for(ListIterator<adjEntry> it2 = (*it).begin(); it2.valid(); it2++)
            {
                int f2_id = 0;
                int j = 0;
                for(ListIterator< List<adjEntry> > it3 = faces.begin(); it3.valid(); it3++)
                {
                    bool do_break = false;
                    for(ListIterator<adjEntry> it4 = (*it3).begin(); it4.valid(); it4++)
                    {
                        if((*it4) == (*it2)->twin())
                        {
                            f2_id = j;
                            do_break = true;
                            break;
                        }
                    }
                    if(do_break)
                        break;
                    j++;
                }

                if(f1_id != f2_id
                        && adjFaces[*(fPG_to_nDG.get(f1_id))].search(*(fPG_to_nDG.get(f2_id))) == -1
                        && adjFaces[*(fPG_to_nDG.get(f2_id))].search(*(fPG_to_nDG.get(f1_id))) == -1)
                {
                    adjFaces[*(fPG_to_nDG.get(f1_id))].pushBack(*(fPG_to_nDG.get(f2_id)));
                    adjFaces[*(fPG_to_nDG.get(f2_id))].pushBack(*(fPG_to_nDG.get(f1_id)));
                    edge e1 = DG.newEdge(*(fPG_to_nDG.get(f1_id)), *(fPG_to_nDG.get(f2_id)));
                    edge e2 = DG.newEdge(*(fPG_to_nDG.get(f2_id)), *(fPG_to_nDG.get(f1_id)));

                    //set edge length:
                    T e_length = -1;
                    for(ListIterator<adjEntry> it_f1 = (*it).begin(); it_f1.valid(); it_f1++)
                    {
                        edge e = (*it_f1)->theEdge();
                        for(ListIterator<adjEntry> it_f2 = (*(faces.get(f2_id))).begin();
                                it_f2.valid();
                                it_f2++)
                        {
                            if((*it_f2)->theEdge() == e)
                            {
                                if(e_length == -1 || edgeLength[mu][e] < e_length)
                                    e_length = edgeLength[mu][e];
                            }
                        }
                    }
                    edgeLengthDG[e1] = e_length;
                    edgeLengthDG[e2] = e_length;
                }

                if(*it2 == ae_f_ext)
                    f_ext_id = f1_id;
                if(*it2 == ae_f_ref)
                    f_ref_id = f1_id;
            } //for (ListIterator<adjEntry> it2 = (*it).begin(); it2.valid(); it2++)
            i++;
        } //for (ListIterator< List<adjEntry> > it = faces.begin(); it.valid(); it++)

        //the faces sharing at least one edge with f_ref:
        node nDG_f_ref = *(fPG_to_nDG.get(f_ref_id));
        List<node> & f_ref_adj_faces = adjFaces[nDG_f_ref];

        //remove node related to f_ref from dual graph:
        DG.delNode(*(fPG_to_nDG.get(f_ref_id)));

        //compute shortest path and set thickness:
        NodeArray<T> dist(DG);
        node nDG_f_ext = *(fPG_to_nDG.get(f_ext_id));
        sssp(DG, nDG_f_ext, edgeLengthDG, dist);
        T minDist = -1;
        for(ListIterator<node> it_adj_faces = f_ref_adj_faces.begin();
                it_adj_faces.valid();
                it_adj_faces++)
        {
            node fDG = *it_adj_faces;
            if(fDG != nDG_f_ext)
            {
                T d = dist[fDG];
                if(minDist == -1 || d < minDist)
                    minDist = d;
            }
        }
        thickness[mu] = minDist + 1;
    }
    break;
    OGDF_NODEFAULT
    }
}


template<class T>
bool EmbedderMaxFaceBiconnectedGraphsLayers<T>::sssp(
    const Graph & G,
    const node & s,
    const EdgeArray<T> & length,
    NodeArray<T> & d)
{
    const T infinity = 20000000; // big number. danger. think about it.

    //Initialize-Single-Source(G, s):
    d.init(G);
    node v;
    edge e;
    forall_nodes(v, G)
    d[v] = infinity;

    d[s] = 0;
    for(int i = 1; i < G.numberOfNodes(); ++i)
    {
        forall_edges(e, G)
        {
            //relax(u, v, w): // e == (u, v), length == w
            if(d[e->target()] > d[e->source()] + length[e])
                d[e->target()] = d[e->source()] + length[e];
        }
    }

    //check for negative cycle:
    forall_edges(e, G)
    {
        if(d[e->target()] > d[e->source()] + length[e])
            return false;
    }

    return true;
}


} // end namespace ogdf

#endif
