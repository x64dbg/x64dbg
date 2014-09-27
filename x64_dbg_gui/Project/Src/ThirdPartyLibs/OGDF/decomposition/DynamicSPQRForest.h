/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class DynamicSPQRForest
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

#ifndef OGDF_DYNAMIC_SPQR_FOREST_H
#define OGDF_DYNAMIC_SPQR_FOREST_H

#include <ogdf/decomposition/DynamicBCTree.h>
#include <ogdf/decomposition/SPQRTree.h>

namespace ogdf
{

/**
 * \brief Dynamic SPQR-forest.
 *
 * This class is an extension of DynamicBCTree.\n
 * It provides a set of SPQR-trees for each B-component of a BC-tree.
 * These SPQR-trees are dynamic, i.e. there are member functions for
 * dynamic updates (edge insertion and node insertion).
 */
class OGDF_EXPORT DynamicSPQRForest : public DynamicBCTree
{

public:

    /** \enum TNodeType
     * \brief Enumeration type for characterizing the SPQR-tree-vertices.
     */
    /** \var TNodeType ogdf::DynamicSPQRForest::SComp
     * denotes a vertex representing an S-component.
     */
    /** \var TNodeType ogdf::DynamicSPQRForest::PComp
     * denotes a vertex representing a P-component.
     */
    /** \var TNodeType ogdf::DynamicSPQRForest::RComp
     * denotes a vertex representing an R-component.
     */
    enum TNodeType
    {
        SComp = SPQRTree::SNode,
        PComp = SPQRTree::PNode,
        RComp = SPQRTree::RNode
    };

protected:

    /**
     * \brief A \e Graph structure containing all SPQR-trees.
     */
    mutable Graph m_T;

    /** @{
     * \brief The root vertices of the SPQR-trees.
     *
     * For each vertex of the BC-tree representing a B-component, this
     * array contains the root vertex of the respective SPQR-tree, or
     * \e NULL, if the SPQR-tree does not exist.
     */
    mutable NodeArray<node> m_bNode_SPQR;
    /**
     * \brief The numbers of S-components.
     *
     * For each vertex of the BC-tree representing a B-component,
     * this array contains the number of S-components of the respective
     * SPQR-tree. If the SPQR-tree does not exist, then the array member
     * is undefined.
     */
    mutable NodeArray<int> m_bNode_numS;
    /**
     * \brief The numbers of P-components.
     *
     * For each vertex of the BC-tree representing a B-component,
     * this array contains the number of P-components of the respective
     * SPQR-tree. If the SPQR-tree does not exist, then the array member
     * is undefined.
     */
    mutable NodeArray<int> m_bNode_numP;
    /**
     * \brief The numbers of R-components.
     *
     * For each vertex of the BC-tree representing a B-component,
     * this array contains the number of R-components of the respective
     * SPQR-tree. If the SPQR-tree does not exist, then the array member
     * is undefined.
     */
    mutable NodeArray<int> m_bNode_numR;

    /** @} @{
     * \brief The types of the SPQR-tree-vertices.
     */
    mutable NodeArray<TNodeType> m_tNode_type;
    /**
     * \brief The owners of the SPQR-tree-vertices in the UNION/FIND
     * structure.
     */
    mutable NodeArray<node> m_tNode_owner;
    /**
     * \brief The virtual edges leading to the parents of the
     * SPQR-tree-vertices.
     */
    mutable NodeArray<edge> m_tNode_hRefEdge;
    /**
     * \brief Lists of real and virtual edges belonging to
     * SPQR-tree-vertices.
     */
    mutable NodeArray<List<edge> > m_tNode_hEdges;

    /** @} @{
     * \brief The positions of real and virtual edges in their
     * \e m_tNode_hEdges lists.
     */
    mutable EdgeArray<ListIterator<edge> > m_hEdge_position;
    /**
     * \brief The SPQR-tree-vertices which the real and virtual edges
     * are belonging to.
     */
    mutable EdgeArray<node> m_hEdge_tNode;
    /**
     * \brief The partners of virtual edges (\e NULL if real).
     */
    mutable EdgeArray<edge> m_hEdge_twinEdge;

    /** @} @{
     * \brief Auxiliary array used by \e createSPQR().
     */
    mutable NodeArray<node> m_htogc;
    /**
     * \brief Auxiliary array used by \e findNCASPQR()
     */
    mutable NodeArray<bool> m_tNode_isMarked;

    /** @}
     * \brief Initialization.
     */
    void init();
    /**
     * \brief creates the SPQR-tree for a given B-component of the
     * BC-tree.
     *
     * An SPQR-tree belonging to a B-component of the BC-tree is only
     * created on demand, i.e. this member function is only called by
     * findSPQRTree() and - under certain circumstances - by
     * updateInsertedEdge().
     * \param vB is a vertex of the BC-tree representing a B-component.
     * \pre \a vB has to be the proper representative of its B-component,
     * i.e. it has to be the root vertex of its respective
     * UNION/FIND-tree.
     * \pre The B-component represented by \a vB must contain at least
     * 3 edges.
     */
    void createSPQR(node vB) const;

