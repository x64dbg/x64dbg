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

#ifndef OGDF_EMBEDDER_MAX_FACE_BICONNECTED_GRAPHS_H
#define OGDF_EMBEDDER_MAX_FACE_BICONNECTED_GRAPHS_H

#include <ogdf/decomposition/StaticSPQRTree.h>
#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/extended_graph_alg.h>


namespace ogdf
{

//! Computes an embedding of a biconnected graph with maximum external face.
/**
 * See the paper "Graph Embedding with Minimum Depth and
 * Maximum External Face" by C. Gutwenger and P. Mutzel (2004) for
 * details.
 */
template<class T>
class EmbedderMaxFaceBiconnectedGraphs
{
public:
    //! Creates an embedder.
    EmbedderMaxFaceBiconnectedGraphs() { }

    /**
     * \brief Embeds \a G by computing and extending a maximum face in \a G
     *   containing \a n.
     * \param G is the original graph.
     * \param adjExternal is assigned an adjacency entry of the external face.
     * \param nodeLength stores for each vertex in \a G its length.
     * \param edgeLength stores for each edge in \a G its length.
     * \param n is a vertex of the original graph. If n is given, a maximum face
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
     * \param newOrder is saving for each node \a n in \a G the new adjacency
     *   list. This is an output parameter.
     * \param adjBeforeNodeArraySource is saving for the source of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
     * \param adjBeforeNodeArrayTarget is saving for the target of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
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
        NodeArray< List<adjEntry> > & newOrder,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
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
     * \param newOrder is saving for each node \a n in \a G the new adjacency
     *   list. This is an output parameter.
     * \param adjBeforeNodeArraySource is saving for the source of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
     * \param adjBeforeNodeArrayTarget is saving for the target of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
     * \param adjExternal is an adjacency entry in the external face.
     */
    static void expandEdgeSNode(
        const StaticSPQRTree & spqrTree,
        NodeArray<bool> & treeNodeTreated,
        const node & mu,
        const node & leftNode,
        const NodeArray<T> & nodeLength,
        const NodeArray< EdgeArray<T> > & edgeLength,
        NodeArray< List<adjEntry> > & newOrder,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
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
     * \param newOrder is saving for each node \a n in \a G the new adjacency
     *   list. This is an output parameter.
     * \param adjBeforeNodeArraySource is saving for the source of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
     * \param adjBeforeNodeArrayTarget is saving for the target of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
     * \param adjExternal is an adjacency entry in the external face.
     */
    static void expandEdgePNode(
        const StaticSPQRTree & spqrTree,
        NodeArray<bool> & treeNodeTreated,
        const node & mu,
        const node & leftNode,
        const NodeArray<T> & nodeLength,
        const NodeArray< EdgeArray<T> > & edgeLength,
        NodeArray< List<adjEntry> > & newOrder,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
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
     * \param newOrder is saving for each node \a n in \a G the new adjacency
     *   list. This is an output parameter.
     * \param adjBeforeNodeArraySource is saving for the source of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
     * \param adjBeforeNodeArrayTarget is saving for the target of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
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
        NodeArray< List<adjEntry> > & newOrder,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
        adjEntry & adjExternal,
        const node & n);

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
     * \param newOrder is saving for each node \a n in \a G the new adjacency
     *   list. This is an output parameter.
     * \param adjBeforeNodeArraySource is saving for the source of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
     * \param adjBeforeNodeArrayTarget is saving for the target of the reference edge
     *   of the skeleton of mu the adjacency entry, before which new entries have
     *   to be inserted.
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
        NodeArray< List<adjEntry> > & newOrder,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
        NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
        adjEntry & adjExternal);
};


template<class T>
void EmbedderMaxFaceBiconnectedGraphs<T>::embed(
    Graph & G,
    adjEntry & adjExternal,
    const NodeArray<T> & nodeLength,
    const EdgeArray<T> & edgeLength,
    const node & n /* = 0*/)
{
    //Base cases (SPQR-Tree implementation would crash with these inputs):
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
                T sizeInMu = largestFaceContainingNode(spqrTree, mus[i], n,
                                                       nodeLength, edgeLengthSkel);
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

    NodeArray< List<adjEntry> > newOrder(G);
    NodeArray<bool> treeNodeTreated(spqrTree.tree(), false);
    ListIterator<adjEntry> it;
    adjExternal = 0;
    NodeArray< ListIterator<adjEntry> > adjBeforeNodeArraySource(spqrTree.tree());
    NodeArray< ListIterator<adjEntry> > adjBeforeNodeArrayTarget(spqrTree.tree());
    expandEdge(spqrTree, treeNodeTreated, bigFaceMu, 0, nodeLength,
               edgeLengthSkel, newOrder, adjBeforeNodeArraySource,
               adjBeforeNodeArrayTarget, adjExternal, n);

    node v;
    forall_nodes(v, G)
    G.sort(v, newOrder[v]);
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphs<T>::adjEntryForNode(
    adjEntry & ae,
    ListIterator<adjEntry> & before,
    const StaticSPQRTree & spqrTree,
    NodeArray<bool> & treeNodeTreated,
    const node & mu,
    const node & leftNode,
    const NodeArray<T> & nodeLength,
    const NodeArray< EdgeArray<T> > & edgeLength,
    NodeArray< List<adjEntry> > & newOrder,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
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
                       nodeLength, edgeLength, newOrder,
                       adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                       adjExternal);
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
void EmbedderMaxFaceBiconnectedGraphs<T>::expandEdge(
    const StaticSPQRTree & spqrTree,
    NodeArray<bool> & treeNodeTreated,
    const node & mu,
    const node & leftNode,
    const NodeArray<T> & nodeLength,
    const NodeArray< EdgeArray<T> > & edgeLength,
    NodeArray< List<adjEntry> > & newOrder,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
    adjEntry & adjExternal,
    const node & n /*= 0*/)
{
    treeNodeTreated[mu] = true;

    switch(spqrTree.typeOf(mu))
    {
    case SPQRTree::SNode:
        expandEdgeSNode(spqrTree, treeNodeTreated, mu, leftNode,
                        nodeLength, edgeLength, newOrder, adjBeforeNodeArraySource,
                        adjBeforeNodeArrayTarget, adjExternal);
        break;
    case SPQRTree::PNode:
        expandEdgePNode(spqrTree, treeNodeTreated, mu, leftNode,
                        nodeLength, edgeLength, newOrder, adjBeforeNodeArraySource,
                        adjBeforeNodeArrayTarget, adjExternal);
        break;
    case SPQRTree::RNode:
        expandEdgeRNode(spqrTree, treeNodeTreated, mu, leftNode,
                        nodeLength, edgeLength, newOrder, adjBeforeNodeArraySource,
                        adjBeforeNodeArrayTarget, adjExternal, n);
        break;
        OGDF_NODEFAULT
    }
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphs<T>::expandEdgeSNode(
    const StaticSPQRTree & spqrTree,
    NodeArray<bool> & treeNodeTreated,
    const node & mu,
    const node & leftNode,
    const NodeArray<T> & nodeLength,
    const NodeArray< EdgeArray<T> > & edgeLength,
    NodeArray< List<adjEntry> > & newOrder,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
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
            adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
                            m_leftNode, nodeLength, edgeLength, newOrder,
                            adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                            adjExternal);

        if(firstStep)
        {
            beforeSource = before;
            firstStep = false;
        }

        ae = ae->twin();
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
                            m_leftNode, nodeLength, edgeLength, newOrder,
                            adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                            adjExternal);

        //set new adjacency entry pair (ae and its twin):
        if(ae->theNode()->firstAdj() == ae)
            ae = ae->theNode()->lastAdj();
        else
            ae = ae->theNode()->firstAdj();
    }
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphs<T>::expandEdgePNode(
    const StaticSPQRTree & spqrTree,
    NodeArray<bool> & treeNodeTreated,
    const node & mu,
    const node & leftNode,
    const NodeArray<T> & nodeLength,
    const NodeArray< EdgeArray<T> > & edgeLength,
    NodeArray< List<adjEntry> > & newOrder,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
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

    edge longestEdge = 0;
    forall_edges(e, S.getGraph())
    {
        if(e == referenceEdge || e == altReferenceEdge)
            continue;
        if(longestEdge == 0 || edgeLength[mu][e] > edgeLength[mu][longestEdge])
            longestEdge = e;
    }

    List<edge> rightEdgeOrder;
    ListIterator<adjEntry> beforeAltRefEdge;

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

        List<edge> edgeList;
        S.getGraph().allEdges(edgeList);
        adjEntry ae;

        //if left node, longest edge at first:
        if(i == 0)
        {
            if(longestEdge->source() == n)
                ae = longestEdge->adjSource();
            else
                ae = longestEdge->adjTarget();

            if(!(referenceEdge == 0) && S.isVirtual(longestEdge))
            {
                node nu = S.twinTreeNode(longestEdge);
                if(longestEdge->source() == n)
                {
                    if(referenceEdge->source() == n)
                        adjBeforeNodeArrayTarget[nu] = adjBeforeNodeArrayTarget[mu];
                    else
                        adjBeforeNodeArrayTarget[nu] = adjBeforeNodeArraySource[mu];
                }
                else
                {
                    if(referenceEdge->source() == n)
                        adjBeforeNodeArraySource[nu] = adjBeforeNodeArrayTarget[mu];
                    else
                        adjBeforeNodeArraySource[nu] = adjBeforeNodeArraySource[mu];
                }
            }

            adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
                            m_leftNode, nodeLength, edgeLength, newOrder,
                            adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                            adjExternal);
        }

