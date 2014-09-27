/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of ClusterPlanRep class, allowing cluster
 * boundary insertion and shortest path edge insertion
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

#ifndef OGDF_CLUSTER_PLAN_REP_H
#define OGDF_CLUSTER_PLAN_REP_H



#include <ogdf/planarity/PlanRepUML.h>
#include <ogdf/cluster/ClusterGraphAttributes.h>
#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/cluster/ClusterArray.h>

#include <ogdf/basic/HashArray.h>


namespace ogdf
{


class OGDF_EXPORT ClusterPlanRep : public PlanRep
{

public:

    ClusterPlanRep(
        const ClusterGraphAttributes & acGraph,
        const ClusterGraph & clusterGraph);

    virtual ~ClusterPlanRep() { }

    void initCC(int i);

    //edge on the cluster boundary, adjSource
    void setClusterBoundary(edge e)
    {
        setEdgeTypeOf(e, edgeTypeOf(e) | clusterPattern());
    }
    bool isClusterBoundary(edge e)
    {
        return ((edgeTypeOf(e) & clusterPattern()) == clusterPattern());
    }
    const ClusterGraph & getClusterGraph() const
    {
        return *m_pClusterGraph;
    }

    /** re-inserts edge eOrig by "crossing" the edges in crossedEdges;
     *   splits each edge in crossedEdges
     * Precond.: eOrig is an edge in the original graph,
     *           the edges in crossedEdges are in this graph,
     *           cluster boundaries are modelled as edge paths
     * \param eOrig: Original edge to be inserted
     * \param crossedEdges: Edges that are crossed by this insertion
     * \param E: The embedding in which the edge is inserted
     */
    void insertEdgePathEmbedded(
        edge eOrig,
        CombinatorialEmbedding & E,
        const SList<adjEntry> & crossedEdges);

    void ModelBoundaries();

    //rootadj is set by ModelBoundaries
    adjEntry externalAdj()
    {
        return m_rootAdj;
    }


    //*************************************************************************
    //structural alterations

    // Expands nodes with degree > 4 and merge nodes for generalizations
    virtual void expand(bool lowDegreeExpand = false);

    virtual void expandLowDegreeVertices(OrthoRep & OR);

    //splits edge e, updates clustercage lists if necessary and returns new edge
    virtual edge split(edge e)
    {
        edge eNew = PlanRep::split(e);

        //update edge to cluster info
        m_edgeClusterID[eNew] = m_edgeClusterID[e];
        m_nodeClusterID[eNew->source()] = m_edgeClusterID[e];

        return eNew;
    }//split


    //returns cluster of edge e
    //edges only have unique numbers if clusters are already modelled
    //we derive the edge cluster from the endnode cluster information
    cluster clusterOfEdge(edge e)
    {

        OGDF_ASSERT(m_clusterOfIndex.isDefined(ClusterID(e->source())))
        OGDF_ASSERT(m_clusterOfIndex.isDefined(ClusterID(e->target())))

        OGDF_ASSERT(
            (ClusterID(e->source()) == ClusterID(e->target())) ||
            (clusterOfIndex(ClusterID(e->source())) ==
             clusterOfIndex(ClusterID(e->target()))->parent()) ||
            (clusterOfIndex(ClusterID(e->target())) ==
             clusterOfIndex(ClusterID(e->source()))->parent()) ||
            (clusterOfIndex(ClusterID(e->target()))->parent() ==
             clusterOfIndex(ClusterID(e->source()))->parent())
        )

        if(ClusterID(e->source()) == ClusterID(e->target()))
            return clusterOfIndex(ClusterID(e->target()));
        if(clusterOfIndex(ClusterID(e->source())) ==
                clusterOfIndex(ClusterID(e->target()))->parent())
            return clusterOfIndex(ClusterID(e->source()));
        if(clusterOfIndex(ClusterID(e->target())) ==
                clusterOfIndex(ClusterID(e->source()))->parent())
            return clusterOfIndex(ClusterID(e->target()));
        if(clusterOfIndex(ClusterID(e->target()))->parent() ==
                clusterOfIndex(ClusterID(e->source()))->parent())
            return clusterOfIndex(ClusterID(e->source()))->parent();

        OGDF_THROW(AlgorithmFailureException);
        //return 0;
    }//clusterOfEdge

    inline int ClusterID(node v) const
    {
        return m_nodeClusterID[v];
    }
    inline int ClusterID(edge e) const
    {
        return m_edgeClusterID[e];
    }
    cluster clusterOfIndex(int i)
    {
        return m_clusterOfIndex[i];
    }

    inline cluster clusterOfDummy(node v)
    {
        OGDF_ASSERT(!original(v))
        OGDF_ASSERT(ClusterID(v) != -1)
        return clusterOfIndex(ClusterID(v));
    }

    //output functions
    void writeGML(const char* fileName, const Layout & drawing);
    void writeGML(const char* fileName);
    void writeGML(ostream & os, const Layout & drawing);


protected:

    //insert boundaries for all given clusters
    void convertClusterGraph(cluster act,
                             AdjEntryArray<edge> & currentEdge,
                             AdjEntryArray<int> & outEdge);

    //insert edges to represent the cluster boundary
    void insertBoundary(cluster C,
                        AdjEntryArray<edge> & currentEdge,
                        AdjEntryArray<int> & outEdge,
                        bool clusterIsLeaf);

    //reinsert edges to planarize the graph after convertClusterGraph
    void reinsertEdge(edge e);

private:

    const ClusterGraph* m_pClusterGraph;

    edgeType clusterPattern()
    {
        return etcSecCluster << etoSecondary;
    }

    adjEntry m_rootAdj; //connects cluster on highest level with non cluster or
    //same level


    //******************
    EdgeArray<int> m_edgeClusterID;
    NodeArray<int> m_nodeClusterID;
    //we maintain an array of index to cluster mappings (CG is const)
    //cluster numbers don't need to be consecutive
    HashArray<int, cluster> m_clusterOfIndex;
};


}//namespace

#endif
