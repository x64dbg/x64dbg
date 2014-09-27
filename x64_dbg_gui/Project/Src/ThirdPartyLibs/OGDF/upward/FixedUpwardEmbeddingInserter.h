/*
 * $Revision: 2555 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 12:12:10 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class FeasibleUpwardPlanarSubgraph which
 *        computes an feasible upward planar subgraph and a feasible upward embedding.
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


#ifndef OGDF_FIXED_UPWARD_EMBEDDING_INSERTER_H
#define OGDF_FIXED_UPWARD_EMBEDDING_INSERTER_H



#include <ogdf/basic/Module.h>
#include <ogdf/upward/UpwardPlanarModule.h>
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/upward/UpwardPlanRep.h>


namespace ogdf
{


class OGDF_EXPORT FixedUpwardEmbeddingInserter : public Module
{
public:
    // construction
    FixedUpwardEmbeddingInserter();

    // destruction
    ~FixedUpwardEmbeddingInserter() { }

    // Insert all edges in UPR
    Module::ReturnType call(UpwardPlanRep & UPR, List<edge> origEdges, EdgeArray<int> & cost);

    bool isUpwardPlanar(Graph & G)
    {
        UpwardPlanarModule upMod;
        return upMod.upwardPlanarityTest(G);
    }

private:

    const int infty;

    //! compute a list of static locked edges, i.e. eges which a priory cannot included in a feasible insertion path.
    void staticLock(UpwardPlanRep & UPR, EdgeArray<bool> & locked, const List<edge> & origEdges, edge e_orig);

    //! compute a list of dynamic locked edges
    void dynamicLock(UpwardPlanRep & UPR, EdgeArray<bool> & locked, face f, adjEntry e_cur);

    void nextFeasibleEdges(UpwardPlanRep & UPR, List<adjEntry> & nextEdges, face f, adjEntry e_cur, EdgeArray<bool> & locked);

    //! compute the minimal feasible insertion path
    void minFIP(UpwardPlanRep & UPR,
                List<edge> & origEdges,
                EdgeArray<int> & cost,
                edge e_orig,
                SList<adjEntry> & path)
    {
        getPath(UPR, origEdges, cost, e_orig, path, false);
    }



    //! compute a constraint feasible insertion path usig heuristic.
    void constraintFIP(UpwardPlanRep & UPR,
                       List<edge> & origEdges,
                       EdgeArray<int> & cost,
                       edge e_orig,
                       SList<adjEntry> & path)
    {
        getPath(UPR, origEdges, cost, e_orig, path, true);
    }

    //! compute an insertion path
    void getPath(UpwardPlanRep & UPR,
                 List<edge> & origEdges,
                 EdgeArray<int> & cost,
                 edge e_orig,
                 SList<adjEntry> & path,
                 bool heuristic);


    //! mark the edges which are dominates by node v
    void markUp(const Graph & G, node v, EdgeArray<bool> & markedEdges);


    //! mark the edges which dominate node v
    void markDown(const Graph & G, node v, EdgeArray<bool> & markedEdges);

    //! compute the feasible edges of the face f with respect to e
    void feasibleEdges(UpwardPlanRep & UPR,
                       face f, // current face
                       adjEntry adj, // current adjEntry, right face muss be f
                       EdgeArray<bool> & locked, // we compute the dyn. locked edges on the fly with respect to e
                       List<adjEntry> feasible // the list of feasible edges in f with respect to e
                      );

    //! return true if current insertion path is contraint feasible
    bool isConstraintFeasible(UpwardPlanRep & UPR,
                              const List<edge> & orig_edges,
                              edge e_orig,
                              adjEntry adj, // the last adjEntry of the insertion path
                              EdgeArray<adjEntry> & predAdj //Array to reconstruction the insertion path
                             );


    //! return true if current insertion path is contraint feasible
    bool isConstraintFeasible(UpwardPlanRep & UPR,
                              List<edge> & origEdges,
                              edge e_orig,
                              SList<adjEntry> & path);

};


} // end namespace ogdf

#endif
