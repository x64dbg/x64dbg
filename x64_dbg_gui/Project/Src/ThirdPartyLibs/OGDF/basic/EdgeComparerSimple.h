/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declares EdgeComparerSimple class.
 *
 * The EdgeComparerSimple compares incident edges of a node
 * based on the position of the last bend point or the position
 * of the adjacent node given by the layout information of the
 * graph.
 *
 * \author Bernd Zey
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


#ifndef OGDF_EDGECOMPARER_SIMPLE_H
#define OGDF_EDGECOMPARER_SIMPLE_H



#include <ogdf/planarity/PlanRep.h>
#include <ogdf/basic/GraphAttributes.h>

namespace ogdf
{



class OGDF_EXPORT EdgeComparerSimple : public VComparer<adjEntry>
{
public:

    EdgeComparerSimple(const GraphAttributes & AG, const node v) : basis(v), m_AG(&AG) { }

    int compare(const adjEntry & e1, const adjEntry & e2) const;

private:
    node basis;
    const GraphAttributes* m_AG;

};//EdgeComparerSimple


}//namespace ogdf

#endif
