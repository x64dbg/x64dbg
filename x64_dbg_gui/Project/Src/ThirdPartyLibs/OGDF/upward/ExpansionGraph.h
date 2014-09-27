/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declares class ExpansionGraph...
 *
 * ...which represents the expansion graph of each biconnected
 * component of a given digraph.
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


#ifndef OGDF_EXPANSION_GRAPH_H
#define OGDF_EXPANSION_GRAPH_H


#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/SList.h>


namespace ogdf
{


//---------------------------------------------------------
// ExpansionGraph
// represents expansion graph of each biconnected component
// of a given digraph, i.e., each vertex v with in- and outdegree
// greater than 1 is expanded into two vertices x and y connected
// by an edge x->y such that all incoming edges are moved from
// v to x and all outgoing edges from v to y
//---------------------------------------------------------
class OGDF_EXPORT ExpansionGraph : public Graph
{
public:
    // constructor
    ExpansionGraph(const Graph & G);

    // number of biconnected components of G
    int numberOfBCs() const
    {
        return m_component.high() + 1;
    }

    // returns number of bic. component containing edge e
    int componentNumber(edge e) const
    {
        return m_compNum[e];
    }

    void setComponentNumber(edge e, int i)
    {
        m_compNum[e] = i;
    }

    // returns list of edges contained in component i
    const SListPure<edge> & component(int i) const
    {
        return m_component[i];
    }

    // returns list of components containing vertex v
    const SList<int> & adjacentComponents(node v) const
    {
        return m_adjComponents[v];
    }


    // original node of node v
    // Precond.: v is a node in the expansion graph
    node original(node v) const
    {
        return m_vOrig[v];
    }

    node representative(node v) const
    {
        node vOrig = m_vOrig[v];
        return (vOrig != 0) ? vOrig : m_vRep[v];
    }

    node copy(node vG) const
    {
        return m_vCopy[vG];
    }

    // original edge of edge e
    // Precond.: e is a edge in the expansion graph
    edge original(edge e) const
    {
        return m_eOrig[e];
    }

    // sets the original node of vCopy to vOriginal
    void setOriginal(node vCopy, node vOriginal)
    {
        m_vOrig[vCopy] = vOriginal;
    }


    // initializes to the expansion graph of the i-th biconnected component
    // of G
    void init(int i);

    // initializes to the expansion graph of G
    // advantage is that the vertices in the created copy are created in the
    // order in which the corresponding originals appear in the list of nodes
    // in G and therefore mostly have the same indices
    // mainly for debbugging purposes
    void init(const Graph & G);

private:
    node getCopy(node vOrig)
    {
        node vCopy = m_vCopy[vOrig];
        if(vCopy == 0)
        {
            vCopy = newNode();
            m_vOrig[m_vCopy[vOrig] = vCopy] = vOrig;
        }
        return vCopy;
    }

    EdgeArray<int>          m_compNum;   // component of edge e
    Array<SListPure<edge> > m_component; // edges in i-th biconnected comp.
    NodeArray<SList<int> >  m_adjComponents; // components containing v
    NodeArray<node>         m_vCopy;     // copy of original vertex
    NodeArray<node>         m_vOrig;     // original vertex of copy
    NodeArray<node>         m_vRep;
    EdgeArray<edge>         m_eOrig;     // original edge of copy
}; // class ExpansionGraph


} // end namespace ogdf


#endif
