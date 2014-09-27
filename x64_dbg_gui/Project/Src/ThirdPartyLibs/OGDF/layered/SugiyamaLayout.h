/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of Sugiyama algorithm.
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

#ifndef OGDF_SUGIYAMA_LAYOUT_H
#define OGDF_SUGIYAMA_LAYOUT_H



#include <ogdf/module/LayoutModule.h>
#include <ogdf/module/RankingModule.h>
#include <ogdf/simultaneous/TwoLayerCrossMinSimDraw.h>
#include <ogdf/module/HierarchyLayoutModule.h>
#include <ogdf/module/HierarchyClusterLayoutModule.h>
#include <ogdf/module/CCLayoutPackModule.h>
#include <ogdf/basic/ModuleOption.h>
#include <ogdf/cluster/ClusterGraphAttributes.h>


namespace ogdf
{

class ExtendedNestingGraph;

/**
 * \brief Sugiyama's layout algorithm.
 *
 * The class SugiyamaLayout represents a customizable implementation
 * of Sugiyama's layout algorithm. The class provides three different
 * algorithm calls:
 *   - Calling the algorithm for a usual graph; this is the well-known
 *     Sugiyama algorithm.
 *   - Calling the algorithm for a cluster graph.
 *   - Calling the algorithm for mixed-upward graphs (e.g., UML class
 *     diagrams), where only some edges are directed and shall point
 *     in the same direction.
 *
 * If the Sugiyama algorithm shall be used for simultaneous drawing,
 * you need to define the different subgraphs by setting the <i>subgraphs</i>
 * option.
 *
 * The implementation used in SugiyamaLayout is based on the following
 * publications:
 *
 * Emden R. Gansner, Eleftherios Koutsofios, Stephen C. North,
 * Kiem-Phong Vo: <i>A technique for drawing directed graphs</i>.
 * IEEE Trans. Software Eng. 19(3), pp. 214-230, 1993.
 *
 * Georg Sander: <i>%Layout of compound directed graphs</i>.
 * Technical Report, Universit&auml;t des Saarlandes, 1996.
 *
 * <H3>Optional parameters</H3>
 * The following options affect the crossing minimization step
 * of the algorithm:
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>runs</i><td>int<td>15
 *     <td>Determines, how many times the crossing minimization is repeated.
 *     Each repetition (except for the first) starts with randomly
 *     permuted nodes on each layer. Deterministic behaviour can be achieved
 *     by setting runs to 1.
 *   </tr><tr>
 *     <td><i>transpose</i><td>bool<td>true
 *     <td>Determines whether the transpose step is
 *     performed after each 2-layer crossing minimization; this step
 *     tries to reduce the number of crossings by switching neighbored
 *     nodes on a layer.
 *   </tr><tr>
 *     <td><i>fails</i><td>int<td>4
 *     <td>The number of times that the number of crossings
 *     may not decrease after a complete top-down bottom-up traversal,
 *     before a run is terminated.
 *   </tr><tr>
 *     <td><i>arrangeCCs</i><td>bool<td>true
 *     <td>If set to true connected components are
 *     laid out separately and the resulting layouts are arranged afterwards
 *     using the packer module.
 *   </tr><tr>
 *     <td><i>minDistCC</i><td>double<td>20.0
 *     <td>Specifies the spacing between connected components of the graph.
 *     Other spacing parameters have to be set in the used hierarchy layout
 *     module.
 *   </tr><tr>
 *     <td><i>alignBaseClasses</i><td>bool<td>false
 *     <td>Determines if base classes of inheritance hierarchies shall be
 *     aligned (only callUML()).
 *   </tr><tr>
 *     <td><i>alignSiblings</i><td>bool<td>false
 *     <td>Determines if siblings in an inheritance tree shall be aligned
 *     (only callUML()).
 *   </tr>
 * </table>
 *
 * The crossing minimization step of the algorithm is affected by the
 * options <i>runs</i>, <i>transpose</i>, and <i>fails</i>. The options
 * <i>alignBaseClasses</i> and <i>alignSiblings</i> are only relevant for
 * laying out mixed-upward graphs, where directed edges are interpreted
 * as <i>generlizations</i> and undirected egdes as <i>associations</i>
 * in a UML class diagram.
 *
 * <H3>%Module options</H3>
 * The various phases of the algorithm can be exchanged by setting
 * module options allowing flexible customization. The algorithm provides
 * the following module options:
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>ranking</i><td>RankingModule<td>LongestPathRanking
 *     <td>The ranking module determines the layering of the graph.
 *   </tr><tr>
 *     <td><i>crossMin</i><td>TwoLayerCrossMin<td>BarycenterHeuristic
 *     <td>The crossMin module performs two-layer crossing
 *     minimization and is applied during the top-down bottom-up traversals.
 *   </tr><tr>
 *     <td><i>crossMinSimDraw</i><td>TwoLayerCrossMinSimDraw<td>SplitHeuristic
 *     <td>The crossMin module used with simultaneous drawing.
 *   </tr><tr>
 *     <td><i>layout</i><td>HierarchyLayoutModule<td>FastHierarchyLayout
 *     <td>The hierarchy layout module that computes the final layout
 *     (call for graph).
 *   </tr><tr>
 *     <td><i>clusterLayout</i><td>HierarchyClusterLayoutModule<td>OptimalHierarchyClusterLayout
 *     <td>The hierarchy layout module that computes the final
 *     layout (call for cluster graph).
 *   </tr><tr>
 *     <td><i>packer</i><td>CCLayoutPackModule<td>TileToRowsCCPacker
 *     <td>The packer module used for arranging connected components.
 *   </tr>
 * </table>
 */
class OGDF_EXPORT SugiyamaLayout : public LayoutModule
{

protected:

