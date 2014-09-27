/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of hierachrical ranking algorithm
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

#ifndef OGDF_LONGEST_PATH_RANKING_H
#define OGDF_LONGEST_PATH_RANKING_H



#include <ogdf/module/RankingModule.h>
#include <ogdf/module/AcyclicSubgraphModule.h>
#include <ogdf/basic/ModuleOption.h>
#include <ogdf/basic/SList.h>
#include <ogdf/basic/tuples.h>
#include <ogdf/basic/NodeArray.h>


namespace ogdf
{

class GraphCopySimple;
class GraphAttributes;


//! The longest-path ranking algorithm.
/**
 * The class LongestPathRanking implements the well-known longest-path
 * ranking algorithm, which can be used as first phase in SugiyamaLayout.
 * The implementation contains a special optimization for reducing
 * edge lengths, as well as special treatment of mixed-upward graphs
 * (e.g., UML class diagrams).
 *
 * <H3>Optional parameters</H3>
 * The following options affect the crossing minimization step
 * of the algorithm:
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>separateDeg0Layer</i><td>bool<td>true
 *     <td>If set to true, isolated nodes are placed on a separate layer.
 *   </tr><tr>
 *     <td><i>separateMultiEdges</i><td>bool<td>true
 *     <td>If set to true, multi-edges will span at least two layers.
 *   </tr><tr>
 *     <td><i>optimizeEdgeLength</i><td>bool<td>true
 *     <td>If set to true the ranking algorithm tries to reduce edge length
 *     even if this might increase the height of the layout. Choose
 *     false if the longest-path ranking known from the literature
 *     shall be used.
 *   </tr><tr>
 *     <td><i>alignBaseClasses</i><td>bool<td>false
 *     <td>If set to true, base classes (in UML class diagrams) are aligned
 *     on the same layer (callUML only).
 *   </tr><tr>
 *     <td><i>alignSiblings</i><td>bool<td>false
 *     <td>If set to true, siblings in inheritance trees are aligned on the
 *     same layer (callUML only).
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
class OGDF_EXPORT LongestPathRanking : public RankingModule
{

    ModuleOption<AcyclicSubgraphModule> m_subgraph; //!< The acyclic sugraph module.
    bool m_sepDeg0; //!< Put isolated nodes on a separate layer?
    bool m_separateMultiEdges; //!< Separate multi-edges?
    bool m_optimizeEdgeLength; //!< Optimize for short edges.
    bool m_alignBaseClasses;   //!< Align base classes (callUML only).
    bool m_alignSiblings;      //!< Align siblings (callUML only).

    int m_offset, m_maxN;

    NodeArray<bool> m_isSource, m_finished;
    NodeArray<SListPure<Tuple2<node, int> > > m_adjacent;
    NodeArray<int> m_ingoing;

public:
    //! Creates an instance of longest-path ranking.
    LongestPathRanking();


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

    //! Call for UML graphs with special treatement of inheritance hierarchies.
    void callUML(const GraphAttributes & AG, NodeArray<int> & rank);


    /** @}
     *  @name Optional parameters
     *  @{
     */

    //! Returns the current setting of option separateDeg0Layer.
    /**
     * If set to true, isolated nodes are placed on a separate layer.
     */
    bool separateDeg0Layer() const
    {
        return m_sepDeg0;
    }

    //! Sets the option separateDeg0Layer to \a sdl.
    void separateDeg0Layer(bool sdl)
    {
        m_sepDeg0 = sdl;
    }

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

    //! Returns the current setting of option optimizeEdgeLength.
    /**
     * If set to true the ranking algorithm tries to reduce edge length
     * even if this might increase the height of the layout. Choose
     * false if the longest-path ranking known from the literature
     * shall be used.
     */
    bool optimizeEdgeLength() const
    {
        return m_optimizeEdgeLength;
    }

    //! Sets the option optimizeEdgeLength to \a b.
    void optimizeEdgeLength(bool b)
    {
        m_optimizeEdgeLength = b;
    }

    //! Returns the current setting of alignment of base classes (callUML only).
    bool alignBaseClasses() const
    {
        return m_alignBaseClasses;
    }

    //! Sets the option for alignment of base classes to \a b.
    void alignBaseClasses(bool b)
    {
        m_alignBaseClasses = b;
    }

    //! Returns the current setting of option for alignment of siblings.
    bool alignSiblings() const
    {
        return m_alignSiblings;
    }

    //! Sets the option for alignment of siblings to \a b.
    void alignSiblings(bool b)
    {
        m_alignSiblings = b;
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
                const EdgeArray<int> & length);

    void join(
        GraphCopySimple & GC,
        NodeArray<node> & superNode,
        NodeArray<SListPure<node> > & joinedNodes,
        node v, node w);

    void dfs(node v);
    void getTmpRank(node v, NodeArray<int> & rank);
    void dfsAdd(node v, NodeArray<int> & rank);
};


} // end namespace ogdf


#endif
