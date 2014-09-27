/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of a Mixed-Model crossings beautifier
 * that uses grid doubling.
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

#ifndef OGDF_MMCB_DOUBLE_GRID_H
#define OGDF_MMCB_DOUBLE_GRID_H



#include <ogdf/planarlayout/MMCBBase.h>


namespace ogdf
{


/**
 * \brief Crossings beautifier using grid doubling.
 */
class OGDF_EXPORT MMCBDoubleGrid : public MMCBBase
{
public:
    //! Creates an instance of the crossings beautifier module.
    MMCBDoubleGrid() { }

    ~MMCBDoubleGrid() { }

protected:
    //! Implements the module call.
    void doCall(const PlanRep & PG, GridLayout & gl, const List<node> & L);
};


} // end namespace ogdf

#endif