    //! the ranking module (level assignment)
    ModuleOption<RankingModule>                m_ranking;

    //! the module for two-layer crossing minimization
    ModuleOption<TwoLayerCrossMin>             m_crossMin;

    ModuleOption<TwoLayerCrossMinSimDraw>      m_crossMinSimDraw;

    //! the hierarchy layout module (final coordinate assignment)
    ModuleOption<HierarchyLayoutModule>        m_layout;

    //! the hierarchy cluster layout module (final coordinate assignment for clustered graphs)
    ModuleOption<HierarchyClusterLayoutModule> m_clusterLayout;

    //! The module for arranging connected components.
    ModuleOption<CCLayoutPackModule>           m_packer;

    int    m_fails;      //!< Option for maximal number of fails.
    int    m_runs;       //!< Option for number of runs.
    bool   m_transpose;  //!< Option for switching on transposal heuristic.
    bool   m_arrangeCCs; //!< Option for laying out components separately.
    double m_minDistCC;  //!< Option for distance between connected components.
    double m_pageRatio;  //!< Option for desired page ratio.
    bool   m_permuteFirst;

    int m_nCrossings;    //!< Number of crossings in computed layout.
    RCCrossings m_nCrossingsCluster;
    Array<bool> m_levelChanged;

    bool m_alignBaseClasses; //!< Option for aligning base classes.
    bool m_alignSiblings;    //!< Option for aligning siblings in inheritance trees.

    EdgeArray<unsigned int>* m_subgraphs; //!< Defines the subgraphs for simultaneous drawing.

public:

    //! Creates an instance of SugiyamaLayout and sets options to default values.
    SugiyamaLayout();

    // destructor
    ~SugiyamaLayout() { }


    /**
     *  @name Algorithm call
     *  @{
     */

    /**
     * \brief Calls the layout algorithm for graph \a AG.
     *
     * Returns the computed layout in \a AG.
     */
    void call(GraphAttributes & AG);

    /**
     * \brief Calls the layout algorithm for clustered graph \a AG.
     *
     * Returns the computed layout in \a AG.
     */
    void call(ClusterGraphAttributes & AG);

