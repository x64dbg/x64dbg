/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Abstract base class for structures on graphs, that need
 *        to be informed about cluster graph changes.
 *
 * Follows the observer pattern: cluster graphs are observable
 * objects that can inform observers on changes made to their
 * structure.
 *
 * \author Martin Gronemann
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

#ifndef OGDF_CLUSTER_GRAPH_OBSERVER_H
#define OGDF_CLUSTER_GRAPH_OBSERVER_H


#include <ogdf/basic/List.h>
#include <ogdf/cluster/ClusterGraph.h>

namespace ogdf
{


//----------------------------------------------------------
// GraphObserver
// abstract base class
// derived classes have to overload nodeDeleted, nodeAdded
// edgeDeleted, edgeAdded
// these functions should be called by Graph before (delete)
// and after (add) its structure
//----------------------------------------------------------
class OGDF_EXPORT ClusterGraphObserver
{
    friend class ClusterGraph;

public:
    ClusterGraphObserver() : m_pClusterGraph(0) {}

    ClusterGraphObserver(const ClusterGraph* CG) : m_pClusterGraph(CG)
    {
        m_itCGList = CG->registerObserver(this);
    }//constructor

    virtual ~ClusterGraphObserver()
    {
        if(m_pClusterGraph) m_pClusterGraph->unregisterObserver(m_itCGList);
    }//destructor

    // associates structure with different graph
    void reregister(const ClusterGraph* pCG)
    {
        //small speedup: check if == m_pGraph
        if(m_pClusterGraph) m_pClusterGraph->unregisterObserver(m_itCGList);
        if((m_pClusterGraph = pCG) != 0) m_itCGList = pCG->registerObserver(this);
    }

    virtual void clusterDeleted(cluster v) = 0;
    virtual void clusterAdded(cluster v)   = 0;
    //virtual void reInit()             = 0;
    //virtual void cleared()            = 0;//Graph cleared

    const ClusterGraph*  getGraph() const
    {
        return m_pClusterGraph;
    }

protected:
    const ClusterGraph* m_pClusterGraph; //underlying clustergraph

    //List entry in cluster graphs list of all registered observers
    ListIterator<ClusterGraphObserver*> m_itCGList;
};

} // end of namespace

#endif
