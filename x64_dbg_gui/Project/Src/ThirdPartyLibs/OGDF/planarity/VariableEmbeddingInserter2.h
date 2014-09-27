/*
 * $Revision: 2566 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 23:10:08 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class VariableEmbeddingInserter2.
 *
 * \author Carsten Gutwenger<br>Jan Papenfu&szlig;
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


#ifndef OGDF_VARIABLE_EMBEDDING_INSERTER_2_H
#define OGDF_VARIABLE_EMBEDDING_INSERTER_2_H


#include <ogdf/module/EdgeInsertionModule.h>


namespace ogdf
{

class OGDF_EXPORT BCandSPQRtrees;
class OGDF_EXPORT ExpandedGraph2;


//---------------------------------------------------------
// VariableEmbeddingInserter2
// edge insertion module that inserts each edge optimally
// into a given embedding.
//---------------------------------------------------------
class OGDF_EXPORT VariableEmbeddingInserter2 : public EdgeInsertionModule
{
public:
    // construction
    VariableEmbeddingInserter2();
    // destruction
    virtual ~VariableEmbeddingInserter2() { }

    //
    // options
    //

    // sets remove-reinsert option (postprocessing)
    // possible values: (see EdgeInsertionModule)
    //    rrNone, rrInserted, rrMostCrossed, rrAll
    void removeReinsert(RemoveReinsertType rrOption)
    {
        m_rrOption = rrOption;
    }

    // returns remove-reinsert option
    RemoveReinsertType removeReinsert() const
    {
        return m_rrOption;
    }


    // sets the portion of most crossed edges used if remove-reinsert option
    // is set to rrMostCrossed
    // this portion no. of edges * percentMostCrossed() / 100
    void percentMostCrossed(double percent)
    {
        m_percentMostCrossed = percent;
    }

    // returns option percentMostCrossed
    double percentMostCrossed() const
    {
        return m_percentMostCrossed;
    }


    // returns the number of runs performed by the postprocessing
    // after algoithm has been called
    int runsPostprocessing() const
    {
        return m_runsPostprocessing;
    }

private:
    // performs actual call
    ReturnType doCall(
        PlanRep & PG,                    // planarized representation
        const List<edge> & origEdges,    // original edge to be inserted
        bool forbidCrossingGens,         // frobid crossings between gen's
        const EdgeArray<int>* costOrig,  // pointer to array of cost of original edges; if pointer is 0 all costs are 1
        const EdgeArray<bool>* forbiddenEdgeOrig);  // pointer to array deciding
    // which original edges are forbidden to cross; if pointer
    // is 0 no edges are explicitly forbidden to cross

    edge crossedEdge(adjEntry adj) const;
    int costCrossed(edge eOrig) const;

    bool                   m_forbidCrossingGens;
    const EdgeArray<int>*  m_costOrig;
    const EdgeArray<bool>* m_forbiddenEdgeOrig;
    Graph::EdgeType        m_typeOfCurrentEdge;

    void insert(edge eOrig, SList<adjEntry> & eip);
    void blockInsert(node s, node t, List<adjEntry> & L);

    PlanRep* m_pPG;

    BCandSPQRtrees* m_pBC;

    void buildSubpath(node v,
                      node vPred,
                      node vSucc,
                      List<adjEntry> & L,
                      ExpandedGraph2 & Exp,
                      node s,
                      node t);
    edge insertEdge(node v, node w, Graph & Exp,
                    NodeArray<node> & GtoExp, List<node> & nodesG);


    // options
    RemoveReinsertType m_rrOption;
    double m_percentMostCrossed;

    // results
    int m_runsPostprocessing;

}; // class VariableEmbeddingInserter2


} // end namespace ogdf


#endif