    /**
     * \brief Calls the layout algorithm for graph \a AG with a given level assignment.
     *
     * Returns the computed layout in \a AG.
     * @param AG is the input graph (with node size information) and is assigned
     *        the computed layout.
     * @param rank defines the level of each node.
     */
    void call(GraphAttributes & AG, NodeArray<int> & rank);

    // special call for UML graphs
    void callUML(GraphAttributes & AG);


    /** @}
     *  @name Optional parameters
     *  @{
     */

    /**
     * \brief Returns the current setting of option fails.
     *
     * This option determines, how many times the total number of crossings
     * after a complete top down or bottom up traversal may not decrease before
     * one repetition is stopped.
     */
    int fails() const
    {
        return m_fails;
    }

    //! Sets the option fails to \a nFails.
    void fails(int nFails)
    {
        m_fails = nFails;
    }

    /**
     * \brief Returns the current setting of option runs.
     *
     * This option determines, how many times the crossing minimization is
     * repeated. Each repetition (except for the first) starts with randomly
     * permuted nodes on each layer. Deterministic behaviour can be achieved
     * by setting runs to 1.
     */
    int runs() const
    {
        return m_runs;
    }

    //! Sets the option runs to \a nRuns.
    void runs(int nRuns)
    {
        m_runs = nRuns;
    }

    /**
     * \brief Returns the current setting of option transpose.
     *
     * If this option is set to true an additional fine tuning step is
     * performed after each traversal, which tries to reduce the total number
     * of crossings by switching adjacent vertices on the same layer.
     */
    bool transpose() const
    {
        return m_transpose;
    }

    //! Sets the option transpose to \a bTranspose.
    void transpose(bool bTranspose)
    {
        m_transpose = bTranspose;
    }

    /**
     * \brief Returns the current setting of option arrangeCCs.
     *
     * If this option is set to true, connected components are laid out
     * separately and arranged using a packing module.
     */
    bool arrangeCCs() const
    {
        return m_arrangeCCs;
    }

    //! Sets the options arrangeCCs to \a bArrange.
    void arrangeCCs(bool bArrange)
    {
        m_arrangeCCs = bArrange;
    }

    /**
     * \brief Returns the current setting of option minDistCC (distance between components).
     *
     * This options defines the minimum distance between connected
     * components of the graph.
     */
    double minDistCC() const
    {
        return m_minDistCC;
    }

    //! Sets the option minDistCC to \a x.
    void minDistCC(double x)
    {
        m_minDistCC = x;
    }

    /**
     * \brief Returns the current setting of option pageRation.
     *
     * This option defines the desired page ratio of the layout and
     * is used by the packing algorithms used for laying out
     * connected components.
     */
    double pageRatio() const
    {
        return m_pageRatio;
    }

    //! Sets the option pageRatio to \a x.
    void pageRatio(double x)
    {
        m_pageRatio = x;
    }

    /**
     * \brief Returns the current setting of option alignBaseClasses.
     *
     * This option defines whether base classes in inheritance hierarchies
     * shall be aligned.
     */
    bool alignBaseClasses() const
    {
        return m_alignBaseClasses;
    }

    //! Sets the option alignBaseClasses to \a b.
    void alignBaseClasses(bool b)
    {
        m_alignBaseClasses = b;
    }

    /**
     * \brief Returns the current setting of option alignSiblings.
     *
     * This option defines whether siblings in inheritance trees
     * shall be aligned.
     */
    bool alignSiblings() const
    {
        return m_alignSiblings;
    }

    //! Sets the option alignSiblings to \a b.
    void alignSiblings(bool b)
    {
        m_alignSiblings = b;
    }

    //! Sets the subgraphs for simultaneous drawing.
    void setSubgraphs(EdgeArray<unsigned int>* esg)
    {
        m_subgraphs = esg;
    }

    //! Returns true iff subgraphs for simultaneous drawing are set.
    bool useSubgraphs() const
    {
        return (m_subgraphs == 0) ? 0 : 1;
    }


