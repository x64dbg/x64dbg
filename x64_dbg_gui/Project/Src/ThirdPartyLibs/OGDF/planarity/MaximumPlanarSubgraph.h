/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class MaximumPlanarSubgraph.
 *
 * \author Karsten Klein
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

#ifndef OGDF_MAXIMUM_PLANAR_SUBGRAPH_H
#define OGDF_MAXIMUM_PLANAR_SUBGRAPH_H

#include <ogdf/basic/Module.h>
#include <ogdf/basic/Timeouter.h>

#include <ogdf/module/PlanarSubgraphModule.h>
#include <ogdf/cluster/ClusterGraph.h>

#include <ogdf/external/abacus.h>

namespace ogdf
{

//--------------------------------------------------------------------------
//MaximumPlanarSubgraph
//Exact computation of a maximum planar subgraph
//--------------------------------------------------------------------------
class OGDF_EXPORT MaximumPlanarSubgraph : public PlanarSubgraphModule
{

#ifndef USE_ABACUS
protected:
    virtual ReturnType doCall(const Graph & G,
                              const List<edge> & preferedEdges,
                              List<edge> & delEdges,
                              const EdgeArray<int>*  pCost,
                              bool preferedImplyPlanar)
    {
        THROW_NO_ABACUS_EXCEPTION;
        return retError;
    }
};
#else // Use_ABACUS

public:
    // Construction
    MaximumPlanarSubgraph() {}
    // Destruction
    virtual ~MaximumPlanarSubgraph() {}

protected:
    // Implements the Planar Subgraph interface.
    // For the given graph \a G, a clustered graph with only
    // a single root cluster is generated.
    // Computes set of edges delEdges, which have to be deleted
    // in order to get a planar subgraph; edges in preferredEdges
    // should be contained in planar subgraph.
    // Status: pCost and preferredEdges are ignored in current implementation.
    virtual ReturnType doCall(const Graph & G,
                              const List<edge> & preferredEdges,
                              List<edge> & delEdges,
                              const EdgeArray<int>*  pCost,
                              bool preferredImplyPlanar);
};

#endif // USE_ABACUS

} //end namespace ogdf


#endif // OGDF_MAXIMUM_PLANAR_SUBGRAPH_H
