/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class PlanarSPQRTree
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


#ifndef OGDF_PLANAR_SPQR_TREE_H
#define OGDF_PLANAR_SPQR_TREE_H


#include <ogdf/decomposition/SPQRTree.h>


namespace ogdf
{

//---------------------------------------------------------
// PlanarSPQRTree
// extension of class SPQRTree for support of embedded graphs
//---------------------------------------------------------

//! SPQR-trees of planar graphs.
/**
 * The class PlanarSPQRTree maintains the triconnected components of a
 * planar biconnected graph G and represents all possible embeddings
 * of G. Each skeleton graph is embedded.
 *
 * The current embeddings of the skeletons define an embedding of G.
 * There are two basic operations for obtaining another embedding
 * of G: reverse(v), which flips the skeleton of an R-node v
 * around its poles, and swap(v,e_1,e_2), which exchanges the
 * positions of the edges e_1 and e_2 in the skeleton of a P-node v.
 */

class OGDF_EXPORT PlanarSPQRTree : public virtual SPQRTree
{
public:

    //
    // a) Access operations
    //

    //! Returns the number of possible embeddings of G.
    double numberOfEmbeddings() const
    {
        return numberOfEmbeddings(rootNode());
    }

    //! Returns the number of possible embeddings of the pertinent graph of node \a v.
    /**
     * \pre \a v is a node in \a T
     */
    double numberOfEmbeddings(node v) const;


    //
    // b) Update operations
    //

    //! Flips the skeleton \a S of \a vT around its poles.
    /**
     * Reverses the order of adjacency entries of each vertex in \a S.
     * \pre \a vT is an R- or P-node in \a T
     */
    void reverse(node vT);

    //! Exchanges the positions of edges \a e1 and \a e2 in skeleton of \a vT.
    /**
     * \pre \a vT is a P-node in \a T and \a e1 and \a e2 are in edges in
     *      skeleton(\a vT)
     */
    void swap(node vT, edge e1, edge e2);

    //! Exchanges the positions of the two edges corresponding to \a adj1 and \a adj2 in skeleton of \a vT.
    /**
     * \pre \a vT is a P-node in \a T and \a adj1 and \a adj2 are in adjacency entries
     *      in skeleton(\a vT) at the same owner node.
     */
    void swap(node vT, adjEntry adj1, adjEntry adj2);

    //! Embeds \a G according to the current embeddings of the skeletons of \a T.
    /**
     * \pre \a G is the graph passed to the constructor of \a T
     */
    void embed(Graph & G);

    //! Embeds all skeleton graphs randomly.
    void randomEmbed();

    //! Embeds all skeleton graphs randomly and embeds \a G according to the embeddings of the skeletons.
    /**
     * \pre \a G is the graph passed to the constructor of \a T
     */
    void randomEmbed(Graph & G)
    {
        randomEmbed();
        embed(G);
    }


protected:
    //! Initialization (adaption of embeding).
    void init(bool isEmbedded);
    void adoptEmbedding();
    void setPosInEmbedding(
        NodeArray<SListPure<adjEntry> > & adjEdges,
        NodeArray<node> & currentCopy,
        NodeArray<adjEntry> & lastAdj,
        SListPure<node> & current,
        const Skeleton & S,
        adjEntry adj);

    // Embeda original graph according to embeddings of skeletons.
    void expandVirtualEmbed(node vT,
                            adjEntry adjVirt,
                            SListPure<adjEntry> & adjEdges);
    void createInnerVerticesEmbed(Graph & G, node vT);

}; // class PlanarSPQRTree


} // end namespace ogdf


#endif
