/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief useable example of the Modular Multilevel Mixer
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

#ifndef OGDF_MMM_EXAMPLE_NICE_LAYOUT_H
#define OGDF_MMM_EXAMPLE_NICE_LAYOUT_H

#include <ogdf/module/LayoutModule.h>
#include <ogdf/internal/energybased/MultilevelGraph.h>

namespace ogdf
{

/** \brief An example Layout using the Modular Mutlievel Mixer
 *
 * This example is tuned for nice drawings for most types of graphs.
 * EdgeCoverMerger and BarycenterPlacer are used as merging and placement
 * strategies. The FastMultipoleEmbedder is for force calculation.
 *
 * For an easy variation of the Modular Multilevel Mixer copy the code in call.
 */
class OGDF_EXPORT MMMExampleNiceLayout : public LayoutModule
{
public:

    //! Constructor
    MMMExampleNiceLayout();

    //! calculates a drawing for the Graph GA
    void call(GraphAttributes & GA);

    //! calculates a drawing for the Graph MLG
    void call(MultilevelGraph & MLG);

private:

};

} // namespace ogdf

#endif