        //all edges except reference edge and longest edge:
        if(i == 0)
        {
            //all virtual edges
            for(ListIterator<edge> it = edgeList.begin(); it.valid(); it++)
            {
                if(*it == referenceEdge || *it == longestEdge || *it == altReferenceEdge || !S.isVirtual(*it))
                    continue;

                node nu = S.twinTreeNode(*it);
                if(!(referenceEdge == 0) && (*it)->source() == n)
                {
                    if(referenceEdge->source() == n)
                        adjBeforeNodeArrayTarget[nu] = adjBeforeNodeArrayTarget[mu];
                    else
                        adjBeforeNodeArrayTarget[nu] = adjBeforeNodeArraySource[mu];
                }
                else if(!(referenceEdge == 0))
                {
                    if(referenceEdge->source() == n)
                        adjBeforeNodeArraySource[nu] = adjBeforeNodeArrayTarget[mu];
                    else
                        adjBeforeNodeArraySource[nu] = adjBeforeNodeArraySource[mu];
                }

                rightEdgeOrder.pushFront(*it);

                if((*it)->source() == n)
                    ae = (*it)->adjSource();
                else
                    ae = (*it)->adjTarget();

                adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
                                m_leftNode, nodeLength, edgeLength, newOrder,
                                adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                                adjExternal);
            }

