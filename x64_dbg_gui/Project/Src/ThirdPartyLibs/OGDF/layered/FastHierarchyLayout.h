/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief declaration and implementation of the third phase of sugiyama
 *
 * \author Sebastian Leipert
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

#ifndef OGDF_FAST_HIERARCHY_LAYOUT_H
#define OGDF_FAST_HIERARCHY_LAYOUT_H



#include <ogdf/module/HierarchyLayoutModule.h>
#include <ogdf/basic/List.h>


namespace ogdf
{


/**
 * \brief Coordinate assignment phase for the Sugiyama algorithm by Buchheim et al..
 *
 * This class implements a hierarchy layout algorithm, i.e., it layouts
 * hierarchies with a given order of nodes on each layer. It is used as a third
 * phase of the Sugiyama algorithm.
 *
 * All edges of the layout will have at most two bends. Additionally,
 * for each edge having exactly two bends, the segment between them is
 * drawn vertically. This applies in particular to the long edges
 * arising in the first phase of the Sugiyama algorithm.
 *
 * The implementation is based on:
 *
 * Christoph Buchheim, Michael J&uuml;nger, Sebastian Leipert: <i>A Fast %Layout
 * Algorithm for k-Level Graphs</i>. LNCS 1984 (Proc. %Graph Drawing 2000),
 * pp. 229-240, 2001.
 *
 * <h3>Optional Parameters</h3>
 *
 * <table>
 *   <tr>
 *     <th>Option</th><th>Type</th><th>Default</th><th>Description</th>
 *   </tr><tr>
 *     <td><i>node distance</i></td><td>double</td><td>3.0</td>
 *     <td>the minimal horizontal distance between two nodes on the same layer</td>
 *   </tr><tr>
 *     <td><i>layer distance</i></td><td>double</td><td>3.0</td>
 *     <td>the minimal vertical distance between two nodes on neighbored layers</td>
 *   </tr><tr>
 *     <td><i>fixed layer distance</i></td><td>bool</td><td>false</td>
 *     <td>if true, the distance between neighbored layers is fixed, otherwise variable</td>
 *   </tr>
 * </table>
 */
class OGDF_EXPORT FastHierarchyLayout : public HierarchyLayoutModule
{
protected:

    void doCall(const Hierarchy & H, GraphCopyAttributes & AGC);

public:
    //! Creates an instance of fast hierarchy layout.
    FastHierarchyLayout();

    //! Copy constructor.
    FastHierarchyLayout(const FastHierarchyLayout &);

    // destructor
    virtual ~FastHierarchyLayout() { }


    //! Assignment operator
    FastHierarchyLayout & operator=(const FastHierarchyLayout &);


    //! Returns the option <i>node distance</i>.
    double nodeDistance() const
    {
        return m_minNodeDist;
    }

    //! Sets the option node distance to \a dist.
    void nodeDistance(double dist)
    {
        m_minNodeDist = dist;
    }

    //! Returns the option <i>layer distance</i>.
    double layerDistance() const
    {
        return m_minLayerDist;
    }

    //! Sets the option layer distance to \a dist.
    void layerDistance(double dist)
    {
        m_minLayerDist = dist;
    }

    //! Returns the option <i>fixed layer distance</i>.
    bool fixedLayerDistance() const
    {
        return m_fixedLayerDist;
    }

    //! Sets the option fixed layer distance to \a b.
    void fixedLayerDistance(bool b)
    {
        m_fixedLayerDist = b;
    }


private:

    int n;      //!< The number of nodes including virtual nodes.
    int m;      //!< The number edge sections.
    int k;      //!< The number of layers.
    int* layer; //!< Stores for every node its layer.
    int* first; //!< Stores for every layer the index of the first node.


    // nodes are numbered top down and from left to right.
    // Is called "internal numbering".
    // Nodes and Layeras are number 0 to n-1 and 0 to k-1, respectively.
    // For thechnical reasons we set first[k] to n.

    /**
     * \brief The list of neighbors in previous / next layer.
     *
     * for every node : adj[0][node] list of neighbors in previous layer;
     * for every node : adj[1][node] list of neighbors in next layer
     */
    List<int>* adj[2];

    /**
     * \brief The nodes belonging to a long edge.
     *
     * for every node : longEdge[node] is a pointer to a list containing all
     * nodes that belong to the same long edge as node.
     */
    List<int>** longEdge;

    double m_minNodeDist; //!< The minimal node distance on a layer.
    double m_minLayerDist;//!< The minimal distance between layers.
    double* breadth;      //!< for every node : breadth[node] = width of the node.
    double* height;       //!< for every layer : height[layer] = height of max{height of node on layer}.
    double* y;            //!< for every layer : y coordinate of layer.
    double* x;            //!< for every node : x coordinate of node.
    /**
     * for every node : minimal possible distance between the center of a node
     * and first[layer[node]].
     */
    double* totalB;

    double* mDist; //!< Similar to totalB, used for temporary storage.

    bool m_fixedLayerDist; //!< 0 if distance between layers should be variable, 1 otherwise.
    bool* virt; //!< for every node : virt[node] = 1 if node is virtual, 0 otherwise.

    void incrTo(double & d, double t)
    {
        if(d < t) d = t;
    }

    void decrTo(double & d, double t)
    {
        if(d > t) d = t;
    }

    bool sameLayer(int n1, int n2) const
    {
        return (n1 >= 0 &&
                n1 < n  &&
                n2 >= 0  &&
                n2 < n  &&
                layer[n1] == layer[n2]);
    }

    bool isFirst(int actNode) const
    {
        return (actNode < 0  ||
                actNode >= n ||
                actNode == first[layer[actNode]]);
    }

    bool isLast(int actNode) const
    {
        return (actNode < 0  ||
                actNode >= n ||
                actNode == first[layer[actNode] + 1] - 1);
    }

    void sortLongEdges(int, int, double*, bool &, double &, int*, bool*);
    bool placeSingleNode(int, int, int, double &, int);
    void placeNodes(int, int, int, int, int);
    void moveLongEdge(int, int, bool*);
    void straightenEdge(int, bool*);
    void findPlacement();
};

} // end namespace ogdf


#endif
