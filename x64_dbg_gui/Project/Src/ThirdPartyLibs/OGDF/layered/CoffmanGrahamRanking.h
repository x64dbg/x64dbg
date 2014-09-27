/*
 * $Revision: 2526 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-03 22:32:03 +0200 (Tue, 03 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of coffman graham ranking algorithm for Sugiyama
 *        algorithm.
 *
 * \author Till Sch&auml;fer
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

#ifndef OGDF_COFFMAN_GRAHAM_RANKING_H
#define OGDF_COFFMAN_GRAHAM_RANKING_H


#include <ogdf/module/RankingModule.h>
#include <ogdf/module/AcyclicSubgraphModule.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/ModuleOption.h>
#include <ogdf/basic/tuples.h>
#include <ogdf/basic/Stack.h>

namespace ogdf
{

//! The coffman graham ranking algorithm.
/**
 * The class CoffmanGrahamRanking implements a node ranking algorithmn based on
 * the coffman graham scheduling algorithm, which can be used as first phase
 * in SugiyamaLayout. The aim of the algorithm is to ensure that the height of
 * the ranking (the number of layers) is kept small.
 */
class OGDF_EXPORT CoffmanGrahamRanking : public RankingModule
{

public:
    //! Creates an instance of coffman graham ranking.
    CoffmanGrahamRanking();


    /**
     *  @name Algorithm call
     *  @{
     */

    //! Computes a node ranking of \a G in \a rank.
    void call(const Graph & G, NodeArray<int> & rank);


    /** @}
     *  @name Module options
     *  @{
     */

    //! Sets the module for the computation of the acyclic subgraph.
    void setSubgraph(AcyclicSubgraphModule* pSubgraph)
    {
        m_subgraph.set(pSubgraph);
    }

    //! @}

    //! Get for the with
    int width() const
    {
        return m_w;
    }

    //! Set for the with
    void width(int w)
    {
        m_w = w;
    }


private:
    // CoffmanGraham data structures
    class _int_set
    {
        int* A, l, p;
    public:
        _int_set() : A(NULL), l(0), p(0) { }
        _int_set(int len) : A(NULL), l(len), p(len)
        {
            if(len > 0)
                A = new int[l];
        }
        ~_int_set()
        {
            delete[] A;
        }

        void init(int len)
        {
            delete A;
            if((l = len) == 0)
                A = NULL;
            else
                A = new int[l];
            p = len;
        }

        int length() const
        {
            return l;
        }

        int operator[](int i) const
        {
            return A[i];
        }

        void insert(int x)
        {
            A[--p] = x;
        }

        bool ready() const
        {
            return (p == 0);
        }
    };

    // CoffmanGraham members
    ModuleOption<AcyclicSubgraphModule> m_subgraph;
    int m_w;
    NodeArray<_int_set> m_s;

    // dfs members
    NodeArray<int> mark;
    StackPure <node>* visited;

    // CoffmanGraham funktions
    void insert(node u, List<Tuple2<node, int> > & ready_nodes);
    void insert(node u, List<node> & ready, const NodeArray<int> & pi);

    // dfs funktions
    void removeTransitiveEdges(Graph & G);
    void dfs(node v);
};


} // end namespace ogdf


#endif
