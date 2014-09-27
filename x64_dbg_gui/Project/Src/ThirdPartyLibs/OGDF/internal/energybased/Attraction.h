/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declares class Attraction.
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

#ifndef OGDF_ATTRACTION_H
#define OGDF_ATTRACTION_H


#include <ogdf/internal/energybased/NodePairEnergy.h>

namespace ogdf
{


//! Energy function for attraction between two adjacent vertices.
/**
 * Implements an energy function that simulates
 * attraction between two adjacent vertices. There is an optimum
 * distance where the energy is zero. The energy grows quadratic
 * with the difference to the optimum distance. The optimum
 * distance between two adjacent vertices depends on the size of
 * the two vertices.
 */
class Attraction: public NodePairEnergy
{
public:
    //Initializes data structures to speed up later computations
    Attraction(GraphAttributes & AG);
    ~Attraction() {}
    //! set the preferred edge length to the absolute value l
    void setPreferredEdgelength(double l)
    {
        m_preferredEdgeLength = l;
    }
    //! set multiplier for the edge length with repspect to node size to multi
    void reinitializeEdgeLength(double multi);
#ifdef OGDF_DEBUG
    void printInternalData() const;
#endif
private:
    //! Average length and height of nodes is multiplied by this factor to get preferred edge length
    static const double MULTIPLIER;
    //! the length that that all edges should ideally have
    double m_preferredEdgeLength;
    //! computes the energy contributed by the two nodes if they are placed at the two given positions
    double computeCoordEnergy(node, node, const DPoint &, const DPoint &) const;
};

}// namespace ogdf

#endif
