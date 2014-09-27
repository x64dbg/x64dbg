/*
 * $Revision: 2524 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-03 09:54:22 +0200 (Tue, 03 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of upward planarization layout algorithm.
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

#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_UPWARD_PLANARIZATION_LAYOUT_H
#define OGDF_UPWARD_PLANARIZATION_LAYOUT_H



#include <ogdf/module/LayoutModule.h>
#include <ogdf/module/HierarchyLayoutModule.h>
#include <ogdf/module/UpwardPlanarizerModule.h>
#include <ogdf/module/UPRLayoutModule.h>
#include <ogdf/basic/ModuleOption.h>
#include <ogdf/upward/UpwardPlanRep.h>
#include <ogdf/upward/LayerBasedUPRLayout.h>
#include <ogdf/upward/SubgraphUpwardPlanarizer.h>



namespace ogdf
{



class OGDF_EXPORT UpwardPlanarizationLayout : public LayoutModule
{
public:

    // constructor: sets options to default values
    UpwardPlanarizationLayout()
    {
        m_cr_nr = 0;
        // set default module
        m_layout.set(new LayerBasedUPRLayout());
        m_UpwardPlanarizer.set(new SubgraphUpwardPlanarizer());
    }

    // destructor
    ~UpwardPlanarizationLayout() { }


    // calls the algorithm for attributed graph AG
    // returns layout information in AG
    void call(GraphAttributes & AG)
    {
        UpwardPlanRep UPR;
        UPR.createEmpty(AG.constGraph());
        m_UpwardPlanarizer.get().call(UPR);
        m_layout.get().call(UPR, AG);
        m_cr_nr = UPR.numberOfCrossings();
        m_numLevels = m_layout.get().numberOfLevels;
    }


    // module option for the computation of the final layout
    void setUPRLayout(UPRLayoutModule* pLayout)
    {
        m_layout.set(pLayout);
    }


    void setUpwardPlanarizer(UpwardPlanarizerModule* pUpwardPlanarizer)
    {
        m_UpwardPlanarizer.set(pUpwardPlanarizer);
    }

    // returns the number of crossings in the layout after the algorithm
    // has been applied
    int numberOfCrossings() const
    {
        return m_cr_nr;
    }

    int numberOfLevels() const
    {
        return m_numLevels;
    }

protected:

    int m_cr_nr;

    int m_numLevels;

    ModuleOption<UpwardPlanarizerModule> m_UpwardPlanarizer;

    ModuleOption<UPRLayoutModule> m_layout;
};


}

#endif
