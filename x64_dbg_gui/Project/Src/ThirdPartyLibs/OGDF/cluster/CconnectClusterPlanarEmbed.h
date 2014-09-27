/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Cluster planarity tests and cluster planar embedding
 * for c-connected clustered graphs. Based on the algorithm
 * by Cohen, Feng and Eades which uses PQ-trees.
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

#ifndef OGDF_CCONNECT_CLUSTER_PLANAR_EMBED_H
#define OGDF_CCONNECT_CLUSTER_PLANAR_EMBED_H


#include <ogdf/internal/planarity/EmbedPQTree.h>
#include <ogdf/cluster/ClusterArray.h>
#include <ogdf/basic/Stack.h>
#include <ogdf/internal/cluster/ClusterPQContainer.h>

namespace ogdf
{

class OGDF_EXPORT CconnectClusterPlanarEmbed
{


public:


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


    //*************************************************************************
    // Constructor
    CconnectClusterPlanarEmbed();

    // Destructor
    virtual ~CconnectClusterPlanarEmbed();

    // Tests if a ClusterGraph is C-planar and embedds it.
    virtual bool embed(ClusterGraph & C, Graph & G);

    // Tests if a ClusterGraph is C-planar and embedds it.
    // Specifies reason for non planarity
    virtual bool embed(ClusterGraph & C, Graph & G, char(&code)[124]);


private:

    bool planarityTest(ClusterGraph & C, cluster & act, Graph & G);

    bool preProcess(ClusterGraph & Ccopy, Graph & Gcopy);

    bool preparation(Graph & subGraph, cluster & origCluster, node superSink);

    bool doEmbed(Graph*           biconComp,
                 NodeArray<int> & numbering,
                 cluster     &    origCluster,
                 node             superSink,
                 Graph      &     subGraph,
                 EdgeArray<edge> & tableEdgesBiComp2SubGraph,
                 EdgeArray<edge> & tableEdgesSubGraph2BiComp,
                 NodeArray<node> & tableNodesBiComp2SubGraph);




    void entireEmbed(Graph & biconComp,
                     NodeArray<SListPure<adjEntry> > & entireEmbedding,
                     NodeArray<SListIterator<adjEntry> > & adjMarker,
                     NodeArray<bool> & mark,
                     node v);

    void recursiveEmbed(ClusterGraph & Ccopy, Graph & Gcopy);

    void prepareParallelEdges(Graph & G);


    void constructWheelGraph(ClusterGraph & C,
                             Graph & G,
                             cluster & parent,
                             cluster & origCl,
                             EmbedPQTree* T,
                             EdgeArray<node> & outgoingTable,
                             node superSink);

    void hubControl(Graph & G, NodeArray<bool> & hubs);

    void nonPlanarCleanup(ClusterGraph & Ccopy, Graph & Gcopy);

    void copyEmbedding(ClusterGraph & Ccopy, Graph & Gcopy, ClusterGraph & C, Graph & G);

    //---------------------------------------------------------
    // private member variables for testing a cluster graph
    //---------------------------------------------------------

    // Stores for every cluster the PQTree corresponding
    // to the biconnected component containing the outgoing
    // edges of the cluster.
    ClusterArray<EmbedPQTree*> m_clusterPQTree;

    //save errorcode for postprocessing if not c-planar
    ccErrorCode m_errorCode;
    // For debugging purposes. Stores the reason for
    // non cluster planarity.
    char errorCode[124];


    //private Members for handling parallel edges
    EdgeArray<ListPure<edge> >  m_parallelEdges;
    EdgeArray<bool>             m_isParallel;
    int m_parallelCount;



    //---------------------------------------------------------
    // private member variables for embedding a cluster graph
    //---------------------------------------------------------

    ClusterGraph* m_instance; //The graph that has to be embedded


    // Stores for every cluster the (partial) embedding of the
    // biconnected components not having outgoing
    // edges of the cluster.
    // The NodeArrays are associated with the subgraphs.
    // The ClusterArray is associtated with the original graph.
    ClusterArray<NodeArray<SListPure<adjEntry> >*> m_clusterEmbedding;

    // Stores for every cluster the subgraph constructed to test
    // the planarity of the cluster
    // The ClusterArray is associated with the original graph.
    ClusterArray<Graph*>                m_clusterSubgraph;

    // Marks for every subgraph of a cluster the nodes that are
    // hubs as true.
    // The NodeArrays are associated with the subgraphs.
    // The ClusterArray is associated with the original graph.
    ClusterArray<NodeArray<bool> *>     m_clusterSubgraphHubs;


    // Stores for every node of every subgraph of a cluster
    // if this node belongs to a wheel graph, corresponding to
    // a child cluster
    // The NodeArrays are associated with the subgraphs.
    // The ClusterArray is associated with the original graph.
    ClusterArray<NodeArray<cluster> *>  m_clusterSubgraphWheelGraph;


    // Stores for every mode of every subgraph of a cluster its
    // corresponding node on the original graph G, if there exists one.
    ClusterArray<NodeArray<node> *>     m_clusterNodeTableNew2Orig;


    ClusterArray<ClusterGraph*>         m_clusterClusterGraph;
    ClusterArray<ClusterArray<cluster>*>m_clusterClusterTableOrig2New;

    // When constructing a wheel graph, we store here for
    // every wheel graph node the corresponding cluster
    // Array is associated with the cluster graph copy.
    NodeArray<cluster>                  m_wheelGraphNodes;

    // Stores for every node in the current graph, if
    // it is a hub.
    // Array is associated with the cluster graph copy.
    NodeArray<bool>                     m_currentHubs;


    // Stores for every cluster of Ccopy the corresponding cluster
    // in the original graph. A key variable, since we track
    // all information via the original clusters.
    ClusterArray<cluster>   m_clusterTableCopy2Orig;

    // Needed to construct the ClusterArray m_clusterTableCopy2Orig.
    ClusterArray<cluster>   m_clusterTableOrig2Copy;

    // Stores for every subgraph the super sink of the subgraph.
    ClusterArray<node>      m_clusterSuperSink;


    // Stores for every node in Gcopy its corresponding node
    // in the original graph unless the node belongs to
    // a wheel graph.
    // The NodeArray is associated with Gcopy.
    NodeArray<node>         m_nodeTableCopy2Orig;

    // Needed to construct the NodeArray m_nodeTableCopy2Orig.
    NodeArray<node>         m_nodeTableOrig2Copy;


    EdgeArray<Stack<edge>*> m_outgoingEdgesAnker;
    ClusterArray<EdgeArray<Stack<edge>*>*> m_clusterOutgoingEdgesAnker;

    // Stores for every original cluster all information on
    // the PQ-Tree that is necessary to construct the embedding.
    ClusterArray<ClusterPQContainer> m_clusterPQContainer;

    // Stores the clusters in calling order of the testing algorithm
    // The stack stores the clusters of the original graph.
    // Needed for recursive embed.
    Stack<cluster> m_callStack;

    // Is true for every original cluster, if the cluster does not
    // have a correspondand in the copy of the cluster graph.
    // This is the case if:
    // a. cluster is son of root cluster and does have exactly one
    //    childcluster and no nodes;
    // b. recursive version of a;
    // c. cluster does have no child clusters and no nodes;
    // d. recursive version of c.
    ClusterArray<bool> m_unsatisfiedCluster;


};

} // end namespace ogdf


#endif
