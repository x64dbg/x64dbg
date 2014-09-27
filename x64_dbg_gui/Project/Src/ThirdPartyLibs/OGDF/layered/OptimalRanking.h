/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of optimal ranking algorithm for Sugiyama
 *        algorithm.
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

#ifndef OGDF_OPTIMAL_RANKING_H
#define OGDF_OPTIMAL_RANKING_H



#include <ogdf/module/RankingModule.h>
#include <ogdf/module/AcyclicSubgraphModule.h>
#include <ogdf/basic/ModuleOption.h>
#include <ogdf/basic/NodeArray.h>


namespace ogdf
{

//! The optimal ranking algorithm.
/**
 * The class OptimalRanking implements the LP-based algorithm for computing
 * a node ranking with minimal edge lengths, which can be used as first phase
 * in SugiyamaLayout.
 *
 * <H3>Optional parameters</H3>
 * The following options affect the crossing minimization step
 * of the algorithm:
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>separateMultiEdges</i><td>bool<td>true
 *     <td>If set to true, multi-edges will span at least two layers.
 *   </tr>
 * </table>
 *
 * <H3>%Module options</H3>
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>subgraph</i><td>AcyclicSubgraphModule<td>DfsAcyclicSubgraph
 *     <td>The module for the computation of the acyclic subgraph.
 *   </tr>
 * </table>
 */
class OGDF_EXPORT OptimalRanking : public RankingModule
{

    ModuleOption<AcyclicSubgraphModule> m_subgraph; // option for acyclic sugraph
    bool m_separateMultiEdges;

public:
    //! Creates an instance of optimal ranking.
    OptimalRanking();


    /**
     *  @name Algorithm call
     *  @{
     */

    //! Computes a node ranking of \a G in \a rank.
    void call(const Graph & G, NodeArray<int> & rank);

    //! Computes a node ranking of \a G with given minimal edge length in \a rank.
    /**
     * @param G is the input graph.
     * @param length specifies the minimal length of each edge.
     * @param rank is assigned the rank (layer) of each node.
     */
    void call(const Graph & G, const EdgeArray<int> & length, NodeArray<int> & rank);

    //! Computes a cost-minimal node ranking of \a G for given edge costs and minimal edge lengths in \a rank.
    /**
     * @param G is the input graph.
     * @param length specifies the minimal length of each edge.
     * @param cost specifies the cost of each edge.
     * @param rank is assigned the rank (layer) of each node.
     */
    void call(
        const Graph & G,
        const EdgeArray<int> & length,
        const EdgeArray<int> & cost,
        NodeArray<int> & rank);


    /** @}
     *  @name Optional parameters
     *  @{
     */

    //! Returns the current setting of option separateMultiEdges.
    /**
     * If set to true, multi-edges will span at least two layers. Since
     * each such edge will have at least one dummy node, the edges will
     * automaticall be separated in a Sugiyama drawing.
     */
    bool separateMultiEdges() const
    {
        return m_separateMultiEdges;
    }

    //! Sets the option separateMultiEdges to \a b.
    void separateMultiEdges(bool b)
    {
        m_separateMultiEdges = b;
    }


    /** @}
     *  @name Module options
     *  @{
     */

    //! Sets the module for the computation of the acyclic subgraph.
    void setSubgraph(AcyclicSubgraphModule* pSubgraph)
    {
        m_subgraph.set(pSubgraph);
    }

    //! @}

private:
    //! Implements the algorithm call.
    void doCall(const Graph & G,
                NodeArray<int> & rank,
                EdgeArray<bool> & reversed,
                const EdgeArray<int> & length,
                const EdgeArray<int> & cost);
};


} // end namespace ogdf


#endif