    /** @{
     * \brief unites two SPQR-tree-vertices (UNION part of UNION/FIND).
     * \param vB is a vertex of the BC-tree representing a B-component.
     * \param sT is a vertex of the SPQR-tree belonging to \a vB.
     * \param tT is a vertex of the SPQR-tree belonging to \a vB.
     * \pre \a vB has to be the proper representative of its B-component,
     * i.e. it has to be the root vertex of its respective
     * UNION/FIND-tree.
     * \pre \a sT and \a tT have to be proper representatives of their
     * triconnected components, i.e. they have to be the root vertices of
     * their respective UNION/FIND-trees.
     * \return the proper representative of the united SPQR-tree-vertex.
     */
    node uniteSPQR(node vB, node sT, node tT);
    /**
     * \brief finds the proper representative of an SPQR-tree-vertex (FIND
     * part of UNION/FIND).
     * \param vT is any vertex of \e m_T.
     * \return the owner of \a vT properly representing a triconnected
     * component, i.e. the root of the UNION/FIND-tree of \a vT.
     */
    node findSPQR(node vT) const;

    /** @} @{
     * \brief finds the nearest common ancestor of \a sT and \a tT.
     * \param sT is a vertex of an SPQR-tree.
     * \param tT is a vertex of an SPQR-tree.
     * \pre \a sT and \a tT must belong to the same SPQR-tree.
     * \pre \a sT and \a tT have to be proper representatives of their
     * triconnected components, i.e. they have to be the root vertices of
     * their respective UNION/FIND-trees.
     * \return the proper representative of the nearest common ancestor of
     * \a sT and \a tT.
     */
    node findNCASPQR(node sT, node tT) const;
    /**
     * \brief finds the shortest path between the two sets of
     * SPQR-tree-vertices which \a sH and \a tH are belonging to.
     * \param sH is a vertex of \e m_H.
     * \param tH is a vertex of \e m_H.
     * \param rT <b>is a reference!</b> It is set to the very vertex of
     * the found path which is nearest to the root vertex of the SPQR-tree.
     * \pre \a sH and \a tH must belong to the same B-component, i.e. to
     * the same SPQR-tree. This SPQR-tree must exist!
     * \return the path in the SPQR-tree as a linear list of vertices.
     * \post <b>The SList<node> instance is created by this function and
     * has to be destructed by the caller!</b>
     */
    SList<node> & findPathSPQR(node sH, node tH, node & rT) const;

    /** @} @{
     * \brief updates an SPQR-tree after a new edge has been inserted into
     * the original graph.
     * \param vB is a BC-tree-vertex representing a B-component. The
     * SPQR-tree, which is to be updated is identified by it.
     * \param eG is a new edge in the original graph.
     * \pre \a vB has to be the proper representative of its B-component,
     * i.e. it has to be the root vertex of its respective
     * UNION/FIND-tree.
     * \pre Both the source and the target vertices of \a eG must belong
     * to the same B-component represented by \a vB.
     * \return the new edge of the original graph.
     */
    edge updateInsertedEdgeSPQR(node vB, edge eG);
    /**
     * \brief updates an SPQR-tree after a new vertex has been inserted
     * into the original graph by splitting an edge into \a eG and \a fG.
     * \param vB is a BC-tree-vertex representing a B-component. The
     * SPQR-tree, which is to be updated is identified by it.
     * \param eG is the incoming edge of the newly inserted vertex which
     * has been generated by a Graph::split() operation.
     * \param fG is the outgoing edge of the newly inserted vertex which
     * has been generated by a Graph::split() operation.
     * \pre The split edge must belong to the B-component which is
     * represented by \a vB.
     * \return the new vertex of the original graph.
     */
    node updateInsertedNodeSPQR(node vB, edge eG, edge fG);

public:

    /** @} @{
     * \brief A constructor.
     *
     * This constructor does only create the dynamic BC-tree rooted at the first
     * edge of \a G. The data structure is prepared for dealing with SPQR-trees,
     * but they will only be created on demand. Cf. member functions findPathSPQR()
     * and updateInsertedEdge().
     * \param G is the original graph.
     */
    DynamicSPQRForest(Graph & G) : DynamicBCTree(G)
    {
        init();
    }

