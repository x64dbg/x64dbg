/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of min-cost-flow algorithm (class
 *        MinCostFlowReinelt)
 *
 * \author Carsten Gutwenger and Gerhard Reinelt
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

#ifndef OGDF_MIN_COST_FLOW_REINELT_H
#define OGDF_MIN_COST_FLOW_REINELT_H


#include <ogdf/module/MinCostFlowModule.h>
#include <limits.h>


namespace ogdf
{


class OGDF_EXPORT MinCostFlowReinelt : public MinCostFlowModule
{
public:
    MinCostFlowReinelt() { }

    // computes min-cost flow
    // Precond.: graph must be connected, lowerBound[e] <= upperBound[e]
    //   for all edges e, sum over all supply[v] equals 0
    // returns true iff a feasible min-cost flow exists
    bool call(
        const Graph & G,                  // directed graph
        const EdgeArray<int> & lowerBound, // lower bound for flow
        const EdgeArray<int> & upperBound, // upper bound for flow
        const EdgeArray<int> & cost,      // cost of an edge
        const NodeArray<int> & supply,    // supply (if neg. demand) of a node
        EdgeArray<int> & flow);           // computed flow

    bool call(
        const Graph & G,                  // directed graph
        const EdgeArray<int> & lowerBound, // lower bound for flow
        const EdgeArray<int> & upperBound, // upper bound for flow
        const EdgeArray<int> & cost,      // cost of an edge
        const NodeArray<int> & supply,    // supply (if neg. demand) of a node
        EdgeArray<int> & flow,            // computed flow
        NodeArray<int> & dual);           // computed dual variables

    int infinity() const
    {
        return INT_MAX;
    }

private:

    struct arctype;

    struct nodetype
    {
        nodetype* father;     /* ->father in basis tree */
        nodetype* successor;  /* ->successor in preorder */
        arctype* arc_id;      /* ->arc (node,father) */
        bool orientation;     /* false<=>basic arc=(father->node)*/
        int dual;             /* value of dual variable */
        int flow;             /* flow in basic arc (node,father) */
        int name;             /* identification of node = node-nr*/
        nodetype* last;       /* last node in subtree */
        int nr_of_nodes;      /* number of nodes in subtree */
    };

    struct arctype
    {
        arctype* next_arc;    /* -> next arc in list */
        nodetype* tail;       /* -> tail of arc */
        nodetype* head;       /* -> head of arc */
        int cost;             /* cost of unit flow */
        int upper_bound;      /* capacity of arc */
        int arcnum;           /* number of arc in input */

        OGDF_NEW_DELETE
    };


    int mcf(
        int mcfNrNodes,
        int mcfNrArcs,
        Array<int> & mcfSupply,
        Array<int> & mcfTail,
        Array<int> & mcfHead,
        Array<int> & mcfLb,
        Array<int> & mcfUb,
        Array<int> & mcfCost,
        Array<int> & mcfFlow,
        Array<int> & mcfDual,
        int* mcfObj
    );

    void start(Array<int> & supply);

    void beacircle(arctype** eplus, arctype** pre, bool* from_ub);
    void beadouble(arctype** eplus, arctype** pre, bool* from_ub);


    Array<nodetype> nodes;     /* node space */
    Array<arctype> arcs;       /* arc space */
    //Array<nodetype *> p;    /*used for starting procedure*/

    nodetype* root;         /*->root of basis tree*/
    nodetype rootStruct;

    arctype* last_n1;       /*->start for search for entering arc in N' */
    arctype* last_n2;       /*->start for search for entering arc in N''*/
    arctype* start_arc;     /* -> initial arc list*/
    arctype* start_b;       /* -> first basic arc*/
    arctype* start_n1;      /* -> first nonbasic arc in n'*/
    arctype* start_n2;      /* -> first nonbasic arc in n''*/
    arctype* startsearch;   /* ->start of search for basis entering arc */
    arctype* searchend;     /* ->end of search for entering arc in bea */
    arctype* searchend_n1;  /*->end of search for entering arc in N' */
    arctype* searchend_n2;  /*->end of search for entering arc in N''*/

    //int artvalue;          /*cost and upper_bound of artificial arc */
    int m_maxCost;         // maximum of the cost of all input arcs

    int nn;                /*number of original nodes*/
    int mm;                /*number of original arcs*/

};


} // end namespace ogdf


#endif