    bool permuteFirst() const
    {
        return m_permuteFirst;
    }
    void permuteFirst(bool b)
    {
        m_permuteFirst = b;
    }


    /** @}
     *  @name Module options
     *  @{
     */

    /**
     * \brief Sets the module option for the node ranking (layer assignment).
     *
     * The layer assignment is the first step of the Sugiyama algorithm
     * and distributes the nodes onto layers. The layer assignment usually
     * respects edge directions; if the graph is not acyclic, the ranking
     * module computes an acyclic subgraph. The ranking module specifies
     * which method is used and usually provides a module option for
     * the acyclic subgraph.
     */
    void setRanking(RankingModule* pRanking)
    {
        m_ranking.set(pRanking);
    }

    /**
     * \brief Sets the module option for the two-layer crossing minimization.
     *
     * This module is called within the top-down and bottom-up traversal
     * of the Sugiyama crossing minimization procedure.
     */
    void setCrossMin(TwoLayerCrossMin* pCrossMin)
    {
        m_crossMin.set(pCrossMin);
    }

    /**
     * \brief Sets the module option for the computation of the final layout.
     *
     * This module receives as input the computed layer assignment and
     * and order of nodes on each layer, and computes the final coordinates
     * of nodes and bend points.
     */
    void setLayout(HierarchyLayoutModule* pLayout)
    {
        m_layout.set(pLayout);
    }

    /**
     * \brief Sets the module option for the computation of the final layout for clustered graphs.
     *
     * This module receives as input the computed layer assignment and
     * and order of nodes on each layer, and computes the final coordinates
     * of nodes and bend points.
     */
    void setClusterLayout(HierarchyClusterLayoutModule* pLayout)
    {
        m_clusterLayout.set(pLayout);
    }

    /**
     * \brief Sets the module option for the arrangement of connected components.
     *
     * If arrangeCCs is set to true, the Sugiyama layout algorithm draws each
     * connected component of the input graph seperately, and then arranges the
     * resulting drawings using this packing module.
     */
    void setPacker(CCLayoutPackModule* pPacker)
    {
        m_packer.set(pPacker);
    }

    /** @}
     *  @name Information after call
     *  The following information can be accessed after calling the algorithm.
     *  @{
     */

    //! Returns the number of crossings in the computed layout (usual graph).
    int numberOfCrossings() const
    {
        return m_nCrossings;
    }

    //! Returns the number of crossings in the computed layout (cluster graph).
    RCCrossings numberOfCrossingsCluster() const
    {
        return m_nCrossingsCluster;
    }

    //! Return the number of layers/levels}
    int numberOfLevels()
    {
        return m_numLevels;
    }

    //! Return the max. number of elements on a layer
    int maxLevelSize()
    {
        return m_maxLevelSize;
    }

    double timeReduceCrossings()
    {
        return m_timeReduceCrossings;
    }

protected:

    void reduceCrossings(Hierarchy & H);
    //void reduceCrossings2(Hierarchy &H);
    void reduceCrossings(ExtendedNestingGraph & H);

private:
    int m_numCC;
    NodeArray<int> m_compGC;

    void doCall(GraphAttributes & AG, bool umlCall);
    void doCall(GraphAttributes & AG, bool umlCall, NodeArray<int> & rank);

    int traverseTopDown(Hierarchy & H);
    int traverseBottomUp(Hierarchy & H);
    //int traverseTopDown2 (Hierarchy &H);
    //int traverseBottomUp2(Hierarchy &H);

    bool transposeLevel(int i, Hierarchy & H);
    void doTranspose(Hierarchy & H);
    void doTransposeRev(Hierarchy & H);

    int m_numLevels;
    int m_maxLevelSize;
    double m_timeReduceCrossings;

    RCCrossings traverseTopDown(ExtendedNestingGraph & H);
    RCCrossings traverseBottomUp(ExtendedNestingGraph & H);

    //NodeArray<double> m_weight;
};


}

#endif
