/*
 * $Revision: 2566 $
 *
 * last checkin:
 *   $Author:klein $
 *   $Date:2007-10-18 17:23:28 +0200 (Thu, 18 Oct 2007) $
 ***************************************************************/

/** \file
 * \brief Cluster Planarity tests and Cluster Planar embedding
 * for C-connected Cluster Graphs
 *
 * \author Sebastian Leipert
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

#ifndef OGDF_CCONNECT_CLUSTER_PLANAR_H
#define OGDF_CCONNECT_CLUSTER_PLANAR_H


#include <ogdf/internal/planarity/PlanarPQTree.h>
#include <ogdf/cluster/ClusterArray.h>
#include <ogdf/basic/EdgeArray.h>

namespace ogdf
{

class OGDF_EXPORT CconnectClusterPlanar
{

public:

    //aus CCCPE oder CCCP wieder entfernen
    enum ccErrorCode
    {
        none = 0,
        nonConnected = 1,
        nonCConnected = 2,
        nonPlanar = 3,
        nonCPlanar = 4
    };
    ccErrorCode errCode()
    {
        return m_errorCode;
    }


    //! Constructor.
    CconnectClusterPlanar();

    //! Destructor.
    virtual ~CconnectClusterPlanar();

    //! Tests if a ClusterGraph is C-planar.
    virtual bool call(ClusterGraph & C);

    //! Tests if a ClusterGraph is C-planar.
    //! Specifies reason for non planarity.
    bool call(ClusterGraph & C, char(&code)[124]);

    //! Tests if a const ClusterGraph is C-planar.
    virtual bool call(const ClusterGraph & C);

private:

    //! Recursive planarity test for clustered graph induced by \a act.
    bool planarityTest(ClusterGraph & C, cluster & act, Graph & G);
    //! Preprocessing that initializes data structures, used in call.
    bool preProcess(ClusterGraph & C, Graph & G);
    //! Prepares the planarity test for one cluster.
    bool preparation(Graph & G, cluster & C, node superSink);
    //! Performs a planarity test on a biconnected component.
    bool doTest(Graph & G,
                NodeArray<int> & numbering,
                cluster & cl,
                node superSink,
                EdgeArray<edge> & edgeTable);

    void prepareParallelEdges(Graph & G);

    //! Constructs the replacement wheel graphs
    void constructWheelGraph(ClusterGraph & C,
                             Graph & G,
                             cluster & parent,
                             PlanarPQTree* T,
                             EdgeArray<node> & outgoingTable);


    //private Members for handling parallel edges
    EdgeArray<ListPure<edge> >  m_parallelEdges;
    EdgeArray<bool>             m_isParallel;
    ClusterArray<PlanarPQTree*> m_clusterPQTree;
    int m_parallelCount;
    char errorCode[124];
    ccErrorCode m_errorCode;


};

} // end namespace ogdf


#endif
