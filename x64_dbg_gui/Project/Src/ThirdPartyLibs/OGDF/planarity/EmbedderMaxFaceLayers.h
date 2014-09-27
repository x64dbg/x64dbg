/*
 * $Revision: 2589 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-12 23:31:45 +0200 (Do, 12. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Computes an embedding of a graph with maximum external face (plus layers approach).
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

#ifndef OGDF_EMBEDDER_MAX_FACE_LAYERS_H
#define OGDF_EMBEDDER_MAX_FACE_LAYERS_H

#include <ogdf/module/EmbedderModule.h>
#include <ogdf/decomposition/BCTree.h>
#include <ogdf/decomposition/StaticSPQRTree.h>

namespace ogdf
{

//! Planar graph embedding with maximum external face (plus layers approach).
/**
 * See the paper "Graph Embedding with Minimum Depth and Maximum External
 * Face" by C. Gutwenger and P. Mutzel (2004) for details.
 * The algorithm for maximum external face is combined with the
 * algorithm for maximum external layers which defines how to embed
 * blocks into inner faces. See diploma thesis "Algorithmen zur
 * Bestimmung von guten Graph-Einbettungen f&uuml;r orthogonale
 * Zeichnungen" (in german) by Thorsten Kerkhof (2007) for details.
 */
class OGDF_EXPORT EmbedderMaxFaceLayers : public EmbedderModule
{
public:
    //constructor and destructor
    EmbedderMaxFaceLayers() { }
    ~EmbedderMaxFaceLayers() { }

    /**
     * \brief Computes an embedding of \a G with maximum external face.
     * \param G is the original graph. Its adjacency list has to be  changed by the embedder.
     * \param adjExternal is assigned an adjacency entry on the external face and has to be set by the embedder.
     */
    void call(Graph & G, adjEntry & adjExternal);

private:
    /**
     * \brief Computes recursively the block graph for every block.
     *
     * \param bT is a block node in the BC-tree.
     * \param cH is a node of bT in the block graph.
     */
    void computeBlockGraphs(const node & bT, const node & cH);

    /**
     * \brief Bottom up traversal of BC-tree.
     *
     * \param bT is the BC-tree node treated in this function call.
     * \param cH is the block node which is related to the cut vertex which is
     *   parent of bT in BC-tree.
     */
    int constraintMaxFace(const node & bT, const node & cH);

    /**
     * \brief Top down traversal of BC-tree.
     *
     * \param bT is the tree node treated in this function call.
     * \param bT_opt is assigned a block node in BC-tree which contains a face which
     *   can be expanded to a maximum face.
     * \param ell_opt is the size of a maximum face.
     */
    void maximumFaceRec(const node & bT, node & bT_opt, int & ell_opt);

    /**
     * \brief Computes the adjacency list for all nodes in a block and calls
     * recursively the function for all blocks incident to nodes in bT.
     *
     * \param bT is the tree node treated in this function call.
     */
    void embedBlock(const node & bT);

    /**
     * \brief Computes the adjacency list for all nodes in a block and calls
     * recursively the function for all blocks incident to nodes in bT.
     *
     * \param bT is the tree node treated in this function call.
     * \param cT is the parent cut vertex node of bT in the BC-tree. cT is 0 if bT
     *   is the root block.
     * \param after is the adjacency entry of the cut vertex, after which bT has to
     *   be inserted.
     */
    void embedBlock(const node & bT, const node & cT, ListIterator<adjEntry> & after);

private:
    /** BC-tree of the original graph */
    BCTree* pBCTree;

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

    /** is saving for each node in the block graphs its cstrLength */
    NodeArray< NodeArray<int> > cstrLength;

    /** saves for every node of G the new adjacency list */
    NodeArray< List<adjEntry> > newOrder;

    /** treeNodeTreated saves for all block nodes in the
     *  BC-tree if it has already been treated or not. */
    NodeArray<bool> treeNodeTreated;

    /** The SPQR-trees of the blocks */
    NodeArray<StaticSPQRTree*> spqrTrees;
};

} // end namespace ogdf

#endif
