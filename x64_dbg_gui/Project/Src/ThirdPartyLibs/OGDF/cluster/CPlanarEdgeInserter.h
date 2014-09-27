/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief  Declares CPlanarEdgeInserter class.
 *
 * Reinsertion of deleted edges in embedded subgraph with
 * modeled cluster boundaries.
 * The inserter class computes a shortest path on the dual
 * graph of the input to find an insertion path
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


#ifndef OGDF_CPLANAR_EDGE_INSERTER_H
#define OGDF_CPLANAR_EDGE_INSERTER_H


#include <ogdf/cluster/ClusterPlanRep.h>

namespace ogdf
{

class NodePair
{
public:
    node m_src, m_tgt;
};

class OGDF_EXPORT CPlanarEdgeInserter
{
    //postprocessing options
    enum PostProcessType {ppNone, ppRemoveReinsert};

public:

    CPlanarEdgeInserter() { }

    virtual ~CPlanarEdgeInserter() { }

    void call(
        ClusterPlanRep & CPR,
        CombinatorialEmbedding & E,
        Graph & G,
        const List<NodePair> & origEdges,
        List<edge> & newEdges);

    void setPostProcessing(PostProcessType p)
    {
        m_ppType = p;
    }

    PostProcessType getPostProcessing()
    {
        return m_ppType;
    }

protected:

    void constructDualGraph(
        ClusterPlanRep & CPR,
        CombinatorialEmbedding & E,
        EdgeArray<edge> & arcRightToLeft,
        EdgeArray<edge> & arcLeftToRight,
        FaceArray<node> & nodeOfFace,
        //NodeArray<face>& faceOfNode,
        EdgeArray<edge> & arcTwin);

    void findShortestPath(
        const CombinatorialEmbedding & E,
        node s, //edge startpoint
        node t, //edge endpoint
        node sDummy, //representing s in network
        node tDummy, //representing t in network
        SList<adjEntry> & crossed,
        FaceArray<node> & nodeOfFace);

    edge insertEdge(
        ClusterPlanRep & CPR,
        CombinatorialEmbedding & E,
        const NodePair & np,
        FaceArray<node> & nodeOfFace,
        EdgeArray<edge> & arcRightToLeft,
        EdgeArray<edge> & arcLeftToRight,
        EdgeArray<edge> & arcTwin,
        NodeArray<cluster> & clusterOfFaceNode,
        const SList<adjEntry> & crossed);

    void setArcStatus(
        edge eArc,
        node oSrc,
        node oTgt,
        const ClusterGraph & CG,
        NodeArray<cluster> & clusterOfFaceNode,
        EdgeArray<edge> & arcTwin);

    //use heuristics to improve the result if possible
    void postProcess();

private:

    Graph* m_originalGraph;
    Graph m_dualGraph;
    EdgeArray<int> m_eStatus; //status of dual graph arcs
    EdgeArray<adjEntry> m_arcOrig; //original edges adj entry
    PostProcessType m_ppType; //defines which kind of postprocessing to use

    //compute for every face the cluster that surrounds it
    void deriveFaceCluster(
        ClusterPlanRep & CPR,
        CombinatorialEmbedding & E,
        const ClusterGraph & CG,
        FaceArray<node> & nodeOfFace,
        NodeArray<cluster> & clusterOfFaceNode);


    //debug
    void writeDual(const char* fileName);
    void writeGML(ostream & os, const Layout & drawing);
};//class CPlanarEdgeInserter

} // end namespace ogdf


#endif