            //all real edges
            for(ListIterator<edge> it = edgeList.begin(); it.valid(); it++)
            {
                if(*it == referenceEdge || *it == longestEdge || *it == altReferenceEdge || S.isVirtual(*it))
                    continue;

                rightEdgeOrder.pushFront(*it);

                if((*it)->source() == n)
                    ae = (*it)->adjSource();
                else
                    ae = (*it)->adjTarget();

                adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
                                m_leftNode, nodeLength, edgeLength, newOrder,
                                adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                                adjExternal);
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
                                m_leftNode, nodeLength, edgeLength, newOrder,
                                adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                                adjExternal);
            }
        }

        //if not left node, longest edge:
        if(i == 1)
        {
            if(longestEdge->source() == n)
                ae = longestEdge->adjSource();
            else
                ae = longestEdge->adjTarget();

            adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
                            m_leftNode, nodeLength, edgeLength, newOrder,
                            adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                            adjExternal);
        }

        //(alternative) reference edge at last:
        if(!(referenceEdge == 0))
        {
            if(n == referenceEdge->source())
                adjBeforeNodeArraySource[mu] = before;
            else
                adjBeforeNodeArrayTarget[mu] = before;
        }
        else
        {
            node newLeftNode;
            if(i == 0)
                newLeftNode = m_leftNode->firstAdj()->twinNode();
            else
                newLeftNode = m_leftNode;

            if(altReferenceEdge->source() == n)
                ae = altReferenceEdge->adjSource();
            else
                ae = altReferenceEdge->adjTarget();

            adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
                            newLeftNode, nodeLength, edgeLength, newOrder,
                            adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                            adjExternal);

            if(i == 0 && S.isVirtual(altReferenceEdge))
            {
                node nu = S.twinTreeNode(altReferenceEdge);
                if(altReferenceEdge->source() == n)
                    beforeAltRefEdge = adjBeforeNodeArrayTarget[nu];
                else
                    beforeAltRefEdge = adjBeforeNodeArraySource[nu];
            }
        }
    }
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphs<T>::expandEdgeRNode(
    const StaticSPQRTree & spqrTree,
    NodeArray<bool> & treeNodeTreated,
    const node & mu,
    const node & leftNode,
    const NodeArray<T> & nodeLength,
    const NodeArray< EdgeArray<T> > & edgeLength,
    NodeArray< List<adjEntry> > & newOrder,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArraySource,
    NodeArray< ListIterator<adjEntry> > & adjBeforeNodeArrayTarget,
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

    adjEntry adjMaxFace = maxFaceContEdge->firstAdj();

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
        if(ae->theEdge() == referenceEdge)
        {
            if(ae->succ())
                m_start_ae = ae->succ();
            else
                m_start_ae = ae->theNode()->firstAdj();
        }
        else
            m_start_ae = ae;

        for(adjEntry aeN = m_start_ae;
                after_ae || aeN != m_start_ae;
                after_ae = (!after_ae || !aeN->succ()) ? false : true,
                aeN = after_ae ? aeN->succ() : (!aeN->succ() ? ae->theNode()->firstAdj() : aeN->succ())
           )
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
                            m_leftNode, nodeLength, edgeLength, newOrder,
                            adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                            adjExternal);

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

    //Simple copy of not treated node's adjacency lists (internal nodes). Setting
    //of left node not necessary, because all nodes are not in external face.
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
                adjEntryForNode(ae, before, spqrTree, treeNodeTreated, mu,
                                ae->theNode(), nodeLength, edgeLength, newOrder,
                                adjBeforeNodeArraySource, adjBeforeNodeArrayTarget,
                                adjExternal);

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
}


