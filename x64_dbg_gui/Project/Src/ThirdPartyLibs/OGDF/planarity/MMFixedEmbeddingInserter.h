/*
 * $Revision: 2583 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-12 01:02:21 +0200 (Do, 12. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief declaration of class MMFixedEmbeddingInserter
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

#ifndef OGDF_MM_FIXED_EMBEDDING_INSERTER_H
#define OGDF_MM_FIXED_EMBEDDING_INSERTER_H



#include <ogdf/module/MMEdgeInsertionModule.h>
#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/FaceArray.h>
#include <ogdf/basic/tuples.h>



namespace ogdf
{


class OGDF_EXPORT FaceSetSimple;
class OGDF_EXPORT NodeSetPure;
class OGDF_EXPORT NodeSet;


//! Minor-monotone edge insertion with fixed embedding.
class OGDF_EXPORT MMFixedEmbeddingInserter : public MMEdgeInsertionModule
{
public:
    //! Creates a minor-monotone fixed embedding inserter.
    MMFixedEmbeddingInserter();

    // destruction
    virtual ~MMFixedEmbeddingInserter() { }


    //! Sets the remove-reinsert option for postprocessing.
    void removeReinsert(RemoveReinsertType rrOption)
    {
        m_rrOption = rrOption;
    }

    //! Returns the current setting of the remove-reinsert option.
    RemoveReinsertType removeReinsert() const
    {
        return m_rrOption;
    }


    //! Sets the portion of most crossed edges used during postprocessing.
    /**
     * The value is only used if the remove-reinsert option is set to rrMostCrossed.
     * The number of edges used in postprocessing is then
     * number of edges * percentMostCrossed() / 100.
     */
    void percentMostCrossed(double percent)
    {
        m_percentMostCrossed = percent;
    }

    //! Returns the current setting of the option percentMostCrossed.
    double percentMostCrossed() const
    {
        return m_percentMostCrossed;
    }


