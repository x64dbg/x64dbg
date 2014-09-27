/*
 * $Revision: 2566 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 23:10:08 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class VariablEmbeddingInserter.
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

#ifndef OGDF_VARIABLE_EMBEDDING_INSERTER_H
#define OGDF_VARIABLE_EMBEDDING_INSERTER_H


#include <ogdf/module/EdgeInsertionModule.h>
#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/FaceArray.h>


namespace ogdf
{

class PlanarSPQRTree;
class ExpandedGraph;
class PlaneGraphCopy;


class BiconnectedComponent;


//! Optimal edge insertion module.
/**
 * The class VariableEmbeddingInserter represents the optimal edge insertion
 * algorithm, which inserts a single edge with a minum number of crossings
 * into a planar graph.
 *
 * The implementation is based on the following publication:
 *
 * Carsten Gutwenger, Petra Mutzel, Rene Weiskircher: <i>Inserting an Edge into
 * a Planar %Graph</i>. Algorithmica 41(4), pp. 289-308, 2005.
 */
class OGDF_EXPORT VariableEmbeddingInserter : public EdgeInsertionModule
{
public:
    //! Creates an instance of variable embedding edge inserter.
    VariableEmbeddingInserter();

    ~VariableEmbeddingInserter() { }

    /**
     * \brief Calls only the postprocessing; assumes that all edges in \a origEdges are already inserted into \a PG.
     *
     * @param PG is the input planarized representation and will also receive the result.
     * @param origEdges is the list of original edges (edges in the original graph
     *        of \a PG) that have to be inserted.
     * \return the status of the result.
     */
    ReturnType callPostprocessing(PlanRep & PG, const List<edge> & origEdges)
    {
        return doCallPostprocessing(PG, origEdges, false, 0, 0, 0);
    }


    /**
     *  @name Optional parameters
     *  @{
     */

    //! Sets the remove-reinsert postprocessing method.
    void removeReinsert(RemoveReinsertType rrOption)
    {
        m_rrOption = rrOption;
    }

    //! Returns the current setting of the remove-reinsert postprocessing method.
    RemoveReinsertType removeReinsert() const
    {
        return m_rrOption;
    }


    //! Sets the option <i>percentMostCrossed</i> to \a percent.
    /**
     * This option determines the portion of most crossed edges used if the remove-reinsert
     * method is set to #rrMostCrossed. This portion is number of edges * percentMostCrossed() / 100.
     */
    void percentMostCrossed(double percent)
    {
        m_percentMostCrossed = percent;
    }

    //! Returns the current setting of option percentMostCrossed.
    double percentMostCrossed() const
    {
        return m_percentMostCrossed;
    }

    /** @}
     *  @name Further information
     *  @{
     */

    //! Returns the number of runs performed by the remove-reinsert method after the algorithm has been called.
    int runsPostprocessing() const
    {
        return m_runsPostprocessing;
    }

    //! @}

private:
    //! Implements the algorithm call.
    ReturnType doCall(
        PlanRep & PG,                 // planarized representation
        const List<edge> & origEdges,    // original edge to be inserted
        bool forbidCrossingGens,         // frobid crossings between gen's
        const EdgeArray<int>* costOrig,  // pointer to array of cost of original edges; if pointer is 0 all costs are 1
        const EdgeArray<bool>* forbiddenEdgeOrig, // pointer to array deciding
        // which original edges are forbidden to cross; if pointer is
        // is 0 no edges are explicitly forbidden to cross
        const EdgeArray<unsigned int>* edgeSubGraph); // pointer to array of SubGraphInformation
    // used in Simultaneous Drawing

    ReturnType doCallPostprocessing(
        PlanRep & PG,                 // planarized representation
        const List<edge> & origEdges,    // original edge to be inserted
        bool forbidCrossingGens,         // frobid crossings between gen's
        const EdgeArray<int>* costOrig,  // pointer to array of cost of original edges; if pointer is 0 all costs are 1
        const EdgeArray<bool>* forbiddenEdgeOrig, // pointer to array deciding
        // which original edges are forbidden to cross; if pointer is
        // is 0 no edges are explicitly forbidden to cross
        const EdgeArray<unsigned int>* edgeSubGraph); // pointer to array of SubGraphInformation

    edge crossedEdge(adjEntry adj) const;
    int costCrossed(edge eOrig) const;

    bool                   m_forbidCrossingGens;
    const EdgeArray<int>*  m_costOrig;
    const EdgeArray<bool>* m_forbiddenEdgeOrig;
    const EdgeArray<unsigned int>*  m_edgeSubgraph;
    Graph::EdgeType        m_typeOfCurrentEdge;

    void insert(node s, node t, SList<adjEntry> & eip);
    void blockInsert(const BiconnectedComponent & G,
                     node s,
                     node t,
                     List<adjEntry> & L);

    bool dfsVertex(node v, int parent);
    bool dfsComp(int i, node parent, node & repT);

    PlanRep* m_pPG;
    node   m_s, m_t;
    edge   m_st;
    SList<adjEntry>* m_pEip;

    static int m_bigM;  // used for SimDraw

    NodeArray<SList<int> > m_compV;
    Array<SList<node> >    m_nodeB;
    Array<SList<edge> >    m_edgeB;
    NodeArray<node>        m_GtoBC;

    bool pathSearch(node v, edge parent, List<edge> & path);
    void buildSubpath(node v,
                      edge eIn,
                      edge eOut,
                      List<adjEntry> & L,
                      ExpandedGraph & Exp,
                      node s,
                      node t);
    edge insertEdge(node v, node w, Graph & Exp,
                    NodeArray<node> & GtoExp, List<node> & nodesG);

    node m_v1, m_v2;

    RemoveReinsertType m_rrOption; //!< The remove-reinsert method.
    double m_percentMostCrossed;   //!< The portion of most crossed edges considered.

    int m_runsPostprocessing; //!< Runs of remove-reinsert method.
};

} // end namespace ogdf

#endif
