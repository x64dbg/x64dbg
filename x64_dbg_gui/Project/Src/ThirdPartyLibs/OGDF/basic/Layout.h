/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class Layout
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

#ifndef OGDF_LAYOUT_H
#define OGDF_LAYOUT_H


#include <ogdf/basic/GraphAttributes.h>


namespace ogdf
{

class GraphCopy;
class PlanRep;


/**
 * \brief Stores a layout of a graph (coordinates of nodes, bend
 *        points of edges).
 *
 */
class OGDF_EXPORT Layout
{
public:
    /** @{
     * \brief Creates a layout associated with no graph.
     */
    Layout() { }

    /**
     * \brief Creates a layout associated with graph \a G.
     *
     * The layout is initialized such that all node positions are (0,0)
     * and all bend point lists of edges are empty.
     *
     * @param G is the corresponding graph .
     */
    Layout(const Graph & G) : m_x(G, 0), m_y(G, 0), m_bends(G) { }

    // destruction
    ~Layout() { }


    /** @} @{
     * \brief Returns a reference to the array storing x-coordinates of nodes.
     */
    const NodeArray<double> & x() const
    {
        return m_x;
    }

    /**
     * \brief Returns a reference to the array storing x-coordinates of nodes.
     */
    NodeArray<double> & x()
    {
        return m_x;
    }

    /** @} @{
     * \brief Returns a reference to the array storing y-coordinates of nodes.
     */
    const NodeArray<double> & y() const
    {
        return m_y;
    }

    /**
     * \brief Returns a reference to the array storing y-coordinates of nodes.
     */
    NodeArray<double> & y()
    {
        return m_y;
    }


    /** @} @{
     * \brief Returns the x-coordinate of node \a v.
     */
    const double & x(node v) const
    {
        return m_x[v];
    }

    /**
     * \brief Returns the x-coordinate of node \a v.
     */
    double & x(node v)
    {
        return m_x[v];
    }

    /** @} @{
     * \brief Returns the y-coordinate of node \a v.
     */
    const double & y(node v) const
    {
        return m_y[v];
    }

    /**
     * \brief Returns the y-coordinate of node \a v.
     */
    double & y(node v)
    {
        return m_y[v];
    }

    /** @} @{
     * \brief Returns the bend point list of edge \a e.
     */
    const DPolyline & bends(edge e) const
    {
        return m_bends[e];
    }

    /**
     * \brief Returns the bend point list of edge \a e.
     */
    DPolyline & bends(edge e)
    {
        return m_bends[e];
    }


    /** @} @{
     * \brief Returns the polyline of edge \a eOrig in \a dpl.
     *
     * @param GC is the input graph copy; \a GC must also be the associated graph.
     * @param eOrig is an edge in the original graph of \a GC.
     * @param dpl is assigned the poyline of \a eOrig.
     */
    void computePolyline(GraphCopy & GC, edge eOrig, DPolyline & dpl) const;

    /**
     * \brief Returns the polyline of edge \a eOrig in \a dpl and clears the
     *        bend points of the copies.
     *
     * The bend point lists of all edges in the edge path corresponding to \a eOrig are
     * empty afterwards! This is a faster version of computePolyline().
     *
     * @param PG is the input graph copy; \a PG must also be the associated graph.
     *        of this layout.
     * @param eOrig is an edge in the original graph of \a GC.
     * @param dpl is assigned the poyline of \a eOrig.
     */
    void computePolylineClear(PlanRep & PG, edge eOrig, DPolyline & dpl);

    /** @} */

private:
    NodeArray<double> m_x;        //!< The x-coordinates of nodes.
    NodeArray<double> m_y;        //!< The y-coordinates of nodes.
    EdgeArray<DPolyline> m_bends; //!< The bend points of edges.

    OGDF_MALLOC_NEW_DELETE
};


} // end namespace ogdf

#endif
