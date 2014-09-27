/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class Repulsion which implements an enrgy
 *        function, where the energy of the layout grows with the
 *        proximity of the vertices.
 *
 * Thus, a layout has lower energy if all vertices are far appart.
 *
 * \author Rene Weiskircher
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

#ifndef OGDF_REPULSION_H
#define OGDF_REPULSION_H


#include <ogdf/internal/energybased/NodePairEnergy.h>


namespace ogdf
{

class Repulsion: public NodePairEnergy
{
public:
    //Initializes data structures to speed up later computations
    Repulsion(GraphAttributes & AG);
private:
    //computes for two vertices an the given positions the repulsive energy
    double computeCoordEnergy(node, node, const DPoint &, const DPoint &) const;
};


}// namespace ogdf

#endif
