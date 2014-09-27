/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class FaceSinkGraph
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

#ifndef OGDF_FACE_SINK_GRAPH_H
#define OGDF_FACE_SINK_GRAPH_H


#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/FaceArray.h>
#include <ogdf/basic/SList.h>


namespace ogdf
{


class OGDF_EXPORT FaceSinkGraph : public Graph
{
public:
    //! constructor (we assume that the original graph is connected!)
    FaceSinkGraph(const ConstCombinatorialEmbedding & E, node s);

    //! default constructor (dummy)
    FaceSinkGraph() : m_pE(0) { }


    void init(const ConstCombinatorialEmbedding & E, node s);


    //! return a reference to the original graph G
    const Graph & originalGraph() const
    {
        return *m_pE;
    }

    //! returns a reference to the embedding E of the original graph G
    const ConstCombinatorialEmbedding & originalEmbedding() const
    {
        return *m_pE;
    }

    //! returns the sink-switch in G corresponding to node v in the face-sink
    //! graph, 0 if v corresponds to a face
    node originalNode(node v) const
    {
        return m_originalNode[v];
    }

    //! returns the face in E corresponding to node v in the face-sink
    //! graph, 0 if v corresponds to a sink-switch
    face originalFace(node v) const
    {
        return m_originalFace[v];
    }

    // returns true iff node v in the face-sink graph corresponds to a
    // face in E containing the source
    bool containsSource(node v) const
    {
        return m_containsSource[v];
    }




    //! returns the list of faces f in E such that there exists an upward-planar
    //! drawing realizing E with f as external face
    //! a node v_T in tree T is returned as representative. v_T is 0 if no possible external face exists.
    node possibleExternalFaces(SList<face> & externalFaces)
    {
        node v_T = checkForest();
        if(v_T != 0)
            gatherExternalFaces(m_T, 0, externalFaces);
        return v_T;
    }


    node faceNodeOf(edge e)
    {
        return dfsFaceNodeOf(m_T, 0,
                             m_pE->rightFace(e->adjSource()), m_pE->rightFace(e->adjTarget()));
    }


    node faceNodeOf(face f)
    {
        return dfsFaceNodeOf(m_T, 0, f, 0);
    }


    //! augments G to an st-planar graph (original implementation)
    /** introduces also new nodes into G corresponding to face-nodes in face sink graph)
     */
    void stAugmentation(
        node h,                       // node corresponding to external face
        Graph & G,                    // original graph (not const)
        SList<node> & augmentedNodes, // list of augmented nodes
        SList<edge> & augmentedEdges); // list of augmented edges

    //! augments G to an st-planar graph
    /** (introduces only one new node as super sink into G)
     */
    void stAugmentation(
        node  h,                      // node corresponding to external face
        Graph & G,                    // original graph (not const)
        node & superSink,             // super sink
        SList<edge> & augmentedEdges); // list of augmented edges

    //! compute the sink switches of all faces.
    // the ext. face muss be set
    void sinkSwitches(FaceArray< List<adjEntry> > & faceSwitches);



private:
    //! constructs face-sink graph
    void doInit();

    //! performs dfs-traversal and checks for backwards edges
    bool dfsCheckForest(
        node v,                   // current node
        node parent,              // its parent in tree
        NodeArray<bool> & visited, // not already visited ?
        // number of internal vertices of G in current tree
        int & nInternalVertices);

    //! builds list of possible external faces
    /** all faces in tree T containing
     * the single source s) by a dfs traversal of T
     */
    void gatherExternalFaces(
        node v,                      // current node
        node parent,                 // its parent
        SList<face> & externalFaces); // returns list of possible external faces

    node dfsFaceNodeOf(node v, node parent, face f1, face f2);

    node dfsStAugmentation(
        node v,                       // current node
        node parent,                  // its parent
        Graph & G,                    // original graph (not const)
        SList<node> & augmentedNodes, // list of augmented nodes
        SList<edge> & augmentedEdges); // list of augmented edges

    node dfsStAugmentation(
        node v,                       // current node
        node parent,                  // its parent
        Graph & G,                    // original graph (not const)
        SList<edge> & augmentedEdges); // list of augmented edges


    //! associated embedding of graph G
    const ConstCombinatorialEmbedding* m_pE;
    node m_source; //!< the single source
    node m_T;      //!< representative of unique tree T

    NodeArray<node> m_originalNode;   //!< original node in G
    NodeArray<face> m_originalFace;   //!< original face in E
    NodeArray<bool> m_containsSource; //!< contains face node the source ?

    /*
    //! traverse the face sink tree and compute the sink witches of each internal faces
    void dfsFST(node v, //current node
        node parent, //parent of v
        FaceArray< List<adjEntry> > &faceSwitches,
        NodeArray<bool> &visited);
        */

    //! checks if the face-sink graph is a forest with
    //!  1) there is exactly one tree T containing no internal vertex of G
    //!  2) all other trees contain exactly one internal vertex of G
    //! a node in tree T is returned as representative
    node checkForest();


    //! return a adjEntry of node v which right face is f. Be Carefully! The adjEntry is not always unique.
    adjEntry getAdjEntry(node v, face f);


}; // class FaceSinkGraph


} // end namespace ogdf


#endif
