/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of Mixed-Model layout algorithm.
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

#ifndef OGDF_MIXED_MODEL_LAYOUT_H
#define OGDF_MIXED_MODEL_LAYOUT_H


#include <ogdf/module/GridLayoutModule.h>
#include <ogdf/basic/ModuleOption.h>
#include <ogdf/module/EmbedderModule.h>
#include <ogdf/module/AugmentationModule.h>
#include <ogdf/module/ShellingOrderModule.h>
#include <ogdf/module/MixedModelCrossingsBeautifierModule.h>


namespace ogdf
{


/**
 * \brief Implementation of the Mixed-Model layout algorithm.
 *
 * The class MixedModelLayout represents the Mixed-Model layout algorithm
 * by Gutwenger and Mutzel, which is based upon ideas by Kant. In
 * particular, Kant's algorithm has been changed concerning the placement
 * phase and the vertex boxes, and it has been generalized to work for
 * connected planar graphs.
 *
 * This algorithm draws a d-planar graph \a G on a grid such that every
 * edge has at most three bends and the minimum angle between two
 * edges is at least 2/\a d radians. \a G must not contain self-loops
 * or multiple edges. The grid size is at most (2<I>n</I>-6) *
 * (3/2<I>n</I>-7/2), the number of bends is at most 5<I>n</I>-15, and
 * every edge has length O(<I>n</I>), where \a G has <I>n</I> nodes.
 *
 * The algorithm runs in several phases. In the preprocessing phase,
 * vertices with degree one are temporarily deleted and the graph is
 * being augmented to a biconnected planar graph using a planar biconnectivity
 * augmentation module. Then, a shelling order for biconnected plane
 * graphs is computed. In the next step, boxes around each
 * vertex are defined in such a way that the incident edges appear
 * regularly along the box. Finally, the coordinates of the vertex boxes
 * are computed taking the degree-one vertices into account.
 *
 * The implementation used in MixedModelLayout is based on the following
 * publication:
 *
 * C. Gutwenger, P. Mutzel: <i>Planar Polyline Drawings with Good Angular
 * Resolution</i>. 6th International Symposium on %Graph
 * Drawing 1998, Montreal (GD '98), LNCS 1547, pp. 167-182, 1998.
 *
 * <H3>Precondition</H3>
 * The input graph needs to be planar and simple (i.e., no self-loops and no
 * multiple edges).
 *
 * <H3>%Module options</H3>
 * Instances of type MixedModelLayout provide the following module options:
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>augmenter</i><td>AugmentationModule<td>PlanarAugmentation
 *     <td>Augments the graph by adding edges to obtain a planar graph with
 *     a certain connectivity (e.g., biconnected or triconnected).
 *   </tr><tr>
 *     <td><i>embedder</i><td>EmbedderModule<td>SimpleEmbedder
 *     <td>Planar embedding algorithm applied after planar augmentation.
 *   </tr><tr>
 *     <td><i>shellingOrder</i><td>ShellingOrderModule<td>BiconnectedShellingOrder
 *     <td>The algorithm to compute a shelling order. The connectivity assured
 *     by the planar augmentation module has to be sufficient for the shelling
 *     order module!
 *   </tr><tr>
 *     <td><i>crossingsBeautifier</i><td>MixedModelCrossingsBeautifierModule<td>MMDummyCrossingsBeautifier
 *     <td>The crossing beautifier is applied as preprocessing to dummy nodes
 *     in the graph that actually represent crossings. Such nodes arise when
 *     using the mixed-model layout algorithm in the planarization approach (see
 *     PlanarizationGridLayout). By default, crossings might look weird, since they
 *     are not drawn as two crossing horizontal and vertical lines; the other available
 *     crossings beautifier correct this.
 *   </tr>
 * </table>
 *
 * <H3>Running Time</H3>
 * The computation of the layout takes time O(<I>n</I>), where <I>n</I> is
 * the number of nodes of the input graph.
 */
class OGDF_EXPORT MixedModelLayout : public GridLayoutPlanRepModule
{
public:
    //! Constructs an instance of the Mixed-Model layout algorithm.
    MixedModelLayout();

    virtual ~MixedModelLayout() { }

    /**
     *  @name Module options
     *  @{
     */

    /**
     * \brief Sets the augmentation module.
     *
     * The augmentation module needs to make sure that the graph gets the
     * connectivity required for calling the shelling order module.
     */
    void setAugmenter(AugmentationModule* pAugmenter)
    {
        m_augmenter.set(pAugmenter);
    }

    //! Sets the shelling order module.
    void setShellingOrder(ShellingOrderModule* pOrder)
    {
        m_compOrder.set(pOrder);
    }

    //! Sets the crossings beautifier module.
    void setCrossingsBeautifier(MixedModelCrossingsBeautifierModule* pBeautifier)
    {
        m_crossingsBeautifier.set(pBeautifier);
    }

    //! Sets the module option for the graph embedding algorithm.
    void setEmbedder(EmbedderModule* pEmbedder)
    {
        m_embedder.set(pEmbedder);
    }

    //! @}

protected:
    //! Implements the algorithm call.
    void doCall(
        PlanRep & PG,
        adjEntry adjExternal,
        GridLayout & gridLayout,
        IPoint & boundingBox,
        bool fixEmbedding);

private:
    ModuleOption<EmbedderModule>      m_embedder;  //!< The planar embedder module.
    ModuleOption<AugmentationModule>  m_augmenter; //!< The augmentation module.
    ModuleOption<ShellingOrderModule> m_compOrder; //!< The shelling order module.
    ModuleOption<MixedModelCrossingsBeautifierModule> m_crossingsBeautifier; //!< The crossings beautifier module.
};


} // end namespace ogdf


#endif
