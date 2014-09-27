/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief constructive compaction applying computation of min-cost
 * flow in the dual of the constraint graphs
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


#ifndef OGDF_FLOW_COMPACTION_H
#define OGDF_FLOW_COMPACTION_H


#include <ogdf/orthogonal/OrthoRep.h>
#include <ogdf/planarity/PlanRep.h>
#include <ogdf/internal/orthogonal/RoutingChannel.h>
#include <ogdf/orthogonal/MinimumEdgeDistances.h>
#include <ogdf/basic/GridLayoutMapped.h>


namespace ogdf
{

template<class ATYPE> class CompactionConstraintGraph;
class Layout;


//! represents compaction algorithm using min-cost flow in the dual of the constraint graph
class OGDF_EXPORT FlowCompaction
{
public:
    //! construction
    FlowCompaction(int maxImprovementSteps = 0,
                   int costGen = 1,
                   int costAssoc = 1);

    //! call of constructive heuristics for orthogonal representation
    void constructiveHeuristics(
        PlanRep & PG,
        OrthoRep & OR,
        const RoutingChannel<int> & rc,
        GridLayoutMapped & drawing);


    //! call of improvement heuristics for orthogonal drawing (variable cages)
    void improvementHeuristics(
        PlanRep & PG,
        OrthoRep & OR,
        const RoutingChannel<int> & rc,
        GridLayoutMapped & drawing);

    //! call of improvement heuristics for orthogonal drawing (tight cages)
    void improvementHeuristics(
        PlanRep & PG,
        OrthoRep & OR,
        //const
        MinimumEdgeDistances<int> & minDist,
        GridLayoutMapped & drawing,
        int originalSeparation //the input value before multiplication test for compaction improvement
    );

    //
    // options

    //! sets option maxImprovementSteps, which is the maximal number of steps performed by improvementHeuristics().
    void maxImprovementSteps(int maxSteps)
    {
        m_maxImprovementSteps = maxSteps;
    }

    //! returns option maxImprovementSteps
    int maxImprovementSteps() const
    {
        return m_maxImprovementSteps;
    }

    //! sets cost of arcs in constraint graph corresponding to generalizations
    void costGen(int c)
    {
        m_costGen = c;
    }

    //! returns option costGen
    int costGen() const
    {
        return m_costGen;
    }

    //! sets cost of arcs in constraint graph corresponding to associations
    void costAssoc(int c)
    {
        m_costAssoc = c;
    }

    //! returns option costGen
    int costAssoc() const
    {
        return m_costAssoc;
    }

    //! sets number of separation scaling improvement steps
    void scalingSteps(int sc)
    {
        m_scalingSteps = sc;
    }

    //! set alignment option
    void align(bool b)
    {
        m_align = b;
    }


private:
    void computeCoords(
        CompactionConstraintGraph<int> & D,
        NodeArray<int> & pos,
        bool fixZeroLength = false,
        bool fixVertexSize = false,
        bool improvementHeuristics = false,
        bool onlyGen = false);
    void dfsAssignPos(
        NodeArray<bool> & visited,
        NodeArray<int> & pos,
        node v,
        int x);

    // options
    int m_maxImprovementSteps; //!< maximal number of improvement steps
    int m_costGen;   //!< cost of arcs in constraint graph corresponding to generalization
    int m_costAssoc; //!< cost of arcs in constraint graph corresponding to associations
    bool m_cageExpense; //!< should cageedges be more expensive than others? will be propagated to compactionConstraintGraph
    //int m_costCage; //!< preliminary: Carsten uses 10
    int m_numGenSteps; //!< number of steps reserved for generalization compaction
    int m_scalingSteps; //!< number of improvement steps with decreasing separation
    bool m_align; //!< toggle if brother nodes in hierarchies should be aligned


    EdgeArray<edge> m_dualEdge;
    EdgeArray<int>  m_flow;
};


} // end namespace ogdf


#endif
