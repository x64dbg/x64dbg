/*
 * $Revision: 2600 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-15 22:58:25 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Module for planarity testing and planar embeddings.
 *
 * \author Markus Chimani
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

#ifndef OGDF_PLANARITY_MODULE_H
#define OGDF_PLANARITY_MODULE_H

#include <ogdf/basic/Graph.h>

namespace ogdf
{

//! Module for planarity testing and planar embeddings.
/**
 * This is a module defining functions to test planarity of graphs, and to embed planar graphs (i.e., find
 * a rotation scheme of the edges around their incident vertices defining a plane graph).
 *
 * Use this module only if you want to be able to (later on) decide which planarity test to use.
 * If you simply want to test planarity or to embed a graph, use the simpler/preferred method:
 * the direct function calls in extended_graph_alg.h (ogdf::isPlanar, ogdf::planarEmbed,
 * ogdf::planarEmbedPlanarGraph), which use the most efficient BoyerMyrvold algorithm.
 */
class PlanarityModule
{

public:

    PlanarityModule() { }
    virtual ~PlanarityModule() { }

    //! Returns true, if G is planar, false otherwise.
    virtual bool isPlanar(const Graph & G) = 0;

    //! Returns true, if G is planar, false otherwise. In the graph is non-planar, the graph may be arbitrariliy changed after the call.
    /**
     * This variant may be slightly faster than the default isPlanar
     */
    virtual bool isPlanarDestructive(Graph & G) = 0;

    //! Returns true, if G is planar, false otherwise. If true, G contains a planar embedding.
    virtual bool planarEmbed(Graph & G) = 0;

    //! Constructs a planar embedding of G. \a G \b has to be planar!
    /**
     * Returns true if the embedding was successful.
     * Returns false, if the given graph was non-planar
     * (and leaves the graph in an at least partially deleted state)
     *
     * This routine may be slightly faster than planarEmbed, but requires \a G to be planar.
     * If \a G is not planar, the graph will be (partially) destroyed while trying to embed it!
     */
    virtual bool planarEmbedPlanarGraph(Graph & G) = 0;

};

}
#endif