private:
    /**
     * \brief Implementation of algorithm call.
     *
     * @param PG is the input planarized expansion and will also receive the result.
     * @param origEdges is the list of original edges (edges in the original graph
     *        of \a PG) that have to be inserted.
     * @param forbiddenEdgeOrig points to an edge array indicating if an original edge is
     *        forbidden to be crossed.
     */
    ReturnType doCall(
        PlanRepExpansion & PG,
        const List<edge> & origEdges,
        const EdgeArray<bool>* forbiddenEdgeOrig);

    //! Constructs the search network (extended dual graph).
    /**
     * @param PG is the planarized expansion.
     * @param E is the corresponding embeddding.
     */
    void constructDual(
        const PlanRepExpansion & PG,
        const CombinatorialEmbedding & E);

    //! Finds a shortest insertion path for an edge.
    /**
     * @param PG is the planarized expansion.
     * @param E is the corresponding embeddding.
     * @param sources is the list of nodes in PG from where the path may start.
     * @param targets is the list of nodes in PG where the path may end.
     * @param crossed is assigned the insertion path. For each crossed edge or
     *        node, we have a pair (\a adj1,\a adj2) in the list; in case of a
     *        crossed edge, \a adj1 corresponds to the crossed edge and adj2
     *        is 0; in case of a crossed node, adj1 (adj2) is the first adjacency
     *        entry assigned to the left (right) node after the split. Additionally,
     *        the first and last element in the list specify, where the path
     *        leaves the source and enters the target node.
     * @param forbiddenEdgeOrig points to an edge array indicating if an original edge is
     *        forbidden to be crossed.
     */
    void findShortestPath(
        const PlanRepExpansion & PG,
        const CombinatorialEmbedding & E,
        const List<node> & sources,
        const List<node> & targets,
        List<Tuple2<adjEntry, adjEntry> > & crossed,
        const EdgeArray<bool>* forbiddenEdgeOrig);

    void prepareAnchorNode(
        PlanRepExpansion & PG,
        CombinatorialEmbedding & E,
        adjEntry & adjStart,
        node srcOrig);

    void preprocessInsertionPath(
        PlanRepExpansion & PG,
        CombinatorialEmbedding & E,
        node srcOrig,
        node tgtOrig,
        //PlanRepExpansion::nodeSplit ns,
        List<Tuple2<adjEntry, adjEntry> > & crossed);

    /**
     * \brief Inserts an edge according to a given insertion path and updates the search network.
     *
     * If an orignal edge \a eOrig is inserted, \a srcOrig and \a tgtOrig are \a eOrig's source
     * and target node, and \a nodeSplit is 0. If a node split is inserted, then \a eOrig is 0,
     * and \a srcOrig and \a tgtOrig refer to the same node (which corresponds to the \a nodeSplit).
     *
     * @param PG is the planarized expansion.
     * @param E is the corresponding embeddding.
     * @param eOrig is the original edge that has to be inserted.
     * @param srcOrig is the original source node.
     * @param tgtOrig is the original target node
     * @param nodeSplit is the node split that has to be inserted.
     * @param crossed is the insertion path as described with findShortestPath().
     */
    void insertEdge(
        PlanRepExpansion & PG,
        CombinatorialEmbedding & E,
        edge eOrig,
        node srcOrig,
        node tgtOrig,
        PlanRepExpansion::NodeSplit* nodeSplit,
        List<Tuple2<adjEntry, adjEntry> > & crossed);

    /**
     * \brief Removes the insertion path of an original edge or a node split and updates the search network.
     *
     * @param PG is the planarized expansion.
     * @param E is the corresponding embeddding.
     * @param eOrig is the original edge whose insertion path shall be removed.
     * @param nodeSplit is the node split whose insertion path shall be removed.
     * @param oldSrc is assigned the source node of the removed insertion path
     *        (might be changed during path removal!).
     * @param oldTgt is assigned the target node of the removed insertion path
     *        (might be changed during path removal!).
     */
    void removeEdge(
        PlanRepExpansion & PG,
        CombinatorialEmbedding & E,
        edge eOrig,
        PlanRepExpansion::NodeSplit* nodeSplit,
        node & oldSrc,
        node & oldTgt);

    /**
     * \brief Inserts dual edges between vertex node \a vDual and left face of \a adj.
     *
     * @param vDual is the dual node of \a adj's node.
     * @param adj is an adjacency entry in the planarized expansion.
     * @param E is the corresponding embeddding.
     */
    void insertDualEdge(node vDual, adjEntry adj, const CombinatorialEmbedding & E);

    /**
     * \brief Inserts all dual edges incident to \a v's dual node.
     *
     * @param v is a node in the planarized expansion.
     * @param E is the corresponding embeddding.
     */
    void insertDualEdges(node v, const CombinatorialEmbedding & E);

    /**
     * \brief Removes a (redundant) node split consisting of a single edge.
     *
     * @param PG is the planarized expansion.
     * @param E is the corresponding embeddding.
     * @param nodeSplit is a node split whose insertion path consists of a single edge.
     */
    void contractSplit(
        PlanRepExpansion & PG,
        CombinatorialEmbedding & E,
        PlanRepExpansion::NodeSplit* nodeSplit);

    /**
     * \brief Reduces the insertion path of a node split at node \a u if required.
     *
     * The insertion path is reduced by unsplitting \a u if \a u has degree 2.
     * @param PG is the planarized expansion.
     * @param E is the corresponding embeddding.
     * @param u is a node in the planarized expansion.
     * @param nsCurrent is a node split which may not be contracted.
     */
    void contractSplitIfReq(
        PlanRepExpansion & PG,
        CombinatorialEmbedding & E,
        node u,
        const PlanRepExpansion::nodeSplit nsCurrent = 0);

    /**
     * \brief Converts a dummy node to a copy of an original node.
     */
    void convertDummy(
        PlanRepExpansion & PG,
        CombinatorialEmbedding & E,
        node u,
        node vOrig,
        PlanRepExpansion::nodeSplit ns);

    /**
     * \brief Collects all anchor nodes (including dummies) of a node.
     *
     * @param v is the current node when traversing all copy nodes of an original node
     *        that are connected in a tree-wise manner.
     * @param nodes is assigned the set of anchor nodes.
     * @param nsParent is the parent node split.
     * @param PG is the planarized expansion.
     */
    void collectAnchorNodes(
        node v,
        NodeSet & nodes,
        const PlanRepExpansion::NodeSplit* nsParent,
        const PlanRepExpansion & PG) const;

    /**
     * \brief Returns all anchor nodes of \a vOrig in n\a nodes.
     *
     * @param vOrig is a node in the original graph.
     * @param nodes ia assigned the set of anchor nodes.
     * @param PG is the planarized expansion.
     */
    void anchorNodes(
        node vOrig,
        NodeSet & nodes,
        const PlanRepExpansion & PG) const;

    /**
     * \brief Finds the set of anchor nodes of \a src and \a tgt.
     *
     * @param src is a node in \a PG representing an original node.
     * @param tgt is a node in \a PG representing an original node.
     * @param sources ia assigned the set of anchor nodes of \a src's original node.
     * @param targets ia assigned the set of anchor nodes of \a tgt's original node.
     * @param PG is the planarized expansion.
     */
    void findSourcesAndTargets(
        node src, node tgt,
        NodeSet & sources,
        NodeSet & targets,
        const PlanRepExpansion & PG) const;

    /**
     * \brief Returns a common dummy node in \a sources and \a targets, or 0 of no such node exists.
     *
     * @param sources is a set of anchor nodes.
     * @param targets is a set of anchor nodes.
     */
    node commonDummy(
        NodeSet & sources,
        NodeSet & targets);

    //! Performs several consistency checks on the seach network.
    bool checkDualGraph(PlanRepExpansion & PG, const CombinatorialEmbedding & E) const;

    bool checkSplitDeg(
        PlanRepExpansion & PG);

    bool origOfDualForbidden(
        edge e,
        const PlanRepExpansion & PG,
        const EdgeArray<bool>* forbiddenEdgeOrig) const
    {
        if(forbiddenEdgeOrig == 0)
            return false;

        if(e->source() == m_vS || e->target() == m_vT)
            return false;

        if(m_primalNode[e->source()] != 0)
            return false;
        if(m_primalNode[e->target()] != 0)
            return false;

        adjEntry adj = m_primalAdj[e];
        if(adj == 0) return false;

        edge eOrig = PG.originalEdge(adj->theEdge());
        if(eOrig != 0)
        {
            //if((*forbiddenEdgeOrig)[eOrig] == true)
            //  cout << "forbidden: " << eOrig << ", dual: " << e << endl;
            return (*forbiddenEdgeOrig)[eOrig];
        }
        else return false;
    }

    void drawDual(
        const PlanRepExpansion & PG,
        const EdgeArray<bool>* forbiddenEdgeOrig);

    RemoveReinsertType m_rrOption; //!< The remove-reinsert option.
    double m_percentMostCrossed;   //!< The percentMostCrossed option.

    Graph m_dual;                      //!< The search network (extended dual graph).
    FaceArray<node>     m_dualOfFace;  //!< The node in dual corresponding to face in primal.
    NodeArray<node>     m_dualOfNode;  //!< The node in dual corresponding to node in primal.
    NodeArray<node>     m_primalNode;  //!< The node in PG corresponding to dual node (0 if face).
    EdgeArray<adjEntry> m_primalAdj;   //!< The adjacency entry in primal graph corresponding to edge in dual.
    AdjEntryArray<edge> m_dualEdge;    //!< The dual edge corresponding to crossing the adjacency entry.
    EdgeArray<int>      m_dualCost;    //!< The cost of an edge in the seach network.

    node m_vS; //!< Represents the start node for the path search.
    node m_vT; //!< Represents the end node for the path search.
    int m_maxCost; //!< The maximal cost of an edge in the search network + 1.

    FaceSetSimple*      m_delFaces;
    FaceSetPure*        m_newFaces;
    NodeSetPure*        m_mergedNodes;
};

} // end namespace ogdf

#endif
