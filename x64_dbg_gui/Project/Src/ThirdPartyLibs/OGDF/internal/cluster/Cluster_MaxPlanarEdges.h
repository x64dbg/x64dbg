/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of a constraint class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem.
 *
 * These constraint do not necessarily belong to the ILP formulation, but
 * have the purpose to strengthen the LP-relaxations in the case of very dense
 * Graphs, by restricting the maximum number of edges that can occur in any optimal
 * solution according to Euler's formula for planar Graphs: |E| <= 3|V|-6
 *
 * \author Mathias Jansen
 *
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

#ifndef OGDF_CLUSTER_MAX_PLANAR_EDGES_H
#define OGDF_CLUSTER_MAX_PLANAR_EDGES_H

#include <ogdf/internal/cluster/Cluster_EdgeVar.h>
#include <ogdf/internal/cluster/basics.h>

#include <abacus/constraint.h>

namespace ogdf
{


class MaxPlanarEdgesConstraint : public ABA_CONSTRAINT
{
#ifdef OGDF_DEBUG
    friend class Sub;
    friend class CPlanarSub;
#endif
public:
    //construction
    MaxPlanarEdgesConstraint(ABA_MASTER* master, int edgeBound, List<nodePair> & edges);
    MaxPlanarEdgesConstraint(ABA_MASTER* master, int edgeBound);

    //destruction
    virtual ~MaxPlanarEdgesConstraint();

    //computes and returns the coefficient for the given variable
    virtual double coeff(ABA_VARIABLE* v);

private:
    List<nodePair> m_edges;
    bool m_graphCons;
};

}

#endif

