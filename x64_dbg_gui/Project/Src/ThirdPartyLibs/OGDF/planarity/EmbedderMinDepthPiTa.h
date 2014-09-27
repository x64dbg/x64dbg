/*
 * $Revision: 2615 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 14:23:36 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief The algorithm computes a planar embedding with minimum
 * depth if the embedding for all blocks of the graph is given.
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

#ifndef OGDF_EMBEDDER_MIN_DEPTH_PITA_H
#define OGDF_EMBEDDER_MIN_DEPTH_PITA_H


#include <ogdf/module/EmbedderModule.h>
#include <ogdf/decomposition/BCTree.h>


namespace ogdf
{

//! Planar graph embedding with minimum block-nesting depth for given embedded blocks.
/**
 * For details see the paper "Minimum Depth Graph Drawing" by M. Pizzonia and R. Tamassia.
 */
class OGDF_EXPORT EmbedderMinDepthPiTa : public EmbedderModule
{
public:
    //constructor
    EmbedderMinDepthPiTa() : m_useExtendedDepthDefinition(true) { }

    /**
     * \brief Computes an embedding of \a G.
     *
     * \param G is the original graph.
     * \param adjExternal is assigned an adjacency entry on the external face.
     */
    void call(Graph & G, adjEntry & adjExternal);

    bool useExtendedDepthDefinition() const
    {
        return m_useExtendedDepthDefinition;
    }
    void useExtendedDepthDefinition(bool b)
    {
        m_useExtendedDepthDefinition = b;
    }

private:
    bool m_useExtendedDepthDefinition;

    /**
     * \brief Computes recursively an embedding for every block by using the
     * planarEmbed function.
     *
     * \param bT is a block node in the BC-tree.
     * \param cH is a node of bT in the auxiliary graph.
     */
    void embedBlocks(const node & bT, const node & cH);

    /**
     * \brief Computes entry in newOrder for a cutvertex.
     *
     * \param vT is a cut vertex node in the BC-tree.
     * \param root is true if vT is the root node of the block-cutface tree.
     */
    void embedCutVertex(const node & vT, bool root = false);

    /**
     * \brief Computes entries in newOrder for nodes in a block.
     *
     * \param bT is a node in the BC-tree.
     * \param parent_cT is a node in the BC-tree.
     */
    void embedBlockVertex(const node & bT, const node & parent_cT);

    /**
     * \brief Computes recursively edge lengths for the block-cutface tree. The length
     *   of an edge from n to a leaf is 1, from n' to n 2 etc.
     *
     * \param n is a node in the block-cutface tree.
     */
    int computeBlockCutfaceTreeEdgeLengths(const node & n);

    /**
     * \brief Computes recursively the diametral tree.
     *
     * \param n is a node in the block-cutface tree.
     */
    void computeTdiam(const node & n);

    /**
     * \brief Directs all edges to \a n and recursively all edges of its children -
     * except the edge to \a n - to the child.
     *
     * \param G is the tree with the inverted edges.
     * \param n is a node in the original tree.
     * \param e is an edge in the original tree.
     */
    void invertPath(Graph & G, const node & n, const edge & e);

    /**
     * \brief Computes for a block bDG of the dual graph the associated block
     *   in the original graph.
     *
     * \param bDG is a node in the block-cutface tree.
     * \param parent is the parent of bDG in the block-cutface tree.
     * \param blocksNodes is assigned a list containing all nodes of the original
     *   graph of the associated block of bDG.
     * \param childBlocks
     */
    node computeBlockMapping(
        const node & bDG,
        const node & parent,
        List<node> & blocksNodes,
        List<node> & childBlocks);

    /**
     * \brief Computes the eccentricity of a node nT in the block-cutface tree
     *   to all nodes further down in the tree and recursively the eccentricity
     *   to all nodes yet further down its children.
     *
     * \param nT is a node in the block-cutface tree.
     */
    int eccentricityBottomUp(const node & nT);

    /**
     * \brief Computes the eccentricity of a node nT in the block-cutface tree
     *   and recursively the eccentricity of its children.
     *
     * \param nT is a node in the block-cutface tree.
     */
    void eccentricityTopDown(const node & nT);

    /**
     * \brief Computes the depth of an embedding Gamma(B).
     *
     * \param bT is a block-node of the BC-tree.
     */
    int depthBlock(const node & bT);

    /**
     * \brief Computes the depth of an embedding Gamma(c).
     *
     * \param cT is a cutvertex-node of the BC-tree.
     */
    int depthCutvertex(const node & cT);

    /**
     * \brief Deletes inserted dummy nodes. If adjExternal is an adjacency entry
     *   of a dummy edge it is reset.
     *
     * \param G is the graph.
     * \param adjExternal is an adjacency entry on the external face.
     */
    void deleteDummyNodes(Graph & G, adjEntry & adjExternal);

private:
    /** the BC-tree of G */
    BCTree* pBCTree;

    /** the tree of pBCTree rooted at a cutface. */
    Graph bcTreePG;

