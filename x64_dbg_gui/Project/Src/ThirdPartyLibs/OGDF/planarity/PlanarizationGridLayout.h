/*
 * $Revision: 2583 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-12 01:02:21 +0200 (Do, 12. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of planarization with grid layout.
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

#ifndef OGDF_PLANARIZATION_GRID_LAYOUT_H
#define OGDF_PLANARIZATION_GRID_LAYOUT_H



#include <ogdf/module/GridLayoutModule.h>
#include <ogdf/basic/ModuleOption.h>
#include <ogdf/module/PlanarSubgraphModule.h>
#include <ogdf/module/EdgeInsertionModule.h>
#include <ogdf/module/GridLayoutModule.h>
#include <ogdf/module/CCLayoutPackModule.h>


namespace ogdf
{


/**
 * \brief The planarization grid layout algorithm.
 *
 * The class PlanarizationGridLayout represents a customizable implementation
 * of the planarization approach for drawing graphs. The class uses a
 * planar grid layout algorithm as a subroutine and allows to generate
 * a usual layout or a grid layout.
 *
 * If the planarization layout algorithm shall be used for simultaneous drawing,
 * you need to define the different subgraphs by setting the <i>subgraphs</i>
 * option.
 *
 * The implementation used in PlanarizationGridLayout is based on the following
 * publication:
 *
 * C. Gutwenger, P. Mutzel: <i>An Experimental Study of Crossing
 * Minimization Heuristics</i>. 11th International Symposium on %Graph
 * Drawing 2003, Perugia (GD '03), LNCS 2912, pp. 13-24, 2004.
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
 * The various phases of the algorithm can be exchanged by setting
 * module options allowing flexible customization. The algorithm provides
 * the following module options:
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>subgraph</i><td>PlanarSubgraphModule<td>FastPlanarSubgraph
 *     <td>The module for the computation of the planar subgraph.
 *   </tr><tr>
 *     <td><i>inserter</i><td>EdgeInsertionModule<td>FixedEmbeddingInserter
 *     <td>The module used for edge insertion which is applied in the second
 *     step of the planarization method. The edges not contained in the planar
 *     subgraph are re-inserted one-by-one, each with as few crossings as possible.
 *   </tr><tr>
 *     <td><i>planarLayouter</i><td>GridLayoutPlanRepModule<td>MixedModelLayout
 *     <td>The planar layout algorithm used to compute a planar layout
 *     of the planarized representation resulting from the crossing minimization step.
 *   </tr><tr>
 *     <td><i>packer</i><td>CCLayoutPackModule<td>TileToRowsCCPacker
 *     <td>The packer module used for arranging connected components.
 *   </tr>
 * </table>
 */
class OGDF_EXPORT PlanarizationGridLayout : public GridLayoutModule
{
public:
    //! Creates an instance of planarization layout and sets options to default values.
    PlanarizationGridLayout();

    ~PlanarizationGridLayout() { }

    /**
     *  @name Optional parameters
     *  @{
     */

    /**
     * \brief Returns the current setting of option pageRatio.
     *
     * This option specifies the desired ration width / height of the computed
     * layout. It is currently only used for packing connected components.
     */
    double pageRatio() const
    {
        return m_pageRatio;
    }

    //! Sets the option pageRatio to \a ratio.
    void pageRatio(double ratio)
    {
        m_pageRatio = ratio;
    }

    /** @}
     *  @name Module options
     *  @{
     */

    /**
     * \brief Sets the module option for the computation of the planar subgraph.
     *
     * The computation of a planar subgraph is the first step in the crossing
     * minimization procedure of the planarization approach.
     */
    void setSubgraph(PlanarSubgraphModule* pSubgraph)
    {
        m_subgraph.set(pSubgraph);
    }

    /**
     * \brief Sets the module option for edge insertion.
     *
     * The edge insertion module is applied in the second step of the planarization
     * method. The edges not contained in the planar subgraph are re-inserted
     * one-by-one, each with as few crossings as possible. The edge insertion
     * module implements the whole second step, i.e., it inserts all edges.
     */
    void setInserter(EdgeInsertionModule* pInserter)
    {
        m_inserter.set(pInserter);
    }

    /**
     * \brief Sets the module option for the planar grid layout algorithm.
     *
     * The planar layout algorithm is used to compute a planar layout
     * of the planarized representation resulting from the crossing
     * minimization step. Planarized representation means that edge crossings
     * are replaced by dummy nodes of degree four, so the actual layout
     * algorithm obtains a planar graph as input. By default, the planar
     * layout algorithm produces an orthogonal drawing.
     */
    void setPlanarLayouter(GridLayoutPlanRepModule* pPlanarLayouter)
    {
        m_planarLayouter.set(pPlanarLayouter);
    }

    /**
     * \brief Sets the module option for the arrangement of connected components.
     *
     * The planarization layout algorithm draws each connected component of
     * the input graph seperately, and then arranges the resulting drawings
     * using a packing algorithm.
     */
    void setPacker(CCLayoutPackModule* pPacker)
    {
        m_packer.set(pPacker);
    }


    /** @}
     *  @name Further information
     *  @{
     */

    //! Returns the number of crossings in computed layout.
    int numberOfCrossings() const
    {
        return m_nCrossings;
    }

    //! @}

protected:
    void doCall(const Graph & G, GridLayout & gridLayout, IPoint & boundingBox);


private:
    //! The module for computing a planar subgraph.
    ModuleOption<PlanarSubgraphModule>    m_subgraph;

    //! The module for edge re-insertion.
    ModuleOption<EdgeInsertionModule>     m_inserter;

    //! The module for computing a planar grid layout.
    ModuleOption<GridLayoutPlanRepModule> m_planarLayouter;

    //! The module for arranging connected components.
    ModuleOption<CCLayoutPackModule>      m_packer;

    double m_pageRatio; //!< The desired page ratio.

    int m_nCrossings; //!< The number of crossings in the computed layout.
};


} // end namespace ogdf


#endif
