/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class DynamicSPQRTree
 *
 * \author Jan Papenfu&szlig;
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


#ifndef OGDF_DYNAMIC_SPQR_TREE_H
#define OGDF_DYNAMIC_SPQR_TREE_H


#include <ogdf/decomposition/SPQRTree.h>
#include <ogdf/decomposition/DynamicSPQRForest.h>
#include <ogdf/decomposition/DynamicSkeleton.h>


namespace ogdf
{

//---------------------------------------------------------
// DynamicSPQRTree
// SPQR-tree data structure (dynamic environment)
//---------------------------------------------------------

/**
 * \brief Linear-time implementation of dynamic SPQR-trees.
 *
 * The class DynamicSPQRTree maintains the arrangement of the triconnected
 * components of a biconnected multi-graph \e G [Hopcroft, Tarjan 1973]
 * as a so-called SPQR tree \e T [Fi Battista, Tamassia, 1996]. We
 * call \e G the original graph of \e T.
 * The class DynamicSPQRTree supports the statical construction of
 * an SPQR-tree for a given graph G, and supports dynamic updates,
 * too.
 *
 * Each node of the tree has an associated type (represented by
 * SPQRTree::NodeType), which is either SNode, PNode, or
 * RNode, and a skeleton (represented by the class DynamicSkeleton).
 * The skeletons of the nodes of \e T are in one-to-one
 * correspondence to the triconnected components of \e G, i.e.,
 * S-nodes correspond to polygons, P-nodes to bonds, and
 * R-nodes to triconnected graphs.
 *
 * In our representation of SPQR-trees, Q-nodes are omitted. Instead,
 * the skeleton S of a node \e v in \e T contains two types of edges:
 * real edges, which correspond to edges in \e G, and virtual edges, which
 * correspond to edges in \e T having \e v as an endpoint.
 * There is a special edge \e er in G at which \e T is rooted, i.e., the
 * root node of \e T is the node whose skeleton contains the real edge
 * corresponding to \e er.
 *
 * The reference edge of the skeleton of the root node is \e er, the
 * reference edge of the skeleton \e S of a non-root node \e v is the virtual
 * edge in \e S that corresponds to the tree edge (parent(\e v),\e v).
 */
class OGDF_EXPORT DynamicSPQRTree : public virtual SPQRTree, public DynamicSPQRForest
{
public:

    friend class DynamicSkeleton;


    // constructors

    /**
     * \brief Creates an SPQR tree \e T for graph \a G rooted at the first edge of \a G.
     * \pre \a G is biconnected and contains at least 3 nodes,
     *      or \a G has exactly 2 nodes and at least 3 edges.
     */
    DynamicSPQRTree(Graph & G) : DynamicSPQRForest(G)
    {
        init(G.firstEdge());
    }

    /**
     * \brief Creates an SPQR tree \e T for graph \a G rooted at the edge \a e.
     * \pre \a e is in \a G, \a G is biconnected and contains at least 3 nodes,
     *      or \a G has exactly 2 nodes and at least 3 edges.
     */
    DynamicSPQRTree(Graph & G, edge e) : DynamicSPQRForest(G)
    {
        init(e);
    }


    // destructor

    ~DynamicSPQRTree();


    //
    // a) Access operations
    //

    //! Returns a reference to the original graph \e G.
    const Graph & originalGraph() const
    {
        return m_G;
    }

    //! Returns a reference to the tree \e T.
    const Graph & tree() const
    {
        return m_T;
    }

    //! Returns the edge of \e G at which \e T is rooted.
    edge rootEdge() const
    {
        return m_rootEdge;
    }

    //! Returns the root node of \e T.
    node rootNode() const
    {
        return findSPQR(m_bNode_SPQR[m_B.firstNode()]);
    }

    //! Returns the number of S-nodes in \e T.
    int numberOfSNodes() const
    {
        return m_bNode_numS[m_B.firstNode()];
    }

    //! Returns the number of P-nodes in \e T.
    int numberOfPNodes() const
    {
        return m_bNode_numP[m_B.firstNode()];
    }

