/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class Planarity which implements an
 *        energy function where the energy of a layout depends
 *        on the number of crossings.
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

#ifndef OGDF_PLANARITY_H
#define OGDF_PLANARITY_H


#include <ogdf/internal/energybased/EnergyFunction.h>
#include <ogdf/basic/Array2D.h>


namespace ogdf
{


class Planarity: public EnergyFunction
{
public:
    //! Initializes data structures to speed up later computations.
    Planarity(GraphAttributes & AG);

    ~Planarity();

    //! Computes energy of initial layout and stores it in \a m_energy.
    void computeEnergy();

private:
    struct ChangedCrossing
    {
        int edgeNum1;
        int edgeNum2;
        bool cross;
    };

    //! Returns 1 if edges cross else 0.
    bool intersect(const edge, const edge) const;

    //! Computes energy of candidate.
    void compCandEnergy();

    //! Changes internal data if candidate is taken.
    void internalCandidateTaken();

    //! Releases memory allocated for \a m_candidateCrossings.
    void clearCandidateCrossings();

    //! Tests if two lines given by four points intersect.
    bool lowLevelIntersect(const DPoint &, const DPoint &, const DPoint &,
                           const DPoint &) const;

#ifdef OGDF_DEBUG
    virtual void printInternalData() const;
#endif

    EdgeArray<int>* m_edgeNums; //!< numbers of edges
    Array2D<bool>* m_crossingMatrix; //!< stores for each pair of edges if they cross

    /**
     * stores for all edges incident to the test node
     * an array with the crossings that change if the candidate position is chosen
     */
    List<ChangedCrossing> m_crossingChanges;

    List<edge> m_nonSelfLoops; //!< list of edges that are not slef loops
}; // class Planarity


}// namespace ogdf

#endif
