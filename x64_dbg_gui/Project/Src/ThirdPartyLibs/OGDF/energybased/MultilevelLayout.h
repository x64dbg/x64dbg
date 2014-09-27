/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class MultilevelLayout which realizes a
 * wrapper for the multilevel layout computation using the
 * Modular Multilevel Mixer.
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

#ifndef OGDF_MULTILEVEL_LAYOUT_H
#define OGDF_MULTILEVEL_LAYOUT_H


#include "ogdf/basic/NodeArray.h"
#include "ogdf/basic/GraphAttributes.h"
#include "ogdf/energybased/multilevelmixer/ModularMultilevelMixer.h"
#include "ogdf/energybased/multilevelmixer/InitialPlacer.h"
#include "ogdf/energybased/multilevelmixer/ScalingLayout.h"
#include "ogdf/packing/ComponentSplitterLayout.h"
#include "ogdf/basic/PreprocessorLayout.h"
#include "ogdf/basic/Constraints.h"

namespace ogdf
{

class OGDF_EXPORT MultilevelLayout : public LayoutModule
{
public:
    //! Constructor
    MultilevelLayout();

    //! Destructor
    virtual ~MultilevelLayout()
    {
        delete m_pp;
    }

    //! Calculates a drawing for the Graph GA.
    void call(GraphAttributes & GA);

    //! Calculates a drawing for the Graph GA and tries to satisfy
    //! the constraints in CG if supported.
    virtual void call(GraphAttributes & GA, GraphConstraints & GC);

    //Setting of the three main phases' methods
    //! Sets the single level layout
    void setLayout(LayoutModule* L);
    //! Sets the method used for coarsening
    void setMultilevelBuilder(MultilevelBuilder* B);
    //! Sets the placement method used when refining the levels again.
    void setPlacer(InitialPlacer* P);


private:
    ModularMultilevelMixer* m_mmm;
    ScalingLayout* m_sc;
    ComponentSplitterLayout* m_cs;
    PreprocessorLayout* m_pp;
};
} //end namespace ogdf
#endif
