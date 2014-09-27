/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class UpwardPlanarModule, which implements
 *        the upward-planarity testing and embedding algorithm for
 *        single-source digraphs by Bertolazzi et al.
 *
 * \author Carsten Gutwenger
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

#ifndef OGDF_UPWARD_PLANAR_MODULE_H
#define OGDF_UPWARD_PLANAR_MODULE_H


#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/SList.h>


namespace ogdf
{

class OGDF_EXPORT SPQRTree;
class OGDF_EXPORT Skeleton;
class OGDF_EXPORT StaticPlanarSPQRTree;
class OGDF_EXPORT ExpansionGraph;
class OGDF_EXPORT FaceSinkGraph;

//---------------------------------------------------------
// UpwardPlanarModule
//---------------------------------------------------------
class OGDF_EXPORT UpwardPlanarModule
{
public:
    // constructor
    UpwardPlanarModule() { }


    //------------------------------
    // general single-source graphs

    // tests if single-source digraph G is upward planar
    bool upwardPlanarityTest(Graph & G)
    {
        NodeArray<SListPure<adjEntry> > adjacentEdges;
        return doUpwardPlanarityTest(G, false, adjacentEdges);
    }

    // tests if single-source digraph G is upward planar and, if true,
    // constructs an upward-planar embeding of G
    bool upwardPlanarEmbed(Graph & G)
    {
        NodeArray<SListPure<adjEntry> > adjacentEdges(G);
        if(doUpwardPlanarityTest(G, true, adjacentEdges) == false)
            return false;
        SList<node> augmentedNodes;
        SList<edge> augmentedEdges;
        doUpwardPlanarityEmbed(G, adjacentEdges, false, augmentedNodes, augmentedEdges);
        return true;
    }

    // tests if single-source digraph G is upward planar and, if true,
    // augments G to a planar st-digraph
    bool upwardPlanarAugment(Graph & G,
                             SList<node> & augmentedNodes,
                             SList<edge> & augmentedEdges)
    {
        NodeArray<SListPure<adjEntry> > adjacentEdges(G);
        if(doUpwardPlanarityTest(G, true, adjacentEdges) == false)
            return false;
        doUpwardPlanarityEmbed(G, adjacentEdges, true, augmentedNodes, augmentedEdges);
        return true;
    }

    // tests if single-source digraph G is upward planar and, if true,
    // augments G to a planar st-digraph
    bool upwardPlanarAugment(Graph & G,
                             node & superSink,
                             SList<edge> & augmentedEdges)
    {
        NodeArray<SListPure<adjEntry> > adjacentEdges(G);
        if(doUpwardPlanarityTest(G, true, adjacentEdges) == false)
            return false;
        doUpwardPlanarityEmbed(G, adjacentEdges, true, superSink, augmentedEdges);
        return true;
    }

    // tests if single-source digraph G is upward planar and, if true,
    // augments G to a planar st-digraph
    bool upwardPlanarAugment(Graph & G)
    {
        node superSink;
        SList<edge> augmentedEdges;
        return upwardPlanarAugment(G, superSink, augmentedEdges);
    }


    // --------------------------------
    // embedded single-source digraphs

    // tests if embedded single-source digraph G can be drawn upward-planar
    // realizing the given embedding and returns in externalFaces the set
    // of faces which can be choosen as external face
    bool testEmbeddedBiconnected(
        const Graph & G,                      // embedded input graph
        const ConstCombinatorialEmbedding & E, // embedding
        SList<face> & externalFaces);         // possible external faces

    // tests if embedded single-source digraph G can be drawn upward-planar
    // realizing the given embedding and augments G to a planar st-digraph
    bool testAndAugmentEmbedded(
        Graph & G,                   // embedded input graph
        SList<node> & augmentedNodes, // augmented nodes
        SList<edge> & augmentedEdges); // augmented edges

    bool testAndAugmentEmbedded(
        Graph & G,                   // embedded input graph
        node & superSink,            // super sink
        SList<edge> & augmentedEdges); // augmented edges


private:

    struct DegreeInfo
    {
        int m_indegSrc;
        int m_outdegSrc;
        int m_indegTgt;
        int m_outdegTgt;
    };

    // classes defined and used in UpwardPlanarModule.cpp
    class OGDF_EXPORT SkeletonInfo;
    class ConstraintRooting;


    // returns the single-source if present, 0 otherwise
    node getSingleSource(const Graph & G);

    //-----------------------------------------------------------
    // the following functions perform actual testing, embedding,
    // and augmenting

    // test and compute adjacency lists of embedding
    bool doUpwardPlanarityTest(
        Graph & G,
        bool  embed,
        NodeArray<SListPure<adjEntry> > & adjacentEdges);

    // embed and compute st-augmentation (original implementation - inserts
    // also new nodes corresponding to faces into G)
    void doUpwardPlanarityEmbed(
        Graph & G,
        NodeArray<SListPure<adjEntry> > & adjacentEdges,
        bool augment,
        SList<node> & augmentedNodes,
        SList<edge> & augmentedEdges);

    // embed and compute st-augmentation (new implementation - inserts only
    // one new node into G which is the super sink)
    void doUpwardPlanarityEmbed(
        Graph & G,
        NodeArray<SListPure<adjEntry> > & adjacentEdges,
        bool augment,
        node & superSink,
        SList<edge> & augmentedEdges);

    // performs the actual test (and computation of sorted adjacency lists) for
    // each biconnected component
    bool testBiconnectedComponent(
        ExpansionGraph & exp,
        node sG,
        int parentBlock,
        bool embed,
        NodeArray<SListPure<adjEntry> > & adjacentEdges);


    //-------------------------------
    // computatation of st-skeletons

    // compute sT-skeletons
    // test for upward-planarity, build constraints for rooting, and find a
    // rooting of the tree satisfying all constraints
    // returns true iff such a rooting exists
    edge directSkeletons(
        SPQRTree & T,
        NodeArray<SkeletonInfo> & skInfo);

    // precompute information: in-/outdegrees in pertinent graph, contains
    // pertinent graph the source?
    void computeDegreesInPertinent(
        const SPQRTree & T,
        node s,
        NodeArray<SkeletonInfo> & skInfo,
        node vT);


    //------------------------
    // embedding of skeletons

    bool initFaceSinkGraph(const Graph & M, SkeletonInfo & skInfo);

    void embedSkeleton(
        Graph & G,
        StaticPlanarSPQRTree & T,
        NodeArray<SkeletonInfo> & skInfo,
        node vT,
        bool extFaceIsLeft);


    //--------------------------
    // assigning sinks to faces

    void assignSinks(
        FaceSinkGraph & F,
        face extFace,
        NodeArray<face> & assignedFace);

    node dfsAssignSinks(
        FaceSinkGraph & F,
        node v,                      // current node
        node parent,                 // its parent
        NodeArray<face> & assignedFace);


    //------------------------------
    // for testing / debugging only

    bool checkDegrees(
        SPQRTree & T,
        node s,
        NodeArray<SkeletonInfo> & skInfo);

    bool virtualEdgesDirectedEqually(const SPQRTree & T);



}; // class UpwardPlanarModule


} // end namespace ogdf


#endif
