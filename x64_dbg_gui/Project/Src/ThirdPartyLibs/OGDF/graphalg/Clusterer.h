/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of Clusterer class that computes a clustering
 *        for a given graph based on the local neighborhood
 *        structure of each edge. Uses the criteria by
 *        Auber, Chiricota, Melancon for small-world graphs to
 *        compute clustering index and edge strength.
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

#ifndef OGDF_CLUSTERER_H
#define OGDF_CLUSTERER_H

#include <ogdf/module/ClustererModule.h>

namespace ogdf
{


/**
 * Clustering is determined based on the threshold values (connectivity
 * thresholds determine edges to be deleted) and stopped if average
 * clustering index drops below m_stopIndex.
 *
 * \pre Input graph has to be connected
 */
class OGDF_EXPORT Clusterer : public ClustererModule
{
public:
    //Constructor taking a graph G to be clustered
    Clusterer(const Graph & G);
    //Default constructor allowing to cluster multiple
    //graphs with the same instance of the Clusterer
    //Clusterer();
    virtual ~Clusterer() {}

    //The clustering can be done recursively (use single threshold
    //on component to delete weak edges (recompute strengths)) or
    //by applying a set of thresholds, set the behaviour in
    //function setRecursive
    virtual void computeClustering(SList<SimpleCluster*> & sl);
    //set the thresholds defining the hierarchy assignment decision
    //should be dependent on the used metrics
    void setClusteringThresholds(const List<double> & threshs);
    //thresholds are computed from edge strengths to split off
    //at least some edges as long as there is a difference between
    //min and max strength (progressive clustering)
    //set this value to 0 to use your own or the default values
    void setAutomaticThresholds(int numValues)
    {
        m_autoThreshNum = numValues;
    }
    //for recursive clustering, only the first threshold is used
    void setRecursive(bool b)
    {
        m_recursive = b;
    }
    //preliminary
    void computeEdgeStrengths(EdgeArray<double> & strength);
    void computeEdgeStrengths(const Graph & G, EdgeArray<double> & strength);

    void createClusterGraph(ClusterGraph & C);

    void setStopIndex(double stop)
    {
        m_stopIndex = stop;
    }

    //compute a clustering index for node v
    //number of connections in neighborhood compared to clique
    virtual double computeCIndex(node v)
    {
        return computeCIndex(*m_pGraph, v);
    }
    virtual double computeCIndex(const Graph & G, node v)
    {
        OGDF_ASSERT(v->graphOf() == &G);
        if(v->degree() < 2) return 1.0;
        int conns = 0; //connections, without v
        NodeArray<bool> neighbor(G, false);
        adjEntry adjE;
        forall_adj(adjE, v)
        {
            neighbor[adjE->twinNode()] = true;
        }
        forall_adj(adjE, v)
        {
            adjEntry adjEE;
            forall_adj(adjEE, adjE->twinNode())
            {
                if(neighbor[adjEE->twinNode()])
                    conns++;
            }
        }
        //connections were counted twice
        double index = conns / 2.0;
        return index / (v->degree() * (v->degree() - 1));
    }

protected:
    EdgeArray<double> m_edgeValue; //strength value for edge clustering index
    NodeArray<double> m_vertexValue; //clustering index for vertices
    List<double> m_thresholds; //clustering level thresholds
    List<double> m_autoThresholds; //automatically generated values (dep. on graph instance)
    List<double> m_defaultThresholds; //some default values
    double m_stopIndex; //average clustering index when recursive clustering stops
    //between 0 and 1
    bool m_recursive; //recursive clustering or list of tresholds
    //bool m_autoThresholds; //compute thresholds according to edge strengths
    int m_autoThreshNum; //number of thresholds to be computed

};//class Clusterer

} //end namespace ogdf

#endif
