/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class DfsAcyclicSubgraph
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

#ifndef OGDF_DFS_ACYCLIC_SUBGRAPH_H
#define OGDF_DFS_ACYCLIC_SUBGRAPH_H



#include <ogdf/module/AcyclicSubgraphModule.h>


namespace ogdf
{

class GraphAttributes;



//! DFS-based algorithm for computing a maximal acyclic subgraph.
/**
 * The algorithm simply removes all DFS-backedges and works in linear-time.
 */
class OGDF_EXPORT DfsAcyclicSubgraph : public AcyclicSubgraphModule
{
public:
    //! Computes the set of edges \a arcSet, which have to be deleted in the acyclic subgraph.
    void call(const Graph & G, List<edge> & arcSet);

    //! Call for UML graph.
    /**
     * Computes the set of edges \a arcSet, which have to be deleted
     * in the acyclic subgraph.
     */
    void callUML(const GraphAttributes & AG, List<edge> & arcSet);

private:
    int dfsFindHierarchies(
        const GraphAttributes & AG,
        NodeArray<int> & hierarchy,
        int i,
        node v);

    void dfsBackedgesHierarchies(
        const GraphAttributes & AG,
        node v,
        NodeArray<int> & number,
        NodeArray<int> & completion,
        int & nNumber,
        int & nCompletion);

};


} // end namespace ogdf


#endif