    /** @} @{
     * \brief finds the proper representative of the SPQR-tree-vertex which
     * a given real or virtual edge is belonging to.
     *
     * This member function has to be used carefully (see <b>Precondition</b>)!
     * \param eH is an edge of \e m_H.
     * \pre The respective SPQR-tree belonging to the B-component represented by
     * the BC-tree-vertex bcproper(\a eH) must exist! Notice, that this condition
     * is fulfilled, if \a eH is a member of a list gained by the hEdgesSPQR()
     * member function, because that member function needs an SPQR-tree-vertex as
     * parameter, which might have been found (and eventually created) by the
     * findPathSPQR() member function.
     * \return the proper representative of the SPQR-tree-vertex which \a eH
     * is belonging to.
     */
    node spqrproper(edge eH) const
    {
        return m_hEdge_tNode[eH] = findSPQR(m_hEdge_tNode[eH]);
    }
    /**
     * \brief returns the twin edge of a given edge of \e m_H, if it is
     * virtual, or \e NULL, if it is real.
     * \param eH is an edge of \e m_H.
     * \return the twin edge of \a eH, if it is virtual, or \e NULL, if it
     * is real.
     */
    edge twinEdge(edge eH) const
    {
        return m_hEdge_twinEdge[eH];
    }

    /** @} @{
     * \brief returns the type of the triconnected component represented by
     * a given SPQR-tree-vertex.
     * \param vT is a vertex of an SPQR-tree.
     * \pre \a vT has to be the proper representative of its triconnected
     * component, i.e. it has to be the root vertex of its respective
     * UNION/FIND-tree. This condition is particularly fulfilled if \a vT
     * is a member of a list gained by the findPathSPQR() member function.
     * \return the type of the triconnected component represented by \a vT.
     */
    TNodeType typeOfTNode(node vT) const
    {
        return m_tNode_type[vT];
    }
    /**
     * \brief returns a linear list of the edges in \e m_H belonging to
     * the triconnected component represented by a given SPQR-tree-vertex.
     * \param vT is a vertex of an SPQR-tree.
     * \pre \a vT has to be the proper representative of its triconnected
     * component, i.e. it has to be the root vertex of its respective
     * UNION/FIND-tree. This condition is particularly fulfilled if \a vT
     * is a member of a list gained by the findPathSPQR() member function.
     * \return a linear list of the edges in \e m_H belonging to the
     * triconnected component represented by \a vT.
     */
    const List<edge> & hEdgesSPQR(node vT) const
    {
        return m_tNode_hEdges[vT];
    }
    /**
     * \brief finds the shortest path between the two sets of
     * SPQR-tree-vertices which \a sH and \a tH are belonging to.
     * \param sH is a vertex of \e m_H.
     * \param tH is a vertex of \e m_H.
     * \pre \a sH and \a tH must belong to the same B-component, i.e. to
     * the same SPQR-tree. This SPQR-tree does not need to exist. If it
     * it does not exist, it will be created.
     * \return the path in the SPQR-tree as a linear list of vertices.
     * \post <b>The SList<node> instance is created by this function and
     * has to be destructed by the caller!</b>
     */
    SList<node> & findPathSPQR(node sH, node tH) const;
    /**
     * \brief returns the virtual edge which leads from one vertex of an
     * SPQR-tree to another one.
     * \param vT is a vertex of an SPQR-tree.
     * \param wT is a vertex of an SPQR-tree.
     * \pre \a vT and \a wT must belong to the same SPQR-tree and must be
     * adjacent.
     * \pre \a vT and \a wT have to be proper representatives of their
     * triconnected components, i.e. they have to be the root vertices of
     * their respective UNION/FIND-trees. This condition is particularly
     * fulfilled if \a vT and \a wT are members of a list gained by the
     * findPathSPQR() member function.
     * \return the virtual edge in \e m_H which belongs to \a wT and
     * leads to \a vT.
     */
    edge virtualEdge(node vT, node wT) const;

    /** @} @{
     * \brief updates the whole data structure after a new edge has been
     * inserted into the original graph.
     *
     * This member function generally updates both BC- and SPQR-trees. If
     * any SPQR-tree of the B-components of the insertion path through
     * the BC-tree exists, the SPQR-tree data structure of the resulting
     * B-component will be valid afterwards. If none of the SPQR-trees
     * does exist in advance, then only the BC-tree is updated and no
     * SPQR-tree is created.
     * \param eG is a new edge in the original graph.
     * \return the new edge of the original graph.
     */
    edge updateInsertedEdge(edge eG);
    /**
     * \brief updates the whole data structure after a new vertex has been
     * inserted into the original graph by splitting an edge into \a eG
     * and \a fG.
     *
     * This member function updates the BC-tree at first. If the SPQR-tree
     * of the B-component which the split edge is belonging to does exist,
     * then it is updated, too. If it does not exist, it is not created.
     * \param eG is the incoming edge of the newly inserted vertex which
     * has been generated by a Graph::split() operation.
     * \param fG is the outgoing edge of the newly inserted vertex which
     * has been generated by a Graph::split() operation.
     * \return the new vertex of the original graph.
     */
    node updateInsertedNode(edge eG, edge fG);

    /** @}
     */
};

}

#endif
