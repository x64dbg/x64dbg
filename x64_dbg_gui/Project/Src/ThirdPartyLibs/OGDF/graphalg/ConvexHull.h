/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of doubly linked lists and iterators
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

#ifndef OGDF_CONVEX_HULL_H
#define OGDF_CONVEX_HULL_H

#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/geometry.h>
#include <ogdf/internal/energybased/MultilevelGraph.h>
#include <vector>

namespace ogdf
{

// all returned Polygons are clockwise (cw)
class OGDF_EXPORT ConvexHull
{
private:
    bool sameDirection(const DPoint & start, const DPoint & end, const DPoint & s, const DPoint & e) const;

    // calculates a convex hull very quickly but only works with cross-free Polygons!
    DPolygon conv(const DPolygon & poly) const;

    // Calculates the Part of the convex hull that is left of line start-end
    // /a points should only contain points that really are left of the line.
    void leftHull(std::vector<DPoint> points, DPoint & start, DPoint & end, DPolygon & hullPoly) const;


public:
    ConvexHull();
    ~ConvexHull();

    DPoint calcNormal(const DPoint & start, const DPoint & end) const;
    double leftOfLine(const DPoint & normal, const DPoint & point, const DPoint & pointOnLine) const;

    DPolygon call(std::vector<DPoint> points) const;
    DPolygon call(GraphAttributes & GA) const;
    DPolygon call(MultilevelGraph & MLG) const;

};

} // namespace ogdf

#endif
