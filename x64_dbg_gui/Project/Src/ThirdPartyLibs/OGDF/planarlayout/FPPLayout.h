/*
 * $Revision: 2566 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 23:10:08 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of the Fraysseix, Pach, Pollack Algorithm (FPPLayout)
 *        algorithm.
 *
 * \author Till Sch&auml;fer
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

#ifndef OGDF_FPP_LAYOUT_H
#define OGDF_FPP_LAYOUT_H

#include <ogdf/basic/Graph_d.h>
#include <ogdf/module/GridLayoutModule.h>

namespace ogdf
{

/**
 * The class FPPLayout represents the layout algorithm by
 * de Fraysseix, Pach, Pollack [DPP90]. This algorithm draws a planar graph G
 * straight-line without crossings. G must not contain self-loops or multiple
 * edges. The grid layout size is (2<i>n</i>-4) * (<i>n</i>-2) for a graph with
 * n nodes (<i>n</i> ≥ 3).
 * The algorithm runs in three phases. In the ﬁrst phase, the graph is
 * augmented by adding new artiﬁcial edges to get a triangulated plane graph.
 * Then, a so-called shelling order (also called canonical ordering)
 * for triangulated planar graphs is computed. In the third phase the vertices
 * are placed incrementally according to the shelling order.
 */
class OGDF_EXPORT FPPLayout : public PlanarGridLayoutModule
{
public:
    FPPLayout();

private:
    void doCall(
        const Graph & G,
        adjEntry adjExternal,
        GridLayout & gridLayout,
        IPoint & boundingBox,
        bool fixEmbedding);

    void computeOrder(
        const GraphCopy & G,
        NodeArray<int> & num,
        NodeArray<adjEntry> & e_wp,
        NodeArray<adjEntry> & e_wq,
        adjEntry e_12,
        adjEntry e_2n,
        adjEntry e_n1);

    void computeCoordinates(
        const GraphCopy & G,
        IPoint & boundingBox,
        GridLayout & gridLayout,
        NodeArray<int> & num,
        NodeArray<adjEntry> & e_wp,
        NodeArray<adjEntry> & e_wq);
};


} // end namespace ogdf


#endif
