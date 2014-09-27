/*
 * $Revision: 2583 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-12 01:02:21 +0200 (Do, 12. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class BalloonLayout. Computes
 * a radial (balloon) layout based on a spanning tree.
 * The algorithm is partially based on the paper
 * "On Balloon Drawings of Rooted Trees" by Lin and Yen
 * and on
 * "Interacting with Huge Hierarchies: Beyond Cone Trees"
 * by Carriere and Kazman
 *
 * \author Karsten Klein;
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
/*
 * The layout is computed by first computing a spanning tree
 * of the graph that is then used to derive the vertices' coordinates.
 * First, the radii at each vertex are computed.
 * Then, depending on the embedding option, the order of the
 * edges around each vertex is optimized to maximize angular
 * resolution and to minimize the aspect ratio.
 *
 * Finally, the layout is shifted into the positive quadrant
 * of the cartesian plane
 * */
#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_BALLOON_LAYOUT_H_
#define OGDF_BALLOON_LAYOUT_H_

#include <ogdf/module/LayoutModule.h>
#include <ogdf/basic/List.h>


namespace ogdf
{

class OGDF_EXPORT BalloonLayout : public LayoutModule
{
public:
    //Root may be defined by center of the graph
    //Directed cases: source/sink
    enum RootSelection {rootCenter, rootHighestDegree};
    //either keep the given embedding or optimize
    //the order wrto angular resolution and minimum aspect ratio
    enum ChildOrder {orderFixed, orderOptimized};
    //compute tree by different methods
    enum TreeComputation {treeBfs, treeDfs, treeBfsRandom};
    //! Constructor, sets options to default values.
    BalloonLayout();
    virtual ~BalloonLayout();
    //! Assignmentoperator.
    BalloonLayout & operator=(const BalloonLayout & bl);

    //! Standard call using the stored parameter settings.
    virtual void call(GraphAttributes & AG);

    /** Call using special parameter settings for fractal model
     * takes radius ratio < 0.5 as parameter.
     */
    virtual void callFractal(GraphAttributes & AG, double ratio = 0.3)
    {
        bool even = getEvenAngles();
        setEvenAngles(true);
        call(AG);
        setEvenAngles(even);
    }

    /// Subtrees may be assigned even angles or angles depending on their size.
    void setEvenAngles(bool b)
    {
        m_evenAngles = b;
    }
    /// returns how the angles are assigned to subtrees.
    bool getEvenAngles()
    {
        return m_evenAngles;
    }


protected:
    //! Computes the spanning tree that is used for the
    //! layout computation, the non-tree edges are
    //! simply added into the layout.
    void computeTree(const Graph & G);
    //! Computes tree by BFS, fills m_parent and m_childCount.
    void computeBFSTree(const Graph & G, node v);
    //! Selects the root of the spanning tree that
    //! is placed in the layout center.
    void selectRoot(const Graph & G);
    //------------------------------------------------
    //! Computes a radius for each of the vertices in G.
    //! fractal model: same radius on same level, such
    //! that r(m) = gamma* r(m-1) where gamma is predefined
    //! SNS model: different radii possible
    //! Optimal: unordered tree, order of children is optimized.
#ifdef OGDF_DEBUG
    void computeRadii(GraphAttributes & AG);
#else
    void computeRadii(const GraphAttributes & AG);
#endif
    //! Computes the angle distribution: assigns m_angle each node.
    void computeAngles(const Graph & G);
    //! Computes coordinates from angles and radii.
    void computeCoordinates(GraphAttributes & AG);

private:
    NodeArray<double> m_radius; //! Radius at node center.
    NodeArray<double> m_oRadius; //!< Outer radius enclosing all children.
    NodeArray<double> m_maxChildRadius; //!< Outer radius of largest child.
    NodeArray<node>   m_parent; //!< Parent in spanning tree.
    NodeArray<int>    m_childCount; //!< Number of children in spanning tree.
    NodeArray<double> m_angle; //!< Angle assigned to nodes.
    NodeArray<double> m_estimate; //!< Rough estimate of circumference of subtrees.
    NodeArray<double> m_size;   //!< Radius of circle around node box.

    NodeArray< List<node> > m_childList;
#ifdef OGDF_DEBUG
    //! Consistency check for the tree.
    void checkTree(const Graph & G, bool treeRoot = true);
    EdgeArray<bool>* m_treeEdge; //!< Holds info about tree edges.
#endif

    //-----------------------
    //optimization parameters
    RootSelection     m_rootSelection; //!< Defines how the tree root is selected
    node              m_treeRoot; //!< Root of tree after computation.
    node              m_root;     //!< Root of tree by selection method.

    double            m_estimateFactor; //!< Weight of value (largestchild / number of children) added to
    // estimate to compute radius.
    double            m_fractalRatio;   //!< Ratio of neighbor radii in fractal model.

    ChildOrder        m_childOrder; //!< How to arrange the children.
    TreeComputation   m_treeComputation; //!< How to derive the spanning tree.
    bool              m_evenAngles; //! Use even angles independent of subtree size.

    void check(Graph & G);

    OGDF_NEW_DELETE
}; //class BalloonLayout

}//end namespace ogdf
#endif /*BALLOONLAYOUT_H_*/
