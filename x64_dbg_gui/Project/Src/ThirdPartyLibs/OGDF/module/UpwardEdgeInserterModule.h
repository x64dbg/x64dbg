/*
 * $Revision: 2583 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-12 01:02:21 +0200 (Do, 12. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of interface for edge insertion algorithms
 *
 * \author Hoi-Ming Wong
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

#ifndef OGDF_UPWARD_EDGE_INSERTER_MODULE_H
#define OGDF_UPWARD_EDGE_INSERTER_MODULE_H


#include <ogdf/upward/UpwardPlanRep.h>
#include <ogdf/basic/Module.h>


namespace ogdf
{


class OGDF_EXPORT UpwardEdgeInserterModule : public Module
{
public:


    //! Initializes an edge insertion module.
    UpwardEdgeInserterModule() { }

    // destruction
    virtual ~UpwardEdgeInserterModule() { }

    /**
     * \brief Inserts all edges in \a origEdges into \a UPR.
     *
     * @param UPR is the input upward planarized representation of a FUPS and will also receive the result.
     * @param origEdges is the list of original edges (edges in the original graph
     *          of \a UPR) that have to be inserted.
     * \return the status of the result.
     */
    ReturnType call(UpwardPlanRep & UPR, const List<edge> & origEdges)
    {
        return doCall(UPR, origEdges, 0, 0);
    }

    /**
     * \brief Inserts all edges in \a origEdges with given costs into \a UPR.
     *
     * @param UPR is the input upward planarized representation of a FUPS and will also receive the result.
     * @param costOrig points to an edge array containing the costs of original edges; edges in
     *        \a UPR without an original edge have zero costs.
     * @param origEdges is the list of original edges (edges in the original graph
     *        of \a UPR) that have to be inserted.
     * \return the status of the result.
     */
    ReturnType call(UpwardPlanRep & UPR,
                    const EdgeArray<int> & costOrig,
                    const List<edge> & origEdges)
    {
        return doCall(UPR, origEdges, &costOrig, 0);
    }


    /**
     * \brief Inserts all edges in \a origEdges with given forbidden edges into \a UPR.
     *
     * @param UPR is the input upward planarized representation of a FUPS and will also receive the result.
     * @param costOrig points to an edge array containing the costs of original edges; edges in
     *        \a UPR without an original edge have zero costs.
     * @param forbidOriginal points to an edge array indicating if an original edge is
     *        forbidden to be crossed.
     * @param origEdges is the list of original edges (edges in the original graph
     *        of \a UPR) that have to be inserted.
     */
    ReturnType call(UpwardPlanRep & UPR,
                    const EdgeArray<int> & costOrig,
                    const EdgeArray<bool> & forbidOriginal,
                    const List<edge> & origEdges)
    {
        return doCall(UPR, origEdges, &costOrig, &forbidOriginal);
    }


    /**
     * \brief Inserts all edges in \a origEdges with given forbidden edges into \a UPR.
     *
     * \pre No forbidden edge may be in \a origEdges.
     *
     * @param UPR is the input upward planarized representation of a FUPS and will also receive the result.
     * @param forbidOriginal points to an edge array indicating if an original edge is
     *        forbidden to be crossed.
     * @param origEdges is the list of original edges (edges in the original graph
     *        of \a UPR) that have to be inserted.
     * \return the status of the result.
     */
    ReturnType call(UpwardPlanRep & UPR,
                    const EdgeArray<bool> & forbidOriginal,
                    const List<edge> & origEdges)
    {
        return doCall(UPR, origEdges, 0, &forbidOriginal);
    }





protected:
    /**
     * \brief Actual algorithm call that has to be implemented by derived classes.
     *
     * @param UPR is the input upward planarized representation of a FUPS and will also receive the result.
     * @param origEdges is the list of original edges (edges in the original graph
     *        of \a UPR) that have to be inserted.
     * @param costOrig points to an edge array containing the costs of original edges; edges in
     *        \a UPR without an original edge have zero costs.
     * @param forbiddenEdgeOrig points to an edge array indicating if an original edge is
     *        forbidden to be crossed.
     */
    virtual ReturnType doCall(UpwardPlanRep & UPR,
                              const List<edge> & origEdges,
                              const EdgeArray<int>*  costOrig,
                              const EdgeArray<bool>* forbiddenEdgeOrig
                             ) = 0;


    OGDF_MALLOC_NEW_DELETE
};

} // end namespace ogdf

#endif
