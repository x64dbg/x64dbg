/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of interface for planar subgraph algorithms.
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

#ifndef OGDF_PLANAR_SUBGRAPH_MODULE_H
#define OGDF_PLANAR_SUBGRAPH_MODULE_H



#include <ogdf/planarity/PlanRepUML.h>
#include <ogdf/basic/Module.h>
#include <ogdf/basic/Logger.h>
#include <ogdf/basic/Timeouter.h>


namespace ogdf
{

/**
 * \brief Interface for planar subgraph algorithms.
 *
 * \see PlanarizationLayout, PlanarizationGridLayout
 */
class OGDF_EXPORT PlanarSubgraphModule : public Module, public Timeouter
{
public:
    //! Initializes a planar subgraph module.
    PlanarSubgraphModule() { }

    // destruction
    virtual ~PlanarSubgraphModule() { }

    /**
     * \brief Returns the set of edges \a delEdges which have to be deleted to obtain the planar subgraph.
     * @param G is the input graph.
     * @param preferedEdges are edges that should be contained in the planar subgraph.
     * @param delEdges is the set of edges that need to be deleted to obtain the planar subgraph.
     * @param preferedImplyPlanar indicates that the edges \a preferedEdges induce a planar graph.
     */
    ReturnType call(const Graph & G,
                    const List<edge> & preferedEdges,
                    List<edge> & delEdges,
                    bool preferedImplyPlanar = false)
    {
        return doCall(G, preferedEdges, delEdges, 0, preferedImplyPlanar);
    }


    /**
     * \brief Returns the set of edges \a delEdges which have to be deleted to obtain the planar subgraph.
     * @param G is the input graph.
     * @param cost are the costs of edges.
     * @param delEdges is the set of edges that need to be deleted to obtain the planar subgraph.
     */
    ReturnType call(const Graph & G, const EdgeArray<int> & cost, List<edge> & delEdges)
    {
        List<edge> preferedEdges;
        return doCall(G, preferedEdges, delEdges, &cost);
    }

    /**
     * \brief Returns the set of edges \a delEdges which have to be deleted to obtain the planar subgraph.
     * @param G is the input graph.
     * @param delEdges is the set of edges that need to be deleted to obtain the planar subgraph.
     */
    ReturnType call(const Graph & G, List<edge> & delEdges)
    {
        List<edge> preferedEdges;
        return doCall(G, preferedEdges, delEdges);
    }


    //! Returns the set of edges \a delEdges which have to be deleted to obtain the planar subgraph.
    ReturnType operator()(const Graph & G,
                          const List<edge> & preferedEdges,
                          List<edge> & delEdges,
                          bool preferedImplyPlanar = false)
    {
        return call(G, preferedEdges, delEdges, preferedImplyPlanar);
    }

    //! Returns the set of edges \a delEdges which have to be deleted to obtain the planar subgraph.
    ReturnType operator()(const Graph & G, List<edge> & delEdges)
    {
        return call(G, delEdges);
    }


    /**
     * \brief Makes \a G planar by deleting edges.
     * @param GC is a copy of the input graph.
     * @param preferedEdges are edges in \a GC that should be contained in the planar subgraph.
     * @param delOrigEdges is the set of original edges whose copy has been deleted in \a GC.
     * @param preferedImplyPlanar indicates that the edges \a preferedEdges induce a planar graph.
     */
    ReturnType callAndDelete(GraphCopy & GC,
                             const List<edge> & preferedEdges,
                             List<edge> & delOrigEdges,
                             bool preferedImplyPlanar = false);

    /**
     * \brief Makes \a G planar by deleting edges.
     * @param GC is a copy of the input graph.
     * @param delOrigEdges is the set of original edges whose copy has been deleted in \a GC.
     */
    ReturnType callAndDelete(GraphCopy & GC, List<edge> & delOrigEdges)
    {
        List<edge> preferedEdges;
        return callAndDelete(GC, preferedEdges, delOrigEdges);
    }

protected:
    // computes set of edges delEdges, which have to be deleted
    // in order to get a planar subgraph; edges in preferedEdges
    // should be contained in planar subgraph
    // must be implemented by derived classes!
    /**
     * \brief Computes the set of edges \a delEdges which have to be deleted to obtain the planar subgraph.
     *
     * This is the actual algorithm call and needs to be implemented
     * by derived classes.
     * @param G is the input graph.
     * @param preferedEdges are edges that should be contained in the planar subgraph.
     * @param delEdges is the set of edges that need to be deleted to obtain the planar subgraph.
     * @param pCost is apointer to an edge array containing the edge costs; this pointer
     *        can be 0 if no costs are given (all edges have cost 1).
     * @param preferedImplyPlanar indicates that the edges \a preferedEdges induce a planar graph.
     */
    virtual ReturnType doCall(const Graph & G,
                              const List<edge> & preferedEdges,
                              List<edge> & delEdges,
                              const EdgeArray<int>*  pCost = 0,
                              bool preferedImplyPlanar = false) = 0;



    OGDF_MALLOC_NEW_DELETE
};

} // end namespace ogdf

#endif
