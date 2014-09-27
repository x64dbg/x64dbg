/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of ClusterPQContainer.
 *
 * Stores information for a biconnected component
 * of a cluster for embedding the cluster in the
 * top down traversal
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


#ifndef OGDF_CLUSTER_PQ_CONTAINER_H
#define OGDF_CLUSTER_PQ_CONTAINER_H

#include <ogdf/cluster/CconnectClusterPlanarEmbed.h>
#include <ogdf/internal/planarity/EmbedPQTree.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/EdgeArray.h>


namespace ogdf
{

class ClusterPQContainer
{

    friend class CconnectClusterPlanarEmbed;


    // Definition
    // incoming edge of v: an edge e = (v,w) with number(v) < number(w)


    // Stores for every node v the keys corresponding to the incoming edges of v
    NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > >* m_inLeaves;

    // Stores for every node v the keys corresponding to the outgoing edges of v
    NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > >* m_outLeaves;

    // Stores for every node v the sequence of incoming edges of v according
    // to the embedding
    NodeArray<SListPure<edge> >* m_frontier;

    // Stores for every node v the nodes corresponding to the
    // opposed sink indicators found in the frontier of v.
    NodeArray<SListPure<node> >* m_opposed;

    // Stores for every node v the nodes corresponding to the
    // non opposed sink indicators found in the frontier of v.
    NodeArray<SListPure<node> >* m_nonOpposed;

    // Table to acces for every edge its corresponding key in the PQTree
    EdgeArray<PlanarLeafKey<IndInfo*>*>* m_edge2Key;

    // Stores for every node its st-number
    NodeArray<int>* m_numbering;

    // Stores for every st-number the node
    Array<node>* m_tableNumber2Node;

    node m_superSink;

    // the subgraph that contains the biconnected component
    // NOT THE COPY OF THE BICONNECTED COMPONENT THAT WAS CONSTRUCTED
    // DURING PLANARITY TESTING. THIS HAS BEEN DELETED.
    Graph*                   m_subGraph;
    // corresponding PQTree
    EmbedPQTree*             m_T;
    // The leaf correpsonding to the edge (s,t).
    PlanarLeafKey<IndInfo*>* m_stEdgeLeaf;

public:

    ClusterPQContainer():
        m_inLeaves(0), m_outLeaves(0), m_frontier(0),
        m_opposed(0), m_nonOpposed(0), m_edge2Key(0),
        m_numbering(0), m_tableNumber2Node(0),
        m_superSink(0), m_subGraph(0), m_T(0), m_stEdgeLeaf(0) { }

    ~ClusterPQContainer() { }

    void init(Graph* subGraph)
    {
        m_subGraph = subGraph;
        m_inLeaves
            = OGDF_NEW NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > >(*subGraph);

        m_outLeaves
            = OGDF_NEW NodeArray<SListPure<PlanarLeafKey<IndInfo*>* > >(*subGraph);

        m_frontier
            = OGDF_NEW NodeArray<SListPure<edge> >(*subGraph);

        m_opposed
            = OGDF_NEW NodeArray<SListPure<node> >(*subGraph);

        m_nonOpposed
            = OGDF_NEW NodeArray<SListPure<node> >(*subGraph);

        m_edge2Key
            = OGDF_NEW EdgeArray<PlanarLeafKey<IndInfo*>*>(*subGraph);

        m_numbering
            = OGDF_NEW NodeArray<int >(*subGraph);

        m_tableNumber2Node
            = OGDF_NEW Array<node>(subGraph->numberOfNodes() + 1);
    }


    void Cleanup()
    {
        if(m_inLeaves)
            delete m_inLeaves;
        if(m_outLeaves)
        {
            node v;
            forall_nodes(v, *m_subGraph)
            {
                while(!(*m_outLeaves)[v].empty())
                {
                    PlanarLeafKey<IndInfo*>* L = (*m_outLeaves)[v].popFrontRet();
                    delete L;
                }
            }
            delete m_outLeaves;
        }
        if(m_frontier)
            delete m_frontier;
        if(m_opposed)
            delete m_opposed;
        if(m_nonOpposed)
            delete m_nonOpposed;
        if(m_edge2Key)
            delete m_edge2Key;
        if(m_T)
        {
            m_T->emptyAllPertinentNodes();
            delete m_T;
        }
        if(m_numbering)
            delete m_numbering;
        if(m_tableNumber2Node)
            delete m_tableNumber2Node;

    }
};

}

#endif
