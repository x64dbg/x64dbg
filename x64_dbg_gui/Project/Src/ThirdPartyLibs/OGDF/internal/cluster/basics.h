/*
 * $Revision: 2555 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 12:12:10 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of the master class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem.
 *
 * Basic classes for c-planarity computation.
 *
 * \author Karsten Klein
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

#ifndef OGDF_CPLANAR_BASICS_H
#define OGDF_CPLANAR_BASICS_H

#include <abacus/master.h>
#include <ogdf/basic/Graph_d.h>
#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/cluster/ClusterGraphAttributes.h>

namespace ogdf
{
class ChunkConnection;

//! Struct for storing the two corresponding nodes of an edge.
struct nodePair
{
    node v1;
    node v2;
    nodePair() {}
    nodePair(node u1, node u2) : v1(u1), v2(u2) {}
    void printMe(ostream & out) const
    {
        out << "(" << v1 << "," << v2 << ")";
    }
};
std::ostream & operator<<(std::ostream & os, const nodePair & v);


//! Struct for attaching the current lp-value to the corresponding edge.
//! Used in the primal heuristic.
struct edgeValue
{
    node src;
    node trg;
    double lpValue;
    bool original;
    edge e;
};

//! Basic constraint type
class BaseConstraint : public ABA_CONSTRAINT
{

public:
    BaseConstraint(ABA_MASTER* master, const ABA_SUB* sub, ABA_CSENSE::SENSE sense, double rhs, bool dynamic, bool local, bool liftable) :
        ABA_CONSTRAINT(master, sub, sense, rhs, dynamic, local, liftable) { }

    virtual ~BaseConstraint() { }

    virtual int coeff(const nodePair & n) = 0;
};
}//end namespace ogdf

#endif
