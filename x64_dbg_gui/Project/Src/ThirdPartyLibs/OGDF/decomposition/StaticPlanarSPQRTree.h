/*
 * $Revision: 2535 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-04 12:19:10 +0200 (Wed, 04 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class StaticPlanarSPQRTree.
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

// disable wrong warnings (VS compiler bug regarding virtual base classes)
#pragma warning(disable:4250)
#endif


#ifndef OGDF_STATIC_PLANAR_SPQR_TREE_H
#define OGDF_STATIC_PLANAR_SPQR_TREE_H


#include <ogdf/decomposition/StaticSPQRTree.h>
#include <ogdf/decomposition/PlanarSPQRTree.h>


namespace ogdf
{


template<class A, class B> class Tuple2;


//---------------------------------------------------------
// StaticPlanarSPQRTree
// extension of class StaticSPQRTree for support of embedded graphs
//---------------------------------------------------------

//! SPQR-trees of planar graphs.
/**
 * The class StaticPlanarSPQRTree maintains the triconnected components of a
 * planar biconnected graph G and represents all possible embeddings
 * of G. Each skeleton graph is embedded.
 *
 * The current embeddings of the skeletons define an embedding of G.
 * There are two basic operations for obtaining another embedding
 * of G: reverse(v), which flips the skeleton of an R-node v
 * around its poles, and swap(v,e_1,e_2), which exchanges the
 * positions of the edges e_1 and e_2 in the skeleton of a P-node v.
 */

class OGDF_EXPORT StaticPlanarSPQRTree : public StaticSPQRTree, public PlanarSPQRTree
{
public:

    // constructors

    //! Creates an SPQR tree \a T for planar graph \a G rooted at the first edge of \a G.
    /**
     * If \a isEmbedded is set to true, \a G must represent a combinatorial
     * embedding, i.e., the counter-clockwise order of the adjacency entries
     * around each vertex defines an embedding.
     * \pre \a G is planar and biconnected and contains at least 3 nodes,
     *      or \a G has exactly 2 nodes and at least 3  edges.
     */
    StaticPlanarSPQRTree(const Graph & G, bool isEmbedded = false) :
        StaticSPQRTree(G)
    {
        PlanarSPQRTree::init(isEmbedded);
    }

    //! Creates an SPQR tree \a T for planar graph \a G rooted at edge \a e.
    /**
     * If \a isEmbedded is set to true, \a G must represent a combinatorial
     * embedding, i.e., the counter-clockwise order of the adjacency entries
     * around each vertex defines an embedding.
     * \pre \a e is an edge in \a G, and \a G is planar and biconnected and
     * contains at least 3 nodes, or \a G has exactly 2 nodes and at least 3
     * edges.
     */
    StaticPlanarSPQRTree(const Graph & G, edge e, bool isEmbedded = false) :
        StaticSPQRTree(G, e)
    {
        PlanarSPQRTree::init(isEmbedded);
    }
};


} // end namespace ogdf


#endif
