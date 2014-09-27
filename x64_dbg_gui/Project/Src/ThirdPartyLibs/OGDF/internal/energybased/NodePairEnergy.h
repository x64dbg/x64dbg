/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declares class NodePairEnergy which implements an energy
 *        function where the energy of a layout depends on the
 *        each pair of nodes.
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

#ifndef OGDF_NODE_PAIR_ENERGY_H
#define OGDF_NODE_PAIR_ENERGY_H


#include <ogdf/internal/energybased/AdjacencyOracle.h>
#include <ogdf/internal/energybased/EnergyFunction.h>
#include <ogdf/internal/energybased/IntersectionRectangle.h>


namespace ogdf
{

class NodePairEnergy: public EnergyFunction
{
public:
    //Initializes data dtructures to speed up later computations
    NodePairEnergy(const String energyname, GraphAttributes & AG);
    virtual ~NodePairEnergy()
    {
        delete m_nodeNums;
        delete m_pairEnergy;
    }
    //computes the energy of the initial layout
    void computeEnergy();
protected:
    //computes the energy stored by a pair of vertices at the given positions
    virtual double computeCoordEnergy(node, node, const DPoint &, const DPoint &) const = 0;
    //returns the internal number given to each vertex
    int nodeNum(node v) const
    {
        return (*m_nodeNums)[v];
    }
    //returns true in constant time if two vertices are adjacent
    bool adjacent(const node v, const node w) const
    {
        return m_adjacentOracle.adjacent(v, w);
    }
    //returns the shape of a vertex as an IntersectionRectangle
    const IntersectionRectangle & shape(const node v) const
    {
        return m_shape[v];
    }

#ifdef OGDF_DEBUG
    virtual void printInternalData() const;
#endif

private:
    NodeArray<int>* m_nodeNums;//stores internal number of each vertex
    Array2D<double>* m_pairEnergy;//stores for each pair of vertices its energy
    NodeArray<double> m_candPairEnergy;//stores for each vertex its pair energy with
    //respect to the vertex to be moved if its new position is chosen
    NodeArray<IntersectionRectangle> m_shape;//stores the shape of each vertex as
    //an IntersectionRectangle
    List<node> m_nonIsolated;//list of vertices with degree greater zero
    const AdjacencyOracle m_adjacentOracle;//structure for constant time adjacency queries
    //function computes energy stored in a certain pair of vertices
    double computePairEnergy(const node v, const node w) const;
    //computes energy of whole layout if new position of the candidate vertex is chosen
    void compCandEnergy();
    //If a candidate change is chosen as the new position, this function sets the
    //internal data accordingly
    void internalCandidateTaken();
};


}// namespace ogdf

#endif
