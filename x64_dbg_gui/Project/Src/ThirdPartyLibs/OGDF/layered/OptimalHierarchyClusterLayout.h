/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of the optimal third phase of the sugiyama
 *        algorithm for clusters.
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

#ifndef OGDF_OPTIMAL_HIERARCHY_CLUSTER_LAYOUT_H
#define OGDF_OPTIMAL_HIERARCHY_CLUSTER_LAYOUT_H



#include <ogdf/module/HierarchyClusterLayoutModule.h>
#include <ogdf/basic/tuples.h>


namespace ogdf
{


//! The LP-based hierarchy cluster layout algorithm.
/**
 * OptimalHierarchyClusterLayout implements a hierarchy layout algorithm
 * fpr cluster graphs that is based on an LP-formulation. It is only
 * available if OGDF is compiled with LP-solver support (e.g., Coin).
 *
 * The used model avoids Spaghetti-effect like routing of edges by using
 * long vertical segments as in FastHierarchyLayout. An additional balancing
 * can be used which balances the successors below a node.
 *
 * <H3>Optional parameters</H3>
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>nodeDistance</i><td>double<td>3.0
 *     <td>The minimal allowed x-distance between nodes on a layer.
 *   </tr><tr>
 *     <td><i>layerDistance</i><td>double<td>3.0
 *     <td>The minimal allowed y-distance between layers.
 *   </tr><tr>
 *     <td><i>fixedLayerDistance</i><td>bool<td>false
 *     <td>If set to true, the distance between neighboured layers is always
 *     layerDistance; otherwise the distance is adjusted (increased) to improve readability.
 *   </tr><tr>
 *     <td><i>weightSegments</i><td>double<td>2.0
 *     <td>The weight of edge segments connecting to vertical segments.
 *   </tr><tr>
 *     <td><i>weightBalancing</i><td>double<td>0.1
 *     <td>The weight for balancing successors below a node; 0.0 means no balancing.
 *   </tr><tr>
 *     <td><i>weightClusters</i><td>double<td>0.05
 *     <td>The weight for cluster boundary variables.
 *   </tr>
 * </table>
 */
class OGDF_EXPORT OptimalHierarchyClusterLayout : public HierarchyClusterLayoutModule
{
#ifndef OGDF_LP_SOLVER
protected:
    void doCall(const ExtendedNestingGraph & H, ClusterGraphCopyAttributes & ACGC)
    {
        OGDF_THROW_PARAM(LibraryNotSupportedException, lnscCoin);
    }

#else

public:
    //! Creates an instance of optimal hierarchy layout for clusters.
    OptimalHierarchyClusterLayout();

    //! Copy constructor.
    OptimalHierarchyClusterLayout(const OptimalHierarchyClusterLayout &);

    // destructor
    ~OptimalHierarchyClusterLayout() { }


    //! Assignment operator.
    OptimalHierarchyClusterLayout & operator=(const OptimalHierarchyClusterLayout &);


    /**
     *  @name Optional parameters
     *  @{
     */

    //! Returns the minimal allowed x-distance between nodes on a layer.
    double nodeDistance() const
    {
        return m_nodeDistance;
    }

    //! Sets the minimal allowed x-distance between nodes on a layer to \a x.
    void nodeDistance(double x)
    {
        if(x >= 0)
            m_nodeDistance = x;
    }

    //! Returns the minimal allowed y-distance between layers.
    double layerDistance() const
    {
        return m_layerDistance;
    }

    //! Sets the minimal allowed y-distance between layers to \a x.
    void layerDistance(double x)
    {
        if(x >= 0)
            m_layerDistance = x;
    }

    //! Returns the current setting of option <i>fixedLayerDistance</i>.
    /**
     * If set to true, the distance is always layerDistance; otherwise
     * the distance is adjusted (increased) to improve readability.
     */
    bool fixedLayerDistance() const
    {
        return m_fixedLayerDistance;
    }

    //! Sets the option <i>fixedLayerDistance</i> to \a b.
    void fixedLayerDistance(bool b)
    {
        m_fixedLayerDistance = b;
    }

    //! Returns the weight of edge segments connecting to vertical segments.
    double weightSegments() const
    {
        return m_weightSegments;
    }

    //! Sets the weight of edge segments connecting to vertical segments to \a w.
    void weightSegments(double w)
    {
        if(w > 0.0 && w <= 100.0)
            m_weightSegments = w;
    }

    //! Returns the weight for balancing successors below a node; 0.0 means no balancing.
    double weightBalancing() const
    {
        return m_weightBalancing;
    }

    //! Sets the weight for balancing successors below a node to \a w; 0.0 means no balancing.
    void weightBalancing(double w)
    {
        if(w >= 0.0 && w <= 100.0)
            m_weightBalancing = w;
    }

    //! Returns the weight for cluster boundary variables.
    double weightClusters() const
    {
        return m_weightClusters;
    }

    //! Sets the weight for cluster boundary variables to \a w.
    void weightClusters(double w)
    {
        if(w > 0.0 && w <= 100.0)
            m_weightClusters = w;
    }

    //! @}

protected:
    //! Implements the algorithm call.
    void doCall(const ExtendedNestingGraph & H, ClusterGraphCopyAttributes & ACGC);

private:
    void buildLayerList(
        const LHTreeNode* vNode,
        List<Tuple2<int, double> > & L);

    void computeXCoordinates(
        const ExtendedNestingGraph & H,
        ClusterGraphCopyAttributes & AGC);
    void computeYCoordinates(
        const ExtendedNestingGraph & H,
        ClusterGraphCopyAttributes & AGC);

    // options
    double m_nodeDistance;       //!< The minimal distance between nodes.
    double m_layerDistance;      //!< The minimal distance between layers.
    bool   m_fixedLayerDistance; //!< Use fixed layer distances?

    double m_weightSegments;  //!< The weight of edge segments.
    double m_weightBalancing; //!< The weight for balancing.
    double m_weightClusters;  //!< The weight for cluster boundary variables.

    // auxiliary data
    ClusterGraphCopyAttributes* m_pACGC;
    const ExtendedNestingGraph* m_pH;

    int m_vertexOffset;
    int m_segmentOffset;
    int m_clusterLeftOffset;
    int m_clusterRightOffset;

    NodeArray<bool>   m_isVirtual;
    NodeArray<int>    m_vIndex;
    ClusterArray<int> m_cIndex;

#endif
};

}


#endif
