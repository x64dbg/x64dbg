/*
 * $Revision: 2524 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-03 09:54:22 +0200 (Tue, 03 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of visibility layout algorithm.
 *
 * \author Hoi-Ming Wong
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

//***
// Visibility Layout Method. see "Graph Drawing" by Di Battista et al.
//***


#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_VISIBILITY_LAYOUT_H
#define OGDF_VISIBILITY_LAYOUT_H

#include <ogdf/module/UpwardPlanarizerModule.h>
#include <ogdf/module/LayoutModule.h>
#include <ogdf/basic/ModuleOption.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/FaceArray.h>
#include <ogdf/upward/UpwardPlanRep.h>
#include <ogdf/upward/SubgraphUpwardPlanarizer.h>

namespace ogdf
{


class OGDF_EXPORT VisibilityLayout : public LayoutModule
{
public:

    VisibilityLayout()
    {
        m_grid_dist = 1;
        // set default module
        m_upPlanarizer.set(new SubgraphUpwardPlanarizer());
    }

    virtual void call(GraphAttributes & GA);

    void layout(GraphAttributes & GA, const UpwardPlanRep & UPROrig);

    void setUpwardPlanarizer(UpwardPlanarizerModule* upPlanarizer)
    {
        m_upPlanarizer.set(upPlanarizer);
    }

    void setMinGridDistance(int dist)
    {
        m_grid_dist = dist;
    }



private:

    //min grid distance
    int m_grid_dist;

    Graph D; // the dual graph of the UPR
    node s_D; // super source of D
    node t_D; // super sink f D

    //node segment of the visibility representation
    struct NodeSegment
    {
        int y; //y coordinate
        int x_l; // left x coordinate
        int x_r; // right x coordiante
    };

    // edge segment of the visibility representation
    struct EdgeSegment
    {
        int y_b; // bottom y coordinate
        int y_t; // top y coordinate
        int x; // x coordiante
    };

    //mapping node to node segment of visibility presentation
    NodeArray<NodeSegment> nodeToVis;

    //mapping edge to edge segment of visibility presentation
    EdgeArray<EdgeSegment> edgeToVis;

    FaceArray<node> faceToNode;
    NodeArray<face> leftFace_node;
    NodeArray<face> rightFace_node;
    EdgeArray<face> leftFace_edge;
    EdgeArray<face> rightFace_edge;

    ModuleOption<UpwardPlanarizerModule> m_upPlanarizer; // upward planarizer

    void constructDualGraph(UpwardPlanRep & UPR);

    void constructVisibilityRepresentation(UpwardPlanRep & UPR);


};


}//namespace

#endif
