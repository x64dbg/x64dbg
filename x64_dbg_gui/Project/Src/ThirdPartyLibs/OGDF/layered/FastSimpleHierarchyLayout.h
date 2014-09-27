/*
 * $Revision: 2583 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-12 01:02:21 +0200 (Do, 12. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief declaration of the FastSimpleHierarchyLayout
 * (third phase of sugiyama)
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

#ifndef OGDF_FAST_SIMPLE_LAYOUT_H
#define OGDF_FAST_SIMPLE_LAYOUT_H


#include <ogdf/module/HierarchyLayoutModule.h>
#include <ogdf/layered/Hierarchy.h>
#include <ogdf/basic/NodeArray.h>

namespace ogdf
{

/**
 * \brief Coordinate assignment phase for the Sugiyama algorithm by Ulrik Brandes and Boris K&ouml;pf
 *
 * This class implements a hierarchy layout algorithm, i.e., it layouts
 * hierarchies with a given order of nodes on each layer. It is used as a third
 * phase of the Sugiyama algorithm.
 *
 * The Algorithm runs in three phases.<br>
 * - Alignment (4x)<br>
 * - Horizontal Compactation (4x)<br>
 * - Balancing
 *
 * The <i>Alignment</i> and <i>Horzontal Compactation</i> phase are calculated downward, upward,
 * leftToRight and rightToLeft. The four resulting layouts are combined in a balancing step.
 *
 * <b>Warning:</b> The implementation is known to not always produce a correct layout.
 * Therefore this Algorithm is for testing purpose only.
 *
 * The implementation is based on:
 *
 * Ulrik Brandes, Boris K&ouml;pf: <i>Fast and Simple Horizontal Coordinate Assignment</i>.
 * LNCS 2002, Volume 2265/2002, pp. 33-36
 *
 * <h3>Optional Parameters</h3>
 *
 * <table>
 *   <tr>
 *     <th>Option</th><th>Type</th><th>Default</th><th>Description</th>
 *   </tr><tr>
 *     <td><i>minimal x separation</i></td><td>int</td><td>150</td>
 *     <td>the minimal horizontal distance between two nodes on the same layer</td>
 *   </tr><tr>
 *     <td><i>layer distance</i></td><td>int</td><td>75</td>
 *     <td>the minimal vertical distance between two nodes on adjacent layers</td>
 *   </tr>
 * </table>
 */
class OGDF_EXPORT FastSimpleHierarchyLayout : public HierarchyLayoutModule
{
private:
    int m_minXSep;
    int m_ySep;
    bool m_balanced;
    bool m_downward;
    bool m_leftToRight;

protected:
    void doCall(const Hierarchy & H, GraphCopyAttributes & AGC);

public:
    /**
     * Constructor for balanced layout. This is usually the best choice!
     *
     * @param minXSep Mimimum separation between each node in x-direction.
     * @param ySep Distance between adjacent layers in y-direction.
     */
    FastSimpleHierarchyLayout(int minXSep = 150, int ySep = 75);

    /**
     * Constructor for specific unbalanced layout.
     * This is for scientific purpose and debugging. If you are not sure then use the other Constructor
     *
     * @param downward The level direction
     * @param leftToRight The node direction on each level
     * @param ySep Distance between adjacent layers in y-direction.
     * @param minXSep Mimimum separation between nodes in x-direction.
     */
    FastSimpleHierarchyLayout(bool downward, bool leftToRight, int minXSep = 150, int ySep = 75);

    //! Copy constructor.
    FastSimpleHierarchyLayout(const FastSimpleHierarchyLayout &);

    // destructor
    virtual ~FastSimpleHierarchyLayout();


    //! Assignment operator
    FastSimpleHierarchyLayout & operator=(const FastSimpleHierarchyLayout &);


private:
    /**
     * Preprocessing step to find all type1 conflicts.
     * A type1 conflict is a crossing of a inner segment with a non-inner segment.
     *
     * This is for preferring straight inner segments.
     *
     * @param H The Hierarchy
     * @param downward The level direction
     * @return (type1Conflicts[v])[u]=true means (u,v) is marked, u is the upper node
     */
    NodeArray<NodeArray<bool> > markType1Conflicts(const Hierarchy & H, bool downward);

    /**
     * Align each node to a node on the next higher level. The result is a blockgraph where each
     * node is in a block whith a nother node when they have the same root.
     *
     * @param H The Hierarchy
     * @param root The root for each node (calculated by this method)
     * @param align The alignment to the next level node (align(v)=u <=> u is aligned to v) (calculated by this method)
     * @param type1Conflicts Type1 conflicts to prefer straight inner segments
     * @param downward The level direction
     * @param leftToRight The node direction on each level
     */
    void verticalAlignment(
        const Hierarchy & H,
        NodeArray<node> & root,
        NodeArray<node> & align,
        const NodeArray<NodeArray<bool> > & type1Conflicts,
        const bool downward,
        const bool leftToRight);

    /**
     * Calculate the coordinates for each node
     *
     * @param align The alignment to the next level node (align(v)=u <=> u is aligned to v)
     * @param H The Hierarchy
     * @param root The root for each node
     * @param x The x-coordinates for each node (calculated by this method)
     * @param leftToRight The node direction on each level
     * @param downward The level direction
     */
    void horizontalCompactation(
        const NodeArray<node> & align,
        const Hierarchy & H,
        const NodeArray<node> root,
        NodeArray<int> & x,
        const bool leftToRight,
        bool downward);

    /**
     * Calculate the coordinate for root nodes (placing)
     *
     * @param v The root node to place
     * @param sink The Sink for each node. A sink identifies each block class (calculated by this method)
     * @param shift The shift for each class (calculated by this method)
     * @param x The class relative x-coordinate for each node (calculated by this method)
     * @param align The alignment to the next level node (align(v)=u <=> u is aligned to v)
     * @param H The Hierarchy
     * @param root The root for each node
     * @param leftToRight The node direction on each level
     */
    void placeBlock(node v, NodeArray<node> & sink, NodeArray<int> & shift,
                    NodeArray<int> & x, const NodeArray<node> & align,
                    const Hierarchy & H, const NodeArray<node> & root, const bool leftToRight);

    /**
     * The twin of an inner Segment
     *
     * @return Parent node which is connected by an inner segment.
     * NULL if there is no parent segment or if the segment is not an inner segment.
     */
    node virtualTwinNode(const Hierarchy & H, const node v, const Hierarchy::TraversingDir dir) const;

    /**
     * Predecessor of v on the same level,
     *
     * @param v The node for which the predecessor should be calculated.
     * @param H The Hierarchy
     * @param leftToRight If true the left predecessor is choosen. Otherwise the right predecessor.
     * @return Predescessor on the same level. NULL if there is no predecessor.
     */
    node pred(const node v, const Hierarchy & H, const bool leftToRight);
};

} // end namespace ogdf


#endif
