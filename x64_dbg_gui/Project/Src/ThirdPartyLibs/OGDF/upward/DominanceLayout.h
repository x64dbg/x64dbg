/*
 * $Revision: 2524 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-03 09:54:22 +0200 (Tue, 03 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of dominance layout algorithm.
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
// Dominance Drawing Method. see "Graph Drawing" by Di Battista et al.
//***



#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_DOMINANCE_LAYOUT_H
#define OGDF_DOMINANCE_LAYOUT_H


#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/ModuleOption.h>
#include <ogdf/basic/Math.h>
#include <ogdf/module/LayoutModule.h>
#include <ogdf/module/UpwardPlanarizerModule.h>
#include <ogdf/upward/UpwardPlanRep.h>
#include <ogdf/upward/SubgraphUpwardPlanarizer.h>

namespace ogdf
{


class OGDF_EXPORT DominanceLayout : public LayoutModule
{
public:

    DominanceLayout()
    {
        m_grid_dist = 1;
        // set default module
        m_upPlanarizer.set(new SubgraphUpwardPlanarizer());

        m_angle = 45.0 / 180.0 * Math::pi;

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

    double m_angle; //rotate angle to obtain an upward drawing; default is 45�

    NodeArray<edge> firstout;
    NodeArray<edge> lastout;
    NodeArray<edge> firstin;
    NodeArray<edge> lastin;

    int m_R;
    int m_L;

    // list of nodes sorted by their x and y coordinate.
    List<node> xNodes;
    List<node> yNodes;

    //coordinate in preliminary layout
    NodeArray<int> xPreCoord;
    NodeArray<int> yPreCoord;

    //final coordinate  of the nodes of the UPR
    NodeArray<int> xCoord;
    NodeArray<int> yCoord;


    //min grid distance
    int m_grid_dist;

    ModuleOption<UpwardPlanarizerModule> m_upPlanarizer; // upward planarizer

    void labelX(const UpwardPlanRep & UPR, node v, int & count);

    void labelY(const UpwardPlanRep & UPR, node v, int & count);

    void compact(const UpwardPlanRep & UPR, GraphAttributes & GA);

    void findTransitiveEdges(const UpwardPlanRep & UPR, List<edge> & edges);

};


}//namespace

#endif
