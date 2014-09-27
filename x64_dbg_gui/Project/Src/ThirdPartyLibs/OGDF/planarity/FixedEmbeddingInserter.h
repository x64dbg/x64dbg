/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief declaration of class FixedEmbeddingInserter
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

#ifndef OGDF_FIXED_EMBEDDING_INSERTER_H
#define OGDF_FIXED_EMBEDDING_INSERTER_H



#include <ogdf/module/EdgeInsertionModule.h>
#include <ogdf/basic/CombinatorialEmbedding.h>
#include <ogdf/basic/FaceArray.h>


template <class E> class FaceArray;


namespace ogdf
{


class OGDF_EXPORT FaceSetSimple;


//! Edge insertion module that inserts each edge optimally into a fixed embedding.
class OGDF_EXPORT FixedEmbeddingInserter : public EdgeInsertionModule
{
public:
    //! Creates an instance of fixed-embedding edge inserter.
    FixedEmbeddingInserter();

    ~FixedEmbeddingInserter() { }

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

    //! Returns the current setting of option <i>percentMostCrossed</i>.
    double percentMostCrossed() const
    {
        return m_percentMostCrossed;
    }

    //! Sets the option <i>keepEmbedding</i> to \a keep.
    /**
     * This option determines if the planar embedding of the planarized representation \a PG passed to the call-method
     * is preserved, or if always a new embedding is computed. If <i>keepEmbedding</i> is set to true,
     * \a PG must always be planarly embedded.
     */
    void keepEmbedding(bool keep)
    {
        m_keepEmbedding = keep;
    }

    //! Returns the current setting of option <i>keepEmbedding</i>.
    bool keepEmbeding() const
    {
        return m_keepEmbedding;
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
        bool forbidCrossinGens,          // frobid crossings between gen's
        const EdgeArray<int>* costOrig,  // pointer to array of cost of
        // original edges; if pointer is 0 all costs are 1
        const EdgeArray<bool>* forbiddenEdgeOrig = 0,
        const EdgeArray<unsigned int>* edgeSubGraph = 0); // pointer to array deciding
    // which original edges are forbidden to cross; if pointer is
    // is 0 no edges are explicitly forbidden to cross

    //! Construct the dual graph.
    /**
     * Marks dual edges corresponding to generalization in m_primalIsGen;
     * assumes that m_pDual, m_primalAdj and m_pNodeOf are already constructed.
     */
    void constructDualForbidCrossingGens(const PlanRepUML & PG,
                                         const CombinatorialEmbedding & E);

    //! Construct the dual graph.
    /**
     * Assumes that m_pDual, m_primalAdj and m_pNodeOf are already constructed.
     */
    void constructDual(const GraphCopy & GC,
                       const CombinatorialEmbedding & E,
                       const EdgeArray<bool>* forbiddenEdgeOrig);

    //! Finds a shortest path in the dual graph augmented by \a s and \a t.
    /**
     * Returns the list of crossed adjacency entries (corresponding
     * to used edges in the dual) in \a crossed.
     */
    void findShortestPath(const CombinatorialEmbedding & E,
                          node s,
                          node t,
                          Graph::EdgeType eType,
                          SList<adjEntry> & crossed);

    //! Finds a weighted shortest path in the dual graph augmented by \a s and \a t.
    /**
     * Uses edges weights given by costOrig;
     * returns the list of crossed adjacency entries (corresponding to used
     * edges in the dual) in \a crossed.
     */
    void findShortestPath(
        const PlanRep & PG,
        const CombinatorialEmbedding & E,
        const EdgeArray<int> & costOrig,
        node s,
        node t,
        Graph::EdgeType eType,
        SList<adjEntry> & crossed,
        const EdgeArray<unsigned int>* edgeSubGraph,
        int eSubGraph);

    void insertEdge(PlanRep & PG,
                    CombinatorialEmbedding & E,
                    edge eOrig,
                    const SList<adjEntry> & crossed,
                    bool forbidCrossingGens,
                    const EdgeArray<bool>* forbiddenEdgeOrig);

    void removeEdge(
        PlanRep & PG,
        CombinatorialEmbedding & E,
        edge eOrig,
        bool forbidCrossingGens,
        const EdgeArray<bool>* forbiddenEdgeOrig);

    edge crossedEdge(adjEntry adj) const;
    int costCrossed(edge eOrig,
                    const PlanRep & PG,
                    const EdgeArray<int> & costOrig,
                    const EdgeArray<unsigned int>* subgraphs) const;

    Graph m_dual;   //!< (Extended) dual graph, constructed/destructed during call.

    EdgeArray<adjEntry> m_primalAdj;   //!< Adjacency entry in primal graph corresponding to edge in dual.
    FaceArray<node>     m_nodeOf;      //!< The node in dual corresponding to face in primal.
    EdgeArray<bool>     m_primalIsGen; //!< true iff corresponding primal edge is a generalization.
    FaceSetSimple*       m_delFaces;
    FaceSetPure*         m_newFaces;

    node m_vS; //!< The node in extended dual representing s.
    node m_vT; //!< The node in extended dual representing t.


    RemoveReinsertType m_rrOption; //!< The remove-reinsert method.
    double m_percentMostCrossed;   //!< The portion of most crossed edges considered.
    bool m_keepEmbedding;

    int m_runsPostprocessing; //!< Runs of remove-reinsert method.
};

} // end namespace ogdf

#endif
