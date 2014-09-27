/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class GraphCopy Attributes which manages
 *        access on a copy of an attributed graph.
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

#ifndef OGDF_A_GRAPH_COPY_H
#define OGDF_A_GRAPH_COPY_H



#include <ogdf/basic/GraphCopy.h>
#include <ogdf/basic/GraphAttributes.h>


namespace ogdf
{

//---------------------------------------------------------
// GraphCopyAttributes
// manages access on copy of an attributed graph
//---------------------------------------------------------
class OGDF_EXPORT GraphCopyAttributes
{

    const GraphCopy* m_pGC;
    GraphAttributes* m_pAG;
    NodeArray<double> m_x, m_y;

public:
    // construction
    GraphCopyAttributes(const GraphCopy & GC, GraphAttributes & AG) :
        m_pGC(&GC), m_pAG(&AG), m_x(GC, 0), m_y(GC, 0) { }

    // destruction
    ~GraphCopyAttributes() { }

    // returns width of node v
    double getWidth(node v) const
    {
        return (m_pGC->isDummy(v) ? 0.0 : m_pAG->width(m_pGC->original(v)));
    }

    // returns height of node v
    double getHeight(node v) const
    {
        return (m_pGC->isDummy(v) ? 0.0 : m_pAG->height(m_pGC->original(v)));
    }

    // returns reference to x-coord. of node v
    const double & x(node v) const
    {
        return m_x[v];
    }

    // returns reference to x-coord. of node v
    double & x(node v)
    {
        return m_x[v];
    }

    // returns reference to y-coord. of node v
    const double & y(node v) const
    {
        return m_y[v];
    }

    // returns reference to y-coord. of node v
    double & y(node v)
    {
        return m_y[v];
    }

    // sets attributes for the original graph in AG
    void transform();
};


} // end namespace ogdf


#endif