    //! Returns the number of R-nodes in \e T.
    int numberOfRNodes() const
    {
        return m_bNode_numR[m_B.firstNode()];
    }

    /**
     * \brief Returns the type of node \a v.
     * \pre \a v is a node in \e T
     */
    NodeType typeOf(node v) const
    {
        return (NodeType)m_tNode_type[findSPQR(v)];
    }

    //! Returns the list of all nodes with type \a t.
    List<node> nodesOfType(NodeType t) const;

    //! Finds the shortest path between the two sets of vertices of \e T which \a s and \a t of \e G belong to.
    SList<node> & findPath(node s, node t)
    {
        return findPathSPQR(m_gNode_hNode[s], m_gNode_hNode[t]);
    }

    /**
     * \brief Returns the skeleton of node \a v.
     * \pre \a v is a node in \e T
     */
    Skeleton & skeleton(node v) const
    {
        v = findSPQR(v);
        if(!m_sk[v]) return createSkeleton(v);
        return *m_sk[v];
    }

    /**
     * \brief Returns the skeleton that contains the real edge \a e.
     * \pre \a e is an edge in \e G
     */
    const Skeleton & skeletonOfReal(edge e) const
    {
        return skeleton(spqrproper(m_gEdge_hEdge[e]));
    }

    /**
     * \brief Returns the skeleton edge that corresponds to the real edge \a e.
     * \pre \a e is an edge in \e G
     */
    edge copyOfReal(edge e) const
    {
        e = m_gEdge_hEdge[e];
        skeleton(spqrproper(e));
        return m_skelEdge[e];
    }

    /**
     * \brief Returns the virtual edge in the skeleton of \a w that
     * corresponds to the tree edge between \a v and \a w.
     * \pre \a v and \a w are adjacent nodes in \e T
     */
    edge skeletonEdge(node v, node w) const
    {
        edge e = virtualEdge(v, w);
        if(!e) return e;
        skeleton(w);
        return m_skelEdge[e];
    }


    //
    // b) Update operations
    //

    /**
     * \brief Roots \e T at edge \a e and returns the new root node of \e T.
     * \pre \a e is an edge in \e G
     */
    node rootTreeAt(edge e);

    /**
     * \brief Roots \e T at node \a v and returns \a v.
     * \pre \a v is a node in \e T
     */
    node rootTreeAt(node v);

    /**
     * \brief Updates the whole data structure after a new edge \a e has
     * been inserted into \e G.
     */
    edge updateInsertedEdge(edge e);

    /**
     * \brief Updates the whole data structure after a new vertex has been
     * inserted into \e G by splitting an edge into \a e and \a f.
     */
    node updateInsertedNode(edge e, edge f);


protected:

    //! Initialization (called by constructors).
    void init(edge e);

    //! Creates the skeleton graph belonging to a given vertex \a vT of \e T.
    DynamicSkeleton & createSkeleton(node vT) const;

    /**
     * \brief Recursively performs the task of adding edges (and nodes)
     * to the pertinent graph \a Gp for each involved skeleton graph.
     */
    void cpRec(node v, PertinentGraph & Gp) const
    {
        v = findSPQR(v);
        for(ListConstIterator<edge> i = m_tNode_hEdges[v].begin(); i.valid(); ++i)
        {
            edge e = m_hEdge_gEdge[*i];
            if(e) cpAddEdge(e, Gp);
            else if(*i != m_tNode_hRefEdge[v]) cpRec(spqrproper(*i), Gp);
        }
    }


    edge m_rootEdge;  //!< edge of \e G at which \e T is rooted

    mutable NodeArray<DynamicSkeleton*> m_sk;        //!< pointer to skeleton of a node in \e T
    mutable EdgeArray<edge>             m_skelEdge;  //!< copies of real and virtual edges in their skeleton graphs (invalid, if the skeleton does not actually exist)
    mutable NodeArray<node>             m_mapV;      //!< temporary array used by \e createSkeleton()

}; // class DynamicSPQRTree


} // end namespace ogdf


#endif
