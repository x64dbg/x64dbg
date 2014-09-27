/*
 * $Revision: 2576 $
 *
 * last checkin:
 *   $Author: klein $
 *   $Date: 2012-07-11 07:50:24 +0200 (Mi, 11. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class GridLayout.
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

#ifndef OGDF_GRID_LAYOUT_H
#define OGDF_GRID_LAYOUT_H


#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/geometry.h>


namespace ogdf
{

class Layout;



/**
 * \brief Representation of a graph's grid layout
 */
class OGDF_EXPORT GridLayout
{
public:
    //! Creates an instance of a grid layout (associated with no graph).
    GridLayout() { }

    //! Creates an instance of a grid layout associated with \a G.
    GridLayout(const Graph & G) : m_x(G, 0), m_y(G, 0), m_bends(G) { }

    // destruction
    virtual ~GridLayout() { }


    //! Returns a reference to the array storing the x-coordinates of nodes.
    const NodeArray<int> & x() const
    {
        return m_x;
    }
    //! Returns a reference to the array storing the x-coordinates of nodes.
    NodeArray<int> & x()
    {
        return m_x;
    }

    //! Returns a reference to the array storing the y-coordinates of nodes.
    const NodeArray<int> & y() const
    {
        return m_y;
    }
    //! Returns a reference to the array storing the y-coordinates of nodes.
    NodeArray<int> & y()
    {
        return m_y;
    }

    //! Returns a reference to the array storing the bend points of edges.
    const EdgeArray<IPolyline> & bends() const
    {
        return m_bends;
    }
    //! Returns a reference to the array storing the bend points of edges.
    EdgeArray<IPolyline> & bends()
    {
        return m_bends;
    }


    //! Returns a reference to the x-coordinate of node \a v.
    const int & x(node v) const
    {
        return m_x[v];
    }
    //! Returns a reference to the x-coordinate of node \a v.
    int & x(node v)
    {
        return m_x[v];
    }

    //! Returns a reference to the y-coordinate of node \a v.
    const int & y(node v) const
    {
        return m_y[v];
    }
    //! Returns a reference to the y-coordinate of node \a v.
    int & y(node v)
    {
        return m_y[v];
    }


    //! Returns a reference to the bend point list of edge \a e.
    const IPolyline & bends(edge e) const
    {
        return m_bends[e];
    }
    //! Returns a reference to the bend point list of edge \a e.
    IPolyline & bends(edge e)
    {
        return m_bends[e];
    }

    //! Returns the polyline of edge \a e (including start and end point!).
    IPolyline polyline(edge e) const;


    //! Initializes the grid layout for graph \a G.
    void init(const Graph & G)
    {
        m_x.init(G, 0);
        m_y.init(G, 0);
        m_bends.init(G);
    }

    //! Initializes the grid layout for no graph (frees memory).
    void init()
    {
        m_x.init();
        m_y.init();
        m_bends.init();
    }

    //! Returns the bend point list of edge \a e without unnecessary bends.
    IPolyline getCompactBends(edge e) const;

    //! Removes all unnecessary bends.
    void compactAllBends();

    /**
     * \brief Checks if the grid layout is reasonable.
     *
     * In particular, the following checks are performed:
     *   - All nodes have to be assigned to distinct grid points.
     *   - All bend points have to be assigned to distinct points.
     *   - No bend point coincides with the position of a node.
     */
    bool checkLayout();

    /**
     * \brief Computes the bounding box of the grid layout.
     *
     * The returned bounding box is (0,0,0,0) if the associated graph is empty
     * or no graph is associated with the grid layout.
     * @param xmin is assigned the minimum x-coordinate in the grid layout.
     * @param xmax is assigned the maximum x-coordinate in the grid layout.
     * @param ymin is assigned the minimum y-coordinate in the grid layout.
     * @param ymax is assigned the maximum y-coordinate in the grid layout.
     */
    void computeBoundingBox(int & xmin, int & xmax, int & ymin, int & ymax);

    //! Computes the total manhattan edge length of the grid layout.
    int totalManhattanEdgeLength() const;

    int maxManhattanEdgeLength() const;
    int manhattanEdgeLength(edge e) const;

    //! Computes the total (euclidean) edge length of the grid layout.
    double totalEdgeLength() const;

    //! Computes the total number of bends in the grid layout.
    int numberOfBends() const;

    /**
     * \brief Transforms the grid layout to a layout.
     *
     * This implementation only copies the grid coordinates to \a drawing; the
     * derived class GridLayoutMapped performs the actual transformation of coordinates.
     */
    virtual void remap(Layout & drawing);

    static int manhattanDistance(const IPoint & ip1, const IPoint & ip2);
    static double euclideanDistance(const IPoint & ip1, const IPoint & ip2);

protected:
    NodeArray<int> m_x;  //!< The x-coordinates of nodes.
    NodeArray<int> m_y;  //!< The y-coordinates of nodes.
    EdgeArray<IPolyline> m_bends; //!< The bend points of edges.

private:
    static bool isRedundant(IPoint & p1, IPoint & p2, IPoint & p3);
    static void compact(IPolyline & ip);


    OGDF_MALLOC_NEW_DELETE
}; // class GridLayout


} // end namespace ogdf

#endif
