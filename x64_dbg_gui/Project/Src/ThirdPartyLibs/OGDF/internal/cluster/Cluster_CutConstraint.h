/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of a constraint class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem.
 *
 * This class represents the cut-constraints belonging to the ILP formulation.
 * Cut-constraints are dynamically separated be means of cutting plane methods.
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

#ifndef OGDF_CLUSTER_CUT_CONSTRAINT_H
#define OGDF_CLUSTER_CUT_CONSTRAINT_H

#include <ogdf/internal/cluster/Cluster_EdgeVar.h>
#include <ogdf/internal/cluster/basics.h>

#include <abacus/constraint.h>

namespace ogdf
{

class CutConstraint : public BaseConstraint
{

public:

    CutConstraint(ABA_MASTER* master, ABA_SUB* sub, List<nodePair> & edges);

    virtual ~CutConstraint();

    // Computes and returns the coefficient for the given variable
    virtual double coeff(ABA_VARIABLE* v)
    {
        EdgeVar* ev = (EdgeVar*)v;
        return (double)coeff(ev->sourceNode(), ev->targetNode());
    }
    inline int coeff(const nodePair & n)
    {
        return coeff(n.v1, n.v2);
    }
    int coeff(node n1, node n2);

    void printMe(ostream & out) const
    {
        out << "[CutCon: ";
        forall_listiterators(nodePair, it, m_cutEdges)
        {
            (*it).printMe(out);
            out << ",";
        }
        out << "]";
    }

private:

    // The list containing the node pairs corresponding to the cut edges
    List<nodePair> m_cutEdges;

};

}

#endif
