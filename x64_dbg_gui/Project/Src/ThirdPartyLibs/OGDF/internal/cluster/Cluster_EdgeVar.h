/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of the variable class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem
 *
 * \author Mathias Jansen
 *
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

#ifndef OGDF_MAX_CPLANAR_EDGE_H
#define OGDF_MAX_CPLANAR_EDGE_H

#include <ogdf/basic/Graph_d.h>
#include <ogdf/basic/Logger.h>

#include <abacus/variable.h>

namespace ogdf
{


class EdgeVar : public ABA_VARIABLE
{
    friend class Sub;
public:
    enum edgeType {ORIGINAL, CONNECT};

    EdgeVar(ABA_MASTER* master, double obj, edgeType eType, node source, node target);
    //! Simple version for cplanarity testing (only connect edges allowed)
    EdgeVar(ABA_MASTER* master, double obj, node source, node target);
    //! Simple version for cplanarity testing (only connect edges allowed, lower bound given)
    EdgeVar(ABA_MASTER* master, double obj, double lbound, node source, node target);

    virtual ~EdgeVar();

    edge theEdge() const
    {
        return m_edge;
    }
    node sourceNode() const
    {
        return m_source;
    }
    node targetNode() const
    {
        return m_target;
    }
    edgeType theEdgeType() const
    {
        return m_eType;
    }
    //double objCoeff() const {return m_objCoeff;}

    virtual void printMe(ostream & out)
    {
        out << "[Var: " << sourceNode() << "->" << targetNode() << " (" << ((theEdgeType() == EdgeVar::ORIGINAL) ? "original" : "connect") << ") ZF=" << obj() << "]";
    }

private:

    // The edge type of the variable
    edgeType m_eType;

    // The corresponding nodes and edge
    node m_source;
    node m_target;
    edge m_edge;

};

}

#endif

