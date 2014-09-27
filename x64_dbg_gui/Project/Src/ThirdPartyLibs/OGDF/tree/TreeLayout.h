/*
 * $Revision: 2583 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-12 01:02:21 +0200 (Do, 12. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of linear time layout algorithm for trees
 *        (TreeLayout) based on Walker's algorithm.
 *
 * \author Christoph Buchheim
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

#ifndef OGDF_TREE_LAYOUT_H
#define OGDF_TREE_LAYOUT_H

#include <ogdf/module/LayoutModule.h>
#include <ogdf/basic/SList.h>

namespace ogdf
{

/**
 * \brief The tree layout algorithm.
 *
 * The class TreeLayout represents the improved version of the tree layout
 * algorithm by Walker presented in:
 *
 * Christoph Buchheim, Michael J&uuml;nger, Sebastian Leipert: <i>Drawing
 * rooted trees in linear time</i>. Software: Practice and Experience 36(6),
 * pp. 651-665, 2006.
 *
 * The algorithm also allows to lay out a forest, i.e., a collection of trees.
 *
 * <H3>Optional parameters</H3>
 * Tree layout provides various optional parameters.
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>siblingDistance</i><td>double<td>20.0
 *     <td>The horizontal spacing between adjacent sibling nodes.
 *   </tr><tr>
 *     <td><i>subtreeDistance</i><td>double<td>20.0
 *     <td>The horizontal spacing between adjacent subtrees.
 *   </tr><tr>
 *     <td><i>levelDistance</i><td>double<td>50.0
 *     <td>The vertical spacing between adjacent levels.
 *   </tr><tr>
 *     <td><i>treeDistance</i><td>double<td>50.0
 *     <td>The horizontal spacing between adjacent trees in a forest.
 *   </tr><tr>
 *     <td><i>orthogonalLayout</i><td>bool<td>false
 *     <td>Determines whether edges are routed in an orthogonal
 *     or straight-line fashion.
 *   </tr><tr>
 *     <td><i>orientation</i><td> #Orientation <td> #topToBottom
 *     <td>Determines if the tree is laid out in a top-to-bottom,
 *     bottom-to-top, left-to-right, or right-to-left fashion.
 *   </tr><tr>
 *     <td><i>selectRoot</i><td> #RootSelectionType <td> #rootIsSource
 *     <td>Determines how to select the root of the tree(s). Possible
 *     selection strategies are to take a (unique) source or sink in
 *     the graph, or to use the coordinates and to select the topmost
 *     node for top-to-bottom orientation, etc.
 *   </tr>
 * </table>
 *
 * The spacing between nodes is determined by the <i>siblingDistance</i>,
 * <i>subtreeDistance</i>, <i>levelDistance</i>, and <i>treeDistance</i>.
 * The layout style is determined by <i>orthogonalLayout</i> and
 * <i>orientation</i>; the root of the tree is selected according to
 * th eselection strategy given by <i>selectRoot</i>.
 */
class OGDF_EXPORT TreeLayout : public LayoutModule
{
public:
    //! Determines how to select the root of the tree.
    enum RootSelectionType
    {
        rootIsSource, //!< Select a source in the graph.
        rootIsSink,   //!< Select a sink in the graph.
        rootByCoord   //!< Use the coordinates, e.g., select the topmost node if orientation is topToBottom.
    };

private:
    double m_siblingDistance;        //!< The minimal distance between siblings.
    double m_subtreeDistance;        //!< The minimal distance between subtrees.
    double m_levelDistance;          //!< The minimal distance between levels.
    double m_treeDistance;           //!< The minimal distance between trees.

    bool m_orthogonalLayout;         //!< Option for orthogonal style (yes/no).
    Orientation m_orientation;       //!< Option for orientation of tree layout.
    RootSelectionType m_selectRoot;  //!< Option for how to determine the root.

    NodeArray<int> m_number;         //!< Consecutive numbers for children.

    NodeArray<node> m_parent;        //!< Parent node, 0 if root.
    NodeArray<node> m_leftSibling;   //!< Left sibling, 0 if none.
    NodeArray<node> m_firstChild;    //!< Leftmost child, 0 if leaf.
    NodeArray<node> m_lastChild;     //!< Rightmost child, 0 if leaf.
    NodeArray<node> m_thread;        //!< Thread, 0 if none.
    NodeArray<node> m_ancestor;      //!< Actual highest ancestor.

    NodeArray<double> m_preliminary; //!< Preliminary x-coordinates.
    NodeArray<double> m_modifier;    //!< Modifier of x-coordinates.
    NodeArray<double> m_change;      //!< Change of shift applied to subtrees.
    NodeArray<double> m_shift;       //!< Shift applied to subtrees.

    SListPure<edge> m_reversedEdges; //!< List of temporarily removed edges.
    Graph*           m_pGraph; //!< The input graph.

public:
    //! Creates an instance of tree layout and sets options to default values.
    TreeLayout();

    //! Copy constructor.
    TreeLayout(const TreeLayout & tl);

    // destructor
    ~TreeLayout();


    /**
     *  @name Algorithm call
     *  @{
     */

