/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class MMSubgraphPlanarizer.
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


#ifndef OGDF_MM_SUBGRAPH_PLANARIZER_H
#define OGDF_MM_SUBGRAPH_PLANARIZER_H

#include <ogdf/module/MMCrossingMinimizationModule.h>
#include <ogdf/module/PlanarSubgraphModule.h>
#include <ogdf/module/MMEdgeInsertionModule.h>
#include <ogdf/basic/ModuleOption.h>


namespace ogdf
{

/**
 * \brief Planarization approach for minor-monotone crossing minimization.
 *
 */
class OGDF_EXPORT MMSubgraphPlanarizer : public MMCrossingMinimizationModule
{
public:
    //! Creates a subgraph planarizer for minor-monotone crossing minimization.
    MMSubgraphPlanarizer();

    //! Sets the module option for the computation of the planar subgraph.
    void setSubgraph(PlanarSubgraphModule* pSubgraph)
    {
        m_subgraph.set(pSubgraph);
    }

    //! Sets the module option for minor-monotone edge insertion.
    void setInserter(MMEdgeInsertionModule* pInserter)
    {
        m_inserter.set(pInserter);
    }

    //! Returns the number of performed permutations in the edge insertion step.
    int permutations()
    {
        return m_permutations;
    }

    //! Sets the number of performed permutations in the edge insertion step.
    void permutations(int p)
    {
        m_permutations = p;
    }

protected:
    virtual ReturnType doCall(PlanRepExpansion & PG,
                              int cc,
                              const EdgeArray<bool>* forbid,
                              int & crossingNumber,
                              int & numNS,
                              int & numSN);

private:
    ModuleOption<PlanarSubgraphModule>  m_subgraph; //!< The planar subgraph module.
    ModuleOption<MMEdgeInsertionModule> m_inserter; //!< The minor-monotone edge insertion module.

    int m_permutations; //!< The number of permutations.
};

}

#endif
