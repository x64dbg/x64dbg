/*
 * $Revision: 2614 $
 *
 * last checkin:
 *   $Author: chimani $
 *   $Date: 2012-07-16 11:30:08 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of Tutte's algorithm
 *
 * The class CoinTutteLayout represents the layout algorithm by
 * Tutte.
 *
 * \par
 * This algorithm draws a planar graph \a G straight-line
 * without crossings. It can also draw non-planar graphs.
 *
 * \par
 * The idea of the algorithm is to place every vertex into the
 * center of gravity by its neighbours.
 *
 * \author David Alberts and Andrea Wagner
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


#ifndef OGDF_TUTTE_LAYOUT_H
#define OGDF_TUTTE_LAYOUT_H

#include <ogdf/module/LayoutModule.h>
#include <ogdf/basic/geometry.h>
#include <ogdf/external/coin.h>

#ifdef USE_COIN
#include <coin/CoinPackedMatrix.hpp>
#endif

namespace ogdf
{

class OGDF_EXPORT TutteLayout : public LayoutModule
{
#ifndef USE_COIN
public:

    void call(GraphAttributes & AG)
    {
        THROW_NO_COIN_EXCEPTION;
    }
    void call(GraphAttributes & AG, const List<node> & givenNodes)
    {
        THROW_NO_COIN_EXCEPTION;
    }

};

#else // USE_COIN
public:

    TutteLayout();
    ~TutteLayout() { }

    DRect bbox() const
    {
        return m_bbox;
    }

    void bbox(const DRect & bb)
    {
        m_bbox = bb;
    }

    void call(GraphAttributes & AG);
    void call(GraphAttributes & AG, const List<node> & givenNodes);


private:

    void setFixedNodes(const Graph & G, List<node> & nodes,
                       List<DPoint> & pos, double radius = 1.0);
    /*! sets the positions of the nodes in a largest face of $G$ in the
    *  form of a regular $k$-gon with the prescribed radius. The
    *  corresponding nodes and their positions are stored in nodes
    *  and pos, respectively. $G$ does not have to be planar!
    */

    void setFixedNodes(const Graph & G, List<node> & nodes, const List<node> & givenNodes,
                       List<DPoint> & pos, double radius = 1.0);
    /*! the method is overloaded for a given set of nodes.
    */

    bool doCall(GraphAttributes & AG,
                const List<node> & fixedNodes,
                List<DPoint> & fixedPositions);

    DRect m_bbox;
};

#endif // USE_COIN

} // end namespace ogdf

#endif // OGDF_TUTTE_LAYOUT_H