    /** a mapping of nodes in bcTreePG to nodes in pBCTree->bcTree() */
    NodeArray<node> nBCTree_to_npBCTree;

    /** a mapping of nodes in pBCTree->bcTree() to nodes in bcTreePG */
    NodeArray<node> npBCTree_to_nBCTree;

    /** an adjacency entry on the external face */
    adjEntry* pAdjExternal;

    /** all blocks */
    NodeArray<Graph> blockG;

    /** a mapping of nodes in the auxiliaryGraph of the BC-tree to blockG */
    NodeArray< NodeArray<node> > nH_to_nBlockEmbedding;

    /** a mapping of edges in the auxiliaryGraph of the BC-tree to blockG */
    NodeArray< EdgeArray<edge> > eH_to_eBlockEmbedding;

    /** a mapping of nodes in blockG to the auxiliaryGraph of the BC-tree */
    NodeArray< NodeArray<node> > nBlockEmbedding_to_nH;

    /** a mapping of edges in blockG to the auxiliaryGraph of the BC-tree */
    NodeArray< EdgeArray<edge> > eBlockEmbedding_to_eH;

    /** saving for each node in the block graphs its length */
    NodeArray< NodeArray<int> > nodeLength;

    /** an array saving the length for each edge in the BC-tree */
    EdgeArray<int> m_cB;

    /**
     * \f$M_B = {cH \in B | m_B(cH) = m_B}\f$ with \f$m_B = \max_{c \in B} m_B(c)\f$
     *  and \f$m_B(c) = \max {0} \cup {m_{c, B'} | c \in B', B' \neq B}\f$.
     */
    NodeArray< List<node> > M_B;

    /** Saving edge lengths for the block-cutface tree. */
    EdgeArray<int> edgeLength_blockCutfaceTree;

    /** The diametrical tree of the block-cutface tree. */
    Graph Tdiam;

    /** Needed in computeTdiam function to know if first vertex was already inserted */
    bool Tdiam_initialized;

    /** Mapping nodes from block-cutvertex tree to the diametrical tree */
    NodeArray<node> nBlockCutfaceTree_to_nTdiam;

    /** Mapping nodes from the diametrical tree to block-cutvertex tree*/
    NodeArray<node> nTdiam_to_nBlockCutfaceTree;

    /** The knot of the diametrical tree */
    node knotTdiam;

    /** adjacency entry of the external face of the embedding of G */
    adjEntry tmpAdjExtFace;

    /**
     * a list of all faces in G, a face in this list is containing a list of all
     * adjacency entries
     */
    List< List<adjEntry> > faces;

    /** mapping faces in G to nodes in DG */
    List<node> fPG_to_nDG;

    /** mapping nodes in DG to faces in DG */
    NodeArray<int> nDG_to_fPG;

    /** the block-cutface tree of G (only the graph, rooted at the external face */
    Graph blockCutfaceTree;

    /** the block-cutface tree of G (the BC-tree of the dual graph) */
    BCTree* pm_blockCutfaceTree;

    /** mapping of nodes from the graph blockCutfaceTree to the BC-tree m_blockCutfaceTree */
    NodeArray<node> nBlockCutfaceTree_to_nm_blockCutfaceTree;

    /** mapping of nodes from the BC-tree m_blockCutfaceTree to the graph blockCutfaceTree */
    NodeArray<node> nm_blockCutfaceTree_to_nBlockCutfaceTree;

    /** a mapping of blocks of the graph G to its dual graph DG */
    NodeArray<node> bPG_to_bDG;

    /** a mapping of blocks of the dual graph DG to its original graph G */
    NodeArray<node> bDG_to_bPG;

    /** saving second highest eccentricity for every node of the block-cutface tree */
    NodeArray<int> eccentricity_alt;

    /** saving eccentricity for every node of the block-cutface tree */
    NodeArray<int> eccentricity;

    /**
     * list of nodes which are only in one block which exists of extactly this
     * node plus a cutvertex
     */
    List<node> oneEdgeBlockNodes;

    /**
     * this list is saving the dummy nodes which were inserted to produce a face
     * in one-edge-blocks
     */
    List<node> dummyNodes;

    /** saves for every node of G the new adjacency list */
    NodeArray< List<adjEntry> > newOrder;

    /**
     * given a node nT (cutvertex or block), G_nT is the subgraph of G
     * associated with the subtree of the BC-tree T rooted at nT
     */
    NodeArray<Graph> G_nT;

    /** a mapping of nodes in G_nT to nodes in G */
    NodeArray< NodeArray<node> > nG_nT_to_nPG;

    /** a mapping of nodes in G to nodes in G_nT */
    NodeArray< NodeArray<node> > nPG_to_nG_nT;

    /** a mapping of edges in G_nT to edges in G */
    NodeArray< EdgeArray<edge> > eG_nT_to_ePG;

    /** a mapping of edges in G to edges in G_nT */
    NodeArray< EdgeArray<edge> > ePG_to_eG_nT;

    /** adjacency entry of the external face of G_nT[nT] */
    NodeArray<adjEntry> Gamma_adjExt_nT;
};

} // end namespace ogdf

#endif
