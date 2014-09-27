/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class ShortestPathWithBFM which computes
 *        shortest paths via Bellman-Ford-Moore.
 *
 * \author Gunnar W. Klau
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

#ifndef OGDF_SHORTEST_PATH_BFM_H
#define OGDF_SHORTEST_PATH_BFM_H


#include <ogdf/module/ShortestPathModule.h>


namespace ogdf
{

class OGDF_EXPORT ShortestPathWithBFM : public ShortestPathModule
{
public:
    ShortestPathWithBFM() { }

    // computes shortest paths
    // Precond.:
    //
    // returns false iff the graph contains a negative cycle
    bool call(
        const Graph & G,                  // directed graph
        const node s,                     // source node
        const EdgeArray<int> & length,    // length of an edge
        NodeArray<int> & d,               // contains shortest path distances after call
        NodeArray<edge> & pi
    );


};


} // end namespace ogdf


#endif
