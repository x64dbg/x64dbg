/*
 * $Revision: 2584 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-12 02:38:07 +0200 (Do, 12. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class ClusterPlanarizationLayout
 * Planarization approach for cluster graphs
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

#ifndef OGDF_CLUSTER_PLANARIZATION_LAYOUT_H
#define OGDF_CLUSTER_PLANARIZATION_LAYOUT_H


#include <ogdf/module/LayoutClusterPlanRepModule.h>
#include <ogdf/module/CCLayoutPackModule.h>
#include <ogdf/basic/ModuleOption.h>


namespace ogdf
{


//! The cluster planarization layout algorithm.
/**
 * The class ClusterPlanarizationLayout implements the planarization
 * approach for drawing clustered graphs. Its implementation is based
 * on the following publication:
 *
 * Giuseppe Di Battista, Walter Didimo, A. Marcandalli: <i>Planarization
 * of Clustered Graphs</i>. LNCS 2265 (Proc. %Graph Drawing 2001), pp. 60-74.
 *
 * <H3>Optional parameters</H3>
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>pageRatio</i><td>double<td>1.0
 *     <td>Specifies the desired ration of width / height of the computed
 *     layout. It is currently only used when packing connected components.
 *   </tr>
 * </table>
 *
 * <H3>%Module options</H3>
 * The algorithm provides the following module options:
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>planarLayouter</i><td>LayoutClusterPlanRepModule<td>ClusterOrthoLayout
 *     <td>The c-planar layout algorithm used to compute a c-planar layout
 *     of the c-planarized representation resulting from the crossing minimization step.
 *   </tr><tr>
 *     <td><i>packer</i><td>CCLayoutPackModule<td>TileToRowsCCPacker
 *     <td>The packer module used for arranging connected components.
 *   </tr>
 * </table>
 */
class OGDF_EXPORT ClusterPlanarizationLayout
{
public:
    //! Creates an instance of cluster planarization layout.
    ClusterPlanarizationLayout();

    // Destruction
    virtual ~ClusterPlanarizationLayout() { }



    //! Calls cluster planarization layout with cluster-graph attributes \a acGraph.
    /**
     * @param G is the input graph.
     * @param acGraph is assigned the computed layout.
     * @param cGraph is the input cluster graph.
     * @param simpleCConnect If set to true, c-connectivity is achieved by adding arbitrary edges (fast).
     */
    virtual void call(
        Graph & G,
        ClusterGraphAttributes & acGraph,
        ClusterGraph & cGraph,
        bool simpleCConnect = true);
    //! Calls cluster planarization layout with cluster-graph attributes \a acGraph.
    /**
     * @param G is the input graph.
     * @param acGraph is assigned the computed layout.
     * @param cGraph is the input cluster graph.
     * @param edgeWeight allows to prefer lightweight edges for planar subgraph computation.
     * @param simpleCConnect If set to true, c-connectivity is achieved by adding arbitrary edges (fast).
     */
    virtual void call(
        Graph & G,
        ClusterGraphAttributes & acGraph,
        ClusterGraph & cGraph,
        EdgeArray<double> & edgeWeight,
        bool simpleCConnect = true);


    //! Returns the current page ratio (= desired width / height of layout).
    double pageRatio() const
    {
        return m_pageRatio;
    }

    //! Sets the page ratio to \a ratio.
    void pageRatio(double ratio)
    {
        m_pageRatio = ratio;
    }

    //! Sets the module option for the planar layout algorithm to \a pPlanarLayouter.
    void setPlanarLayouter(LayoutClusterPlanRepModule* pPlanarLayouter)
    {
        m_planarLayouter.set(pPlanarLayouter);
    }

    //! Sets the module option for the arrangement of connected components to \a pPacker.
    void setPacker(CCLayoutPackModule* pPacker)
    {
        m_packer.set(pPacker);
    }

    ////! Returns the number of crossings in the layout produced in last call.
    //int numberOfCrossings() const {
    //  return m_nCrossings;
    //}


protected:
    struct ClusterPosition
    {
        double m_minx, m_maxx, m_miny, m_maxy, m_width, m_height;
    };

    void computeClusterPositions(
        ClusterPlanRep & CP,
        Layout drawing,
        HashArray<int, ClusterPosition> & CA);


private:
    ModuleOption<LayoutClusterPlanRepModule> m_planarLayouter; //!< The planar layouter.
    ModuleOption<CCLayoutPackModule>         m_packer; //!< The packing algorithm.

    double m_pageRatio; //!< The page ratio.

    int m_nCrossings;//!< The number of crossings (not yet used!).
};


} // end namespace ogdf


#endif