    /**
     * \brief Calls tree layout for graph attributes \a GA.
     *
     * \pre The graph is a tree.
     *
     * The order of children is given by the adjacency lists;
     * the successor of the unique in-edge of a non-root node
     * leads to its leftmost child; the leftmost child of the root
     * is given by its first adjacency entry.
     * @param GA is the input graph and will also be assigned the layout information.
     */
    void call(GraphAttributes & GA);

    /**
     * \brief Calls tree layout for graph attributes \a GA.
     *
     * \pre The graph is a tree.
     *
     * Sorts the adjacency entries according to the positions of adjacent
     * vertices in \a GA.
     * @param GA is the input graph and will also be assigned the layout information.
     * @param G is the graph associated with \a GA.
     */
    void callSortByPositions(GraphAttributes & GA, Graph & G);


    /** @}
     *  @name Optional parameters
     *  @{
     */

    //! Returns the the minimal required horizontal distance between siblings.
    double siblingDistance() const
    {
        return m_siblingDistance;
    }

    //! Sets the the minimal required horizontal distance between siblings to \a x.
    void siblingDistance(double x)
    {
        m_siblingDistance = x;
    }

    //! Returns the minimal required horizontal distance between subtrees.
    double subtreeDistance() const
    {
        return m_subtreeDistance;
    }

    //! Sets the minimal required horizontal distance between subtrees to \a x.
    void subtreeDistance(double x)
    {
        m_subtreeDistance = x;
    }

    //! Returns the minimal required vertical distance between levels.
    double levelDistance() const
    {
        return m_levelDistance;
    }

    //! Sets the minimal required vertical distance between levels to \a x.
    void levelDistance(double x)
    {
        m_levelDistance = x;
    }

    //! Returns the minimal required horizontal distance between trees in the forest.
    double treeDistance() const
    {
        return m_treeDistance;
    }

    //! Sets the minimal required horizontal distance between trees in the forest to \a x.
    void treeDistance(double x)
    {
        m_treeDistance = x;
    }

    //! Returns whether orthogonal edge routing style is used.
    bool orthogonalLayout() const
    {
        return m_orthogonalLayout;
    }

    //! Sets the option for orthogonal edge routing style to \a b.
    void orthogonalLayout(bool b)
    {
        m_orthogonalLayout = b;
    }

    //! Returns the option that determines the orientation of the layout.
    Orientation orientation() const
    {
        return m_orientation;
    }

    //! Sets the option that determines the orientation of the layout to \a orientation.
    void orientation(Orientation orientation)
    {
        m_orientation = orientation;
    }

    //! Returns the option that determines how the root is selected.
    RootSelectionType rootSelection() const
    {
        return m_selectRoot;
    }

    //! Sets the option that determines how the root is selected to \a rootSelection.
    void rootSelection(RootSelectionType rootSelection)
    {
        m_selectRoot = rootSelection;
    }


    /** @}
     *  @name Operators
     *  @{
     */

    //! Assignment operator.
    TreeLayout & operator=(const TreeLayout & tl);

    //! @}

private:
    class AdjComparer;

    void adjustEdgeDirections(Graph & G, node v, node parent);
    void setRoot(GraphAttributes & AG, Graph & tree);
    void undoReverseEdges(GraphAttributes & AG);

    // initialize all node arrays and
    // compute the tree structure from the adjacency lists
    //
    // returns the root node
    void initializeTreeStructure(const Graph & tree, List<node> & roots);

    // delete all node arrays
    void deleteTreeStructure();

    // returns whether node v is a leaf
    int isLeaf(node v) const;

    // returns the successor of node v on the left/right contour
    // returns 0 if there is none
    node nextOnLeftContour(node v) const;
    node nextOnRightContour(node v) const;

    // recursive bottom up traversal of the tree for computing
    // preliminary x-coordinates
    void firstWalk(node subtree, const GraphAttributes & AG, bool upDown);

    // space out the small subtrees on the left hand side of subtree
    // defaultAncestor is used for all nodes with obsolete m_ancestor
    void apportion(
        node subtree,
        node & defaultAncestor,
        const GraphAttributes & AG,
        bool upDown);

    // recursive top down traversal of the tree for computing final
    // x-coordinates
    void secondWalkX(node subtree, double modifierSum, GraphAttributes & AG);
    void secondWalkY(node subtree, double modifierSum, GraphAttributes & AG);

    // compute y-coordinates and edge shapes
    void computeYCoordinatesAndEdgeShapes(node root, GraphAttributes & AG);
    void computeXCoordinatesAndEdgeShapes(node root, GraphAttributes & AG);

    void findMinX(GraphAttributes & AG, node root, double & minX);
    void findMinY(GraphAttributes & AG, node root, double & minY);
    void findMaxX(GraphAttributes & AG, node root, double & maxX);
    void findMaxY(GraphAttributes & AG, node root, double & maxY);
    void shiftTreeX(GraphAttributes & AG, node root, double shift);
    void shiftTreeY(GraphAttributes & AG, node root, double shift);
};

} // end namespace ogdf

#endif