template<class T>
void EmbedderMaxFaceBiconnectedGraphs<T>::compute(
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
T EmbedderMaxFaceBiconnectedGraphs<T>::computeSize(
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
    if(G.numberOfEdges() == 2)
    {
        edge e1 = G.firstEdge();
        edge e2 = e1->succ();
        return edgeLength[e1] + edgeLength[e2] + nodeLength[e1->source()] + nodeLength[e1->target()];
    }
    StaticSPQRTree spqrTree(G);
    NodeArray< EdgeArray<T> > edgeLengthSkel;
    return computeSize(G, nodeLength, edgeLength, spqrTree, edgeLengthSkel);
}


template<class T>
T EmbedderMaxFaceBiconnectedGraphs<T>::computeSize(
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
    if(G.numberOfEdges() == 2)
    {
        edge e1 = G.firstEdge();
        edge e2 = e1->succ();
        return edgeLength[e1] + edgeLength[e2] + nodeLength[e1->source()] + nodeLength[e1->target()];
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
T EmbedderMaxFaceBiconnectedGraphs<T>::computeSize(
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
    if(G.numberOfEdges() == 2)
    {
        edge e1 = G.firstEdge();
        edge e2 = e1->succ();
        return edgeLength[e1] + edgeLength[e2] + nodeLength[e1->source()] + nodeLength[e1->target()];
    }
    StaticSPQRTree spqrTree(G);
    NodeArray< EdgeArray<T> > edgeLengthSkel;
    compute(G, nodeLength, edgeLength, spqrTree, edgeLengthSkel);
    return computeSize(G, n, nodeLength, edgeLength, spqrTree, edgeLengthSkel);
}


template<class T>
T EmbedderMaxFaceBiconnectedGraphs<T>::computeSize(
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
T EmbedderMaxFaceBiconnectedGraphs<T>::computeSize(
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
        return edgeLength[e1] + edgeLength[e2] + nodeLength[e1->source()] + nodeLength[e1->target()];
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
void EmbedderMaxFaceBiconnectedGraphs<T>::bottomUpTraversal(
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
void EmbedderMaxFaceBiconnectedGraphs<T>::topDownTraversal(
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

            edgeLength[nu][referenceEdgeOfNu] =
                L - edgeLength[mu][eSnu] - nodeLength[S.original(eSnu->source())] - nodeLength[S.original(eSnu->target())];
        }
        else if(spqrTree.typeOf(mu) == SPQRTree::PNode)
        {
            //The component length of the reference edge of nu is the length of the longest
            //edge in S different from e_{S, nu}.
            edge ed2;
            edge longestEdge = 0;
            forall_edges(ed2, S.getGraph())
            {
                if(ed2 != eSnu && (longestEdge == 0 || edgeLength[mu][ed2] > edgeLength[mu][longestEdge]))
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
            edgeLength[nu][referenceEdgeOfNu] =
                biggestFaceSize - edgeLength[mu][eSnu]
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
T EmbedderMaxFaceBiconnectedGraphs<T>::largestFaceContainingNode(
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
T EmbedderMaxFaceBiconnectedGraphs<T>::largestFaceInSkeleton(
    const StaticSPQRTree & spqrTree,
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


} // end namespace ogdf

#endif
