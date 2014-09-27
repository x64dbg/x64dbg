/*
 * $Revision: 2528 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-03 23:05:08 +0200 (Tue, 03 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Module for simdraw manipulator classes
 *
 * \author Michael Schulz
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

#ifndef OGDF_SIMDRAW_MANIPULATOR_MODULE_H
#define OGDF_SIMDRAW_MANIPULATOR_MODULE_H

#include<ogdf/simultaneous/SimDraw.h>

namespace ogdf
{
//! Interface for simdraw manipulators
/**
*  To avoid class SimDraw to become too large, several functions
*  have been outsourced. These are systematically
*  grouped in creation methods (SimDrawCreator), algorithm calls
*  (SimDrawCaller) and coloring methods (SimDrawColorizer).
*
*  A manipulator instance always needs a SimDraw instance (base instance)
*  to work on. The base instance is linked by pointers,
*  thus a change within the base instance after initializing does
*  not cause trouble:
*  \code
*  SimDraw SD;
*  SimDrawCreatorSimple SDCr(SD);
*  SimDrawColorizer SDCo(SD);
*  SDCr.createTrees_GKV05(4);
*  SimDrawCaller SDCa(SD);
*  SDCa.callUMLPlanarizationLayout();
*  SDCo.addColor();
*  \endcode
*/
class OGDF_EXPORT SimDrawManipulatorModule
{

protected:
    //! pointer to current simdraw instance
    SimDraw* m_SD;

    //! pointer to current graph
    Graph* m_G;

    //! pointer to current graphattributes
    GraphAttributes* m_GA;

public:
    //! default constructor
    /** creates its own simdraw instance
    */
    SimDrawManipulatorModule();

    //! constructor
    SimDrawManipulatorModule(SimDraw & SD)
    {
        init(SD);
    }

    //! initializing base instance
    void init(SimDraw & SD);

    //! returns base instance
    const SimDraw & constSimDraw() const
    {
        return *m_SD;
    }
};

} // end namespace ogdf

#endif
