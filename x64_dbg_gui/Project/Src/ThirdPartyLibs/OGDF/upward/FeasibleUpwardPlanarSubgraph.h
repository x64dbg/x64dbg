/*
 * $Revision: 2566 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 23:10:08 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class FeasibleUpwardPlanarSubgraph which
 *        computes an feasible upward planar subgraph and a feasible upward embedding.
 *
 * \author Hoi-Ming Wong
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


#ifndef OGDF_FEASIBLE_UPWARD_PLANAR_SUBGRAPH_H
#define OGDF_FEASIBLE_UPWARD_PLANAR_SUBGRAPH_H



#include <ogdf/basic/Module.h>
#include <ogdf/basic/GraphCopy.h>


namespace ogdf
{


class OGDF_EXPORT FeasibleUpwardPlanarSubgraph : public Module
{
public:
    // construction
    FeasibleUpwardPlanarSubgraph() { }
    // destruction
    ~FeasibleUpwardPlanarSubgraph() { }

    // Computes a feasible upward planar subgraph fups with feasible a
    // embedding gamma.
    ReturnType call(
        Graph & G, // connected single source graph
        GraphCopy & Fups, // the feasible upward planar subgraph
        adjEntry & extFaceHandle, // the right face of this adjEntry is the ext. face of the embedded fups
        List<edge> & delEdges, // the list of deleted edges (original edges)
        bool multisources,  // true, if the original input graph has multi sources
        // and G is an tranformed single source graph (by introducing a super source)
        int runs); // number of runs

    // Computes a feasible upward planar subgraph fups with feasible a
    // embedding gamma.
    ReturnType call(
        const Graph & G,
        GraphCopy & Fups,
        adjEntry & extFaceHandle,
        List<edge> & delEdges,
        bool multisources);

    // construct a merge graph with repsect to gamma and its test acyclicity
    bool constructMergeGraph(
        GraphCopy & M, // copy of the original graph, muss be embedded
        adjEntry adj_orig, // the adjEntry of the original graph, which right face is the ext. Face and adj->theNode() is the source
        const List<edge> & del_orig); // deleted edges


    //! return a adjEntry of node v which right face is f. Be Carefully! The adjEntry is not always unique.
    adjEntry getAdjEntry(const CombinatorialEmbedding & Gamma, node v, face f)
    {
        adjEntry adj = 0;
        forall_adj(adj, v)
        {
            if(Gamma.rightFace(adj) == f)
                break;
        }

        OGDF_ASSERT(Gamma.rightFace(adj) == f);

        return adj;
    }

private:

    //! Compute a (random) span tree of the input sT-Graph.
    /*
     * @param GC The input graph.
     * @param &delEdges The deleted edges (original edges).
     * @param random compute a random span tree
     * @multisource true, if the original graph got multisources. In this case, the incident edges of
     *  the source are allways included in the span tree
     */
    void getSpanTree(GraphCopy & GC, List<edge> & delEdges, bool random, bool multisources);

    /*
     * Function use by geSpannTree to compute the spannig tree.
     */
    void dfs_visit(const Graph & G, edge e, NodeArray<bool> & visited, EdgeArray<bool> & treeEdges, bool random);




};


} // end namespace ogdf

#endif
