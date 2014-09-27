/*
 * $Revision: 2583 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-12 01:02:21 +0200 (Do, 12. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief declaration of class MMVariableEmbeddingInserter
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

#ifndef OGDF_MM_VARIABLE_EMBEDDING_INSERTER_H
#define OGDF_MM_VARIABLE_EMBEDDING_INSERTER_H



#include <ogdf/module/MMEdgeInsertionModule.h>
#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/FaceArray.h>
#include <ogdf/basic/tuples.h>



namespace ogdf
{

class OGDF_EXPORT NodeSet;
class OGDF_EXPORT StaticPlanarSPQRTree;


//! Minor-monotone edge insertion with variable embedding.
class OGDF_EXPORT MMVariableEmbeddingInserter : public MMEdgeInsertionModule
{
public:
    //! Creates a minor-monotone fixed embedding inserter.
    MMVariableEmbeddingInserter();

    // destruction
    virtual ~MMVariableEmbeddingInserter() { }


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
    class Block;
    class ExpandedSkeleton;

    typedef PlanRepExpansion::Crossing Crossing;

    struct AnchorNodeInfo
    {
        AnchorNodeInfo()
        {
            m_adj_1 = m_adj_2 = 0;
        }
        AnchorNodeInfo(adjEntry adj)
        {
            m_adj_1 = adj;
            m_adj_2 = 0;
        }
        AnchorNodeInfo(adjEntry adj_1, adjEntry adj_2)
        {
            m_adj_1 = adj_1;
            m_adj_2 = adj_2;
        }

        adjEntry m_adj_1;
        adjEntry m_adj_2;
    };

    enum PathType { pathToEdge = 0, pathToSource = 1, pathToTarget = 2 };

    struct Paths
    {
        Paths() :
            m_addPartLeft(3), m_addPartRight(3),
            m_paths(3),
            m_src(0, 2, 0), m_tgt(0, 2, 0),
            m_pred(0, 2, 0)
        { }

        Array<SList<adjEntry> > m_addPartLeft;
        Array<SList<adjEntry> > m_addPartRight;
        Array<List<Crossing> >  m_paths;
        Array<AnchorNodeInfo>   m_src;
        Array<AnchorNodeInfo>   m_tgt;
        Array<int>              m_pred;
    };

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

    /**
     * \brief Collects all anchor nodes (including dummies) of a node.
     *
     * @param v is the current node when traversing all copy nodes of an original node
     *        that are connected in a tree-wise manner.
     * @param nodes is assigned the set of anchor nodes.
     * @param nsParent is the parent node split.
     */
    void collectAnchorNodes(
        node v,
        NodeSet & nodes,
        const PlanRepExpansion::NodeSplit* nsParent) const;

    /**
     * \brief Finds the set of anchor nodes of \a src and \a tgt.
     *
     * @param src is a node in \a PG representing an original node.
     * @param tgt is a node in \a PG representing an original node.
     * @param sources ia assigned the set of anchor nodes of \a src's original node.
     * @param targets ia assigned the set of anchor nodes of \a tgt's original node.
     */
    void findSourcesAndTargets(
        node src, node tgt,
        NodeSet & sources,
        NodeSet & targets) const;

    /**
     * \brief Returns all anchor nodes of \a vOrig in n\a nodes.
     *
     * @param vOrig is a node in the original graph.
     * @param nodes ia assigned the set of anchor nodes.
     */
    void anchorNodes(
        node vOrig,
        NodeSet & nodes) const;

    static node commonDummy(
        NodeSet & sources,
        NodeSet & targets);

    /**
     * \brief Computes insertion path \a eip.
     *
     * The possible start and end nodes of the insertion path have to be stored in
     * \a m_pSources and \a m_pTargets.
     * @param eip    is assigned the insertion path (the crossed edges).
     * @param vStart is assigned the start point of the insertion path.
     * @param vEnd   is assigned the end point of the insertion path.
     */
    void insert(List<Crossing> & eip, AnchorNodeInfo & vStart, AnchorNodeInfo & vEnd);

    node prepareAnchorNode(
        const AnchorNodeInfo & anchor,
        node vOrig,
        bool isSrc,
        edge & eExtra);

    void preprocessInsertionPath(
        const AnchorNodeInfo & srcInfo,
        const AnchorNodeInfo & tgtInfo,
        node srcOrig,
        node tgtOrig,
        node & src,
        node & tgt,
        edge & eSrc,
        edge & eTgt);

    node preparePath(
        node vAnchor,
        adjEntry adjPath,
        bool bOrigEdge,
        node vOrig);

    void findPseudos(
        node vDummy,
        adjEntry adjSrc,
        AnchorNodeInfo & infoSrc,
        SListPure<node> & pseudos);

    void insertWithCommonDummy(
        edge eOrig,
        node vDummy,
        node & src,
        node & tgt);

    /**
     * \brief Implements vertex case of recursive path search in BC-tree.
     *
     * @param v      is the node in the graph currently visited during BC-tree traversal.
     * @param parent is the parent block in DFS-traversal.
     * @param eip is (step-by-step) assigned the insertion path (crossed edges).
     * @param vStart is assigned the start point of \a eip.
     * @param vEnd   is assigned the end point of \a eip.
     */
    bool dfsVertex(node v,
                   int parent,
                   List<Crossing> & eip,
                   AnchorNodeInfo & vStart,
                   AnchorNodeInfo & vEnd);

    /**
     * \brief Implements block case of recursive path search in BC-tree.
     *
     * @param i is the block in the graph currently visited during BC-tree traversal.
     * @param parent is the parent node in DFS-traversal.
     * @param repS is assigned the representative (nodein the graph) of a source node.
     * @param eip is (step-by-step) assigned the insertion path (crossed edges).
     * @param vStart is assigned the start point of \a eip.
     * @param vEnd   is assigned the end point of \a eip.
     */
    bool dfsBlock(int i,
                  node parent,
                  node & repS,
                  List<Crossing> & eip,
                  AnchorNodeInfo & vStart,
                  AnchorNodeInfo & vEnd);

    bool pathSearch(node v, edge parent, const Block & BC, List<edge> & path);

    /**
     * \brief Computes optimal insertion path in block \a BC.
     *
     * @param BC      is the block.
     * @param L       is assigned the insertion path (the crossed edges).
     * @param srcInfo is assigned the start point of the insertion path.
     * @param tgtInfo is assigned the end point of the insertion path.
     */
    void blockInsert(
        Block & BC,
        List<Crossing> & L,
        AnchorNodeInfo & srcInfo,
        AnchorNodeInfo & tgtInfo);

    void buildSubpath(
        node v,
        edge eIn,
        edge eOut,
        Paths & paths,
        bool & bPathToEdge,
        bool & bPathToSrc,
        bool & bPathToTgt,
        ExpandedSkeleton & Exp);

    void contractSplitIfReq(node u);
    void convertDummy(
        node u,
        node vOrig,
        PlanRepExpansion::nodeSplit ns_0);

    void writeEip(const List<Crossing> & eip);

    RemoveReinsertType m_rrOption; //!< The remove-reinsert option.
    double m_percentMostCrossed;   //!< The percentMostCrossed option.

    PlanRepExpansion* m_pPG; //!< Pointer to the planarized expansion.

    NodeSet* m_pSources; //!< The set of possible start nodes of an insertion path.
    NodeSet* m_pTargets; //!< The set of possible end nodes of an insertion path.

    NodeArray<SList<int> > m_compV; //!< The list of blocks containing a node \a v.
    Array<SList<node> >    m_nodeB; //!< The list of nodes in block \a i.
    Array<SList<edge> >    m_edgeB; //!< The list of edges in block \a i.
    NodeArray<node>        m_GtoBC; //!< Maps a node in the planarized expansion to the corresponding node in block.

    bool m_conFinished; //!< Stores if a possible target node in a block has already been found.
    const EdgeArray<bool>* m_forbiddenEdgeOrig;
};

} // end namespace ogdf

#endif
