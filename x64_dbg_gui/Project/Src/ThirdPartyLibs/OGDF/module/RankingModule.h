/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of interface for ranking algorithms.
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

#ifndef OGDF_RANKING_MODULE_H
#define OGDF_RANKING_MODULE_H



#include <ogdf/basic/Graph.h>


namespace ogdf
{


/**
 * \brief Interface of algorithms for computing a node ranking.
 *
 * \see SugiyamaLayout
 */
class OGDF_EXPORT RankingModule
{
public:
    //! Initializes a ranking module.
    RankingModule() { }

    virtual ~RankingModule() { }

    /**
     * \brief Computes a node ranking of the digraph \a G in \a rank.
     *
     * This method is the actual algorithm call and must be implemented by
     * derived classes.
     *
     * @param G is the input digraph.
     * @param rank is assigned the node ranking.
     */
    virtual void call(const Graph & G, NodeArray<int> & rank) = 0;

    virtual void call(const Graph & G, const EdgeArray<int> & /* length */, const EdgeArray<int> & /* cost */, NodeArray<int> & rank)
    {
        call(G, rank);
    }

    /**
     * \brief Computes a node ranking of the digraph \a G in \a rank.
     *
     * @param G is the input digraph.
     * @param rank is assigned the node ranking.
     */
    void operator()(const Graph & G, NodeArray<int> & rank)
    {
        call(G, rank);
    }

    OGDF_MALLOC_NEW_DELETE
};


} // end namespace ogdf


#endif
