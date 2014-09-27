/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of the classes BoyerMyrvoldInit and BucketLowPoint
 *
 * \author Jens Schmidt
 *
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

#ifndef OGDF_BOYER_MYRVOLD_INIT_H
#define OGDF_BOYER_MYRVOLD_INIT_H


#include <ogdf/internal/planarity/BoyerMyrvoldPlanar.h>
#include <ogdf/basic/List.h>


namespace ogdf
{

//! This class is used in the Boyer-Myrvold planarity test for preprocessing purposes.
/**
* Among these is the computation of lowpoints, highestSubtreeDFIs,
* separatedDFSChildList and of course building the DFS-tree.
*/
class BoyerMyrvoldInit
{
public:
    //! Constructor, the parameter BoyerMyrvoldPlanar is needed
    BoyerMyrvoldInit(BoyerMyrvoldPlanar* pBM);

    //! Destructor
    ~BoyerMyrvoldInit() { }

    //! Creates the DFSTree
    void computeDFS();

    //! Computes lowpoint, highestSubtreeDFI and links virtual to nonvirtual vertices
    void computeLowPoints();

    //! Computes the list of separated DFS children for all nodes
    void computeDFSChildLists();

    // avoid automatic creation of assignment operator
    //! Assignment operator is undefined!
    BoyerMyrvoldInit & operator=(const BoyerMyrvoldInit &);

private:
    //! The input graph
    Graph & m_g;

    //! Some parameters... see BoyerMyrvold.h for further instructions
    const int & m_embeddingGrade;
    const bool & m_randomDFSTree;

    //! Link to non-virtual vertex of a virtual Vertex.
    /** A virtual vertex has negative DFI of the DFS-Child of related non-virtual Vertex
     */
    NodeArray<node> & m_realVertex;

    //! The one and only DFI-Array
    NodeArray<int> & m_dfi;

    //! Returns appropriate node from given DFI
    Array<node> & m_nodeFromDFI;

    //! Links to opposite adjacency entries on external face in clockwise resp. ccw order
    /** m_link[0]=CCW, m_link[1]=CW
     */
    NodeArray<adjEntry> (&m_link)[2];

    //! The adjEntry which goes from DFS-parent to current vertex
    NodeArray<adjEntry> & m_adjParent;

    //! The DFI of the least ancestor node over all backedges
    /** If no backedge exists, the least ancestor is the DFI of that node itself
     */
    NodeArray<int> & m_leastAncestor;

    //! Contains the type of each \a edge
    /** @param 0 = EDGE_UNDEFINED
     * @param 1 = EDGE_SELFLOOP
     * @param 2 = EDGE_BACK
     * @param 3 = EDGE_DFS
     * @param 4 = EDGE_DFS_PARALLEL
     * @param 5 = EDGE_BACK_DELETED
     */
    EdgeArray<int> & m_edgeType;

    //! The lowpoint of each \a node
    NodeArray<int> & m_lowPoint;

    //! The highest DFI in a subtree with \a node as root
    NodeArray<int> & m_highestSubtreeDFI;

    //! A list to all separated DFS-children of \a node
    /** The list is sorted by lowpoint values (in linear time)
    */
    NodeArray<ListPure<node> > & m_separatedDFSChildList;

    //! Pointer to \a node contained in the DFSChildList of his parent, if exists.
    /** If node isn't in list or list doesn't exist, the pointer is set to NULL.
    */
    NodeArray<ListIterator<node> > & m_pNodeInParent;

    //! Creates and links a virtual vertex of the node belonging to \a father
    void createVirtualVertex(const adjEntry father);
};

//! BucketFunction for lowPoint buckets
/** Parameter lowPoint may not be deleted til destruction of this class.
*/
class BucketLowPoint : public BucketFunc<node>
{
public:
    BucketLowPoint(const NodeArray<int> & lowPoint) : m_pLow(&lowPoint) { }

    //! This function has to be derived from BucketFunc, it gets the buckets from lowPoint-Array
    int getBucket(const node & v)
    {
        return (*m_pLow)[v];
    }
private:
    //! Stored to be able to get the buckets
    const NodeArray<int>* m_pLow;
};

}


#endif
