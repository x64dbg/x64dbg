/*
 * $Revision: 2615 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 14:23:36 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of interface for edge insertion algorithms
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

#ifndef OGDF_EDGE_INSERTION_MODULE_H
#define OGDF_EDGE_INSERTION_MODULE_H


#include <ogdf/planarity/PlanRepUML.h>
#include <ogdf/basic/Logger.h>
#include <ogdf/basic/Module.h>
#include <ogdf/basic/Timeouter.h>


namespace ogdf
{

/**
 * \brief Interface for edge insertion algorithms
 *
 * \see SubgraphPlanarizer
 */
class OGDF_EXPORT EdgeInsertionModule : public Module, public Timeouter
{
public:
    //! The postprocessing method.
    enum RemoveReinsertType
    {
        rrNone,        //!< No postprocessing.
        rrInserted,    //!< Postprocessing only with the edges that have to be inserted.
        rrMostCrossed, //!< Postprocessing with the edges involved in the most crossings.
        rrAll,         //!< Postproceesing with all edges.
        rrIncremental, //!< Full postprocessing after each edge insertion.
        rrIncInserted  //!< Postprocessing for (so far) inserted edges after each edge insertion.
    };

    //! Initializes an edge insertion module.
    EdgeInsertionModule() { }
    // destruction
    virtual ~EdgeInsertionModule() { }

    /**
     * \brief Inserts all edges in \a origEdges into \a PG.
     *
     * @param PG is the input planarized representation and will also receive the result.
     * @param origEdges is the list of original edges (edges in the original graph
     *        of \a PG) that have to be inserted.
     * \return the status of the result.
     */
    ReturnType call(PlanRep & PG, const List<edge> & origEdges)
    {
        return doCall(PG, origEdges, false, 0, 0, 0);
    }

    /**
     * \brief Inserts all edges in \a origEdges with given costs into \a PG.
     *
     * @param PG is the input planarized representation and will also receive the result.
     * @param costOrig is an edge array containing the costs of original edges; edges in
     *        \a PG without an original edge have zero costs.
     * @param origEdges is the list of original edges (edges in the original graph
     *        of \a PG) that have to be inserted.
     * \return the status of the result.
     */
    ReturnType call(PlanRep & PG,
                    const EdgeArray<int> & costOrig,
                    const List<edge> & origEdges)
    {
        return doCall(PG, origEdges, false, &costOrig, 0, 0);
    }

    /**
     * \brief Inserts all edges in \a origEdges with given costs into \a PG, considering the Simultaneous Drawing Setting.
     *
     * @param PG is the input planarized representation and will also receive the result.
     * @param costOrig is an edge array containing the costs of original edges; edges in
     *        \a PG without an original edge have zero costs.
     * @param origEdges is the list of original edges (edges in the original graph
     *        of \a PG) that have to be inserted.
     * @param edgeSubGraph is an edge array specifying to which subgraph the edge belongs
     * \return the status of the result.
     */
    ReturnType call(PlanRep & PG,
                    const EdgeArray<int> & costOrig,
                    const List<edge> & origEdges,
                    const EdgeArray<unsigned int> & edgeSubGraph)
    {
        return doCall(PG, origEdges, false, &costOrig, 0, &edgeSubGraph);
    }

    /**
     * \brief Inserts all edges in \a origEdges with given forbidden edges into \a PG.
     *
     * \pre No forbidden edge may be in \a origEdges.
     *
     * @param PG is the input planarized representation and will also receive the result.
     * @param forbidOriginal is an edge array indicating if an original edge is
     *        forbidden to be crossed.
     * @param origEdges is the list of original edges (edges in the original graph
     *        of \a PG) that have to be inserted.
     * \return the status of the result.
     */
    ReturnType call(PlanRep & PG,
                    const EdgeArray<bool> & forbidOriginal,
                    const List<edge> & origEdges)
    {
        return doCall(PG, origEdges, false, 0, &forbidOriginal, 0);
    }

