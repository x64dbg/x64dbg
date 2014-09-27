/*
 * $Revision: 2599 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-15 22:39:24 +0200 (So, 15. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief A simple embedder algorithm.
 *
 * \author Thorsten Kerkhof (thorsten.kerkhof@udo.edu)
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

#ifndef OGDF_SIMPLE_EMBEDDER_H
#define OGDF_SIMPLE_EMBEDDER_H

#include <ogdf/module/EmbedderModule.h>
#include <ogdf/basic/extended_graph_alg.h>
#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/planarity/PlanRep.h>

namespace ogdf
{

//! Planar graph embedding by using default planarEmbed.
class OGDF_EXPORT SimpleEmbedder : public EmbedderModule
{
public:
    // construction / destruction
    SimpleEmbedder() { }
    ~SimpleEmbedder() { }

    /**
     * \brief Call embedder algorithm.
     * \param G is the original graph. Its adjacency list is changed by the embedder.
     * \param adjExternal is an adjacency entry on the external face and is set by the embedder.
     */
    void call(Graph & G, adjEntry & adjExternal);

private:
    /**
     * \brief Find best suited external face according to certain criteria.
     * \param PG is a planar representation of the original graph.
     * \param E is a combinatorial embedding of the original graph.
     * \return Best suited external face.
     */
    face findBestExternalFace(const PlanRep & PG, const CombinatorialEmbedding & E);

};

} // end namespace ogdf

#endif
