/*
 * $Revision: 2566 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 23:10:08 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of CPlanarSubClusteredGraph class.
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

#ifndef OGDF_CPLANAR_SUBCLUSTERED_GRAPH_H
#define OGDF_CPLANAR_SUBCLUSTERED_GRAPH_H


#include <ogdf/cluster/ClusterPlanRep.h>
#include <ogdf/internal/cluster/CPlanarSubClusteredST.h>

namespace ogdf
{

//! Constructs a c-planar subclustered graph of the input on base of a spanning tree
class OGDF_EXPORT CPlanarSubClusteredGraph
{

public:

    CPlanarSubClusteredGraph() { }

    virtual void call(const ClusterGraph & CG, EdgeArray<bool> & inSub);

    virtual void call(
        const ClusterGraph & CGO,
        EdgeArray<bool> & inSub,
        List<edge> & leftOver);

    //! Uses \a edgeWeight to compute clustered planar subgraph
    virtual void call(
        const ClusterGraph & CGO,
        EdgeArray<bool> & inSub,
        List<edge> & leftOver,
        EdgeArray<double> & edgeWeight);

private:

    //****************************************************
    //data fields

    //store status of original edge: in subclustered graph?
    //also used to check spanning tree
    EdgeArray<int> m_edgeStatus;

};//cplanarsubclusteredgraph

} // end namespace ogdf


#endif