    /**
     * \brief Inserts all edges in \a origEdges with given costs and forbidden edges into \a PG.
     *
     * \pre No forbidden edge may be in \a origEdges.
     *
     * @param PG is the input planarized representation and will also receive the result.
     * @param costOrig is an edge array containing the costs of original edges; edges in
     *        \a PG without an original edge have zero costs.
     * @param forbidOriginal is an edge array indicating if an original edge is
     *        forbidden to be crossed.
     * @param origEdges is the list of original edges (edges in the original graph
     *        of \a PG) that have to be inserted.
     * \return the status of the result.
     */
    ReturnType call(PlanRep & PG,
                    const EdgeArray<int> & costOrig,
                    const EdgeArray<bool> & forbidOriginal,
                    const List<edge> & origEdges)
    {
        return doCall(PG, origEdges, false, &costOrig, &forbidOriginal, 0);
    }

    // inserts all edges in origEdges into PG using edge costs given by costOrig and edgeSubGraph;
    // explicitly forbids crossing of those original edges for which
    // forbidOriginal[eG] == true; no such edge may be in the list origEdges!
    ReturnType call(PlanRep & PG,
                    const EdgeArray<int> & costOrig,
                    const EdgeArray<bool> & forbidOriginal,
                    const List<edge> & origEdges,
                    const EdgeArray<unsigned int> & edgeSubGraph)
    {
        return doCall(PG, origEdges, false, &costOrig, &forbidOriginal, &edgeSubGraph);
    }

    /**
     * \brief Inserts all edges in \a origEdges into \a PG while avoiding crossings
     *        between generalizations.
     *
     * @param PG is the input planarized representation and will also receive the result.
     * @param origEdges is the list of original edges (edges in the original graph
     *        of \a PG) that have to be inserted.
     * \return the status of the result.
     */
    ReturnType callForbidCrossingGens(PlanRepUML & PG,
                                      const List<edge> & origEdges)
    {
        return doCall(PG, origEdges, true, 0, 0, 0);
    }

    /**
     * \brief Inserts all edges in \a origEdges with given costs into \a PG while
     *        avoiding crossings between generalizations.
     *
     * @param PG is the input planarized representation and will also receive the result.
     * @param costOrig is an edge array containing the costs of original edges; edges in
     *        \a PG without an original edge have zero costs.
     * @param origEdges is the list of original edges (edges in the original graph
     *        of \a PG) that have to be inserted.
     * \return the status of the result.
     */
    ReturnType callForbidCrossingGens(PlanRepUML & PG,
                                      const EdgeArray<int> & costOrig,
                                      const List<edge> & origEdges)
    {
        return doCall(PG, origEdges, true, &costOrig, 0, 0);
    }

    //! Returns the number of postprocessing runs after the algorithm has been called.
    virtual int runsPostprocessing() const
    {
        return 0;
    }


#ifdef OGDF_DEBUG
    bool checkCrossingGens(const PlanRepUML & PG);
#endif

protected:
    /**
     * \brief Actual algorithm call that has to be implemented by derived classes.
     *
     * @param PG is the input planarized representation and will also receive the result.
     * @param origEdges is the list of original edges (edges in the original graph
     *        of \a PG) that have to be inserted.
     * @param forbidCrossingGens is true if generalizations are not allowed to cross each other.
     * @param costOrig points to an edge array containing the costs of original edges; edges in
     *        \a PG without an original edge have zero costs.
     * @param forbiddenEdgeOrig points to an edge array indicating if an original edge is
     *        forbidden to be crossed.
     * @param edgeSubGraph is used for simultaneous embedding and specifies for each edge
     *        to which subgraphs it belongs.
     */
    virtual ReturnType doCall(PlanRep & PG,
                              const List<edge> & origEdges,
                              bool forbidCrossingGens,
                              const EdgeArray<int>*  costOrig,
                              const EdgeArray<bool>* forbiddenEdgeOrig,
                              const EdgeArray<unsigned int>*  edgeSubGraph) = 0;


    OGDF_MALLOC_NEW_DELETE
};

} // end namespace ogdf

#endif
