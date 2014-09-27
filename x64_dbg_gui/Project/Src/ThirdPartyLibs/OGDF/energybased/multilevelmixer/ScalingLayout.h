/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief ScalingLayout scales and calls a secondary layout
 *
 * \author Gereon Bartel
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

#ifndef OGDF_SCALING_LAYOUT_H
#define OGDF_SCALING_LAYOUT_H

#include <ogdf/basic/ModuleOption.h>
#include <ogdf/module/MultilevelLayoutModule.h>
#include <ogdf/energybased/multilevelmixer/ModularMultilevelMixer.h>
#include <ogdf/internal/energybased/MultilevelGraph.h>


namespace ogdf
{

/*!\class ScalingLayout ScalingLayout.h "ogdf/energybased/multilevelmixer/ScalingLayout.h"
 * \brief Scales a Graph relative to the ScalingType.
 *
 * For use with ModularMultilevelMixer.
 */
class OGDF_EXPORT ScalingLayout : public MultilevelLayoutModule
{
public:
    /*!
     * \brief To define the relative scale used for a Graph, the ScalingType is applied.
     */
    enum ScalingType
    {
        //! Scales by a factor relative to the drawing.
        st_relativeToDrawing,
        /*!
         * Scales by a factor relative to the avg edge weights
         * to be used in combination with the fixed edge length
         * setting in ModularMultilevelMixer.
         */
        st_relativeToAvgLength,
        //! Scales by a factor relative to the desired Edgelength m_desEdgeLength.
        st_relativeToDesiredLength,
        //! Absolute factor, can be used to scale relative to level size change.
        st_absolute
    };

    ScalingLayout();

    /**
     * \brief Computes a layout of graph \a GA.
     *
     * @param GA is the input graph and will also be assigned the layout information.
     */
    void call(GraphAttributes & GA);

    /**
     * \brief Computes a layout of graph \a MLG.
     *
     * @param MLG is the input graph and will also be assigned the layout information.
     */
    void call(MultilevelGraph & MLG);

    /*!
     * \brief Sets the minimum and the maximum scaling factor.
     *
     * @param min sets the minimum
     * @param max sets the maximum
     */
    void setScaling(double min, double max);

    /*!
     * \brief Sets how often the scaling should be repeated.
     *
     * @param steps is the number of repeats
     */
    void setExtraScalingSteps(unsigned int steps);

    /*!
     * \brief Sets a LayoutModule that should be applied after scaling.
     *
     * @param layout is the secondary LayoutModule
     */
    void setSecondaryLayout(LayoutModule* layout);

    /*!
     * \brief Is used to compute the scaling relatively to the level size change when ScalingType st_absolute is used.
     *
     * @param mmm is the ModularMultilevelMixer
     */
    void setMMM(ModularMultilevelMixer* mmm);

    /*!
     * \brief Sets a ScalingType wich sets the relative scale for the Graph
     *
     * @param type is the ScalingType
     */
    void setScalingType(ScalingType type);

    /*!
     * \brief Sets how often the LayoutModule should be applied.
     *
     * @param repeats is the number of repeats
     */
    void setLayoutRepeats(unsigned int repeats);
    //TODO: only a workaround, this should be retrieved from the layout module
    //when we have a interface class on top of Layoutmodule that allows this
    void setDesiredEdgeLength(double eLength);

private:

    // Usually a simple force-directed / energy-based Layout should be chosen.
    ModuleOption<LayoutModule> m_secondaryLayoutModule;

    double m_minScaling;
    double m_maxScaling;
    ModularMultilevelMixer* m_mmm;//!< Used to derive level size ratio if st_absolute
    double m_desEdgeLength;

    // 0 = scale to maxScaling only
    unsigned int m_extraScalingSteps;

    unsigned int m_layoutRepeats;

    ScalingType m_scalingType;
};

} // namespace ogdf

#endif
