/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class BCTree
 *
 * \author Jan Papenfu&szlig;
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

#ifndef OGDF_BC_TREE_H
#define OGDF_BC_TREE_H

#include <ogdf/basic/BoundedStack.h>
#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/SList.h>

namespace ogdf
{

/**
 * \brief Static BC-trees.
 *
 * This class provides static BC-trees.\n
 * The data structure consists of three parts:
 * - The original graph itself (\e G) is represented by an ordinary ogdf::Graph
 *   structure.
 * - The BC-tree (\e B) is represented by an ogdf::Graph structure, each
 *   vertex representing a B-component or a C-component.
 * - The biconnected components graph (\e H), which contains a set of copies of
 *   the biconnected components and the cut-vertices of the original graph,
 *   combined but not interconnected within a single ogdf::Graph structure.
 */
class OGDF_EXPORT BCTree
{

public:

    /** \enum GNodeType
     * \brief Enumeration type for characterizing the vertices of the original
     * graph.
     */
    /** \var GNodeType ogdf::BCTree::Normal
     * denotes an ordinary vertex, i.e. not a cut-vertex.
     */
    /** \var GNodeType ogdf::BCTree::CutVertex
     * denotes a cut-vertex.
     */
    enum GNodeType { Normal, CutVertex };
    /** \enum BNodeType
     * \brief Enumeration type for characterizing the BC-tree-vertices.
     */
    /** \var BNodeType ogdf::BCTree::BComp
     * denotes a vertex representing a B-component.
     */
    /** \var BNodeType ogdf::BCTree::CComp
     * denotes a vertex representing a C-component.
     */
    enum BNodeType { BComp, CComp };

protected:

    /**
     * \brief The original graph.
     */
    Graph & m_G;
    /**
     * \brief The BC-tree.
     *
     * Each vertex is representing a biconnected component (B-component) or a
     * cut-vertex (C-component) of the original graph.
     */
    Graph m_B;
    /**
     * \brief The biconnected components graph.
     *
     * This graph contains copies of the the biconnected components (B-components)
     * and the cut-vertices (C-components) of the original graph. The copies of the
     * B- and C-components of the original graph are not interconnected, i.e. the
     * biconnected components graph is representing B-components as isolated
     * biconnected subgraphs and C-components as isolated single vertices. Thus the
     * copies of the edges and non-cut-vertices of the original graph are
     * unambiguous, but each cut-vertex of the original graph being common to a
     * C-component and several B-components appears multiple times.
     */
    mutable Graph m_H;

    /** @{
     * \brief The number of B-components.
     */
    int m_numB;
    /**
     * \brief The number of C-components.
     */
    int m_numC;

    /** @} @{
     * \brief Array of marks for the vertices of the original graph.
     *
     * They are needed during the generation of the BC-tree by DFS method.
     */
    NodeArray<bool> m_gNode_isMarked;
    /**
     * \brief An injective mapping vertices(\e G) -> vertices(\e H).
     *
     * For each vertex \e vG of the original graph:
     * - If \e vG is not a cut-vertex, then m_gNode_hNode[\e vG] is the very vertex
     *   of the biconnected components graph corresponding to \e vG.
     * - If \e vG is a cut-vertex, then m_gNode_hNode[\e vG] is the very vertex of
     *   the biconnected components graph representing the C-component, which \e vG
     *   is belonging to, as a single isolated vertex.
     */
    NodeArray<node> m_gNode_hNode;
    /**
     * \brief A bijective mapping edges(\e G) -> edges(\e H).
     *
     * For each edge \e eG of the original graph, m_gEdge_hEdge[\e eG] is the very
     * edge of the biconnected components graph corresponding to \e eG.
     */
    EdgeArray<edge> m_gEdge_hEdge;

    /** @} @{
     * \brief Array that contains the type of each BC-tree-vertex.
     */
    NodeArray<BNodeType> m_bNode_type;
    /**
     * \brief Array of marks for the BC-tree-vertices.
     *
     * They are needed for searching for the nearest common ancestor of two
     * vertices of the BC-tree.
     */
    mutable NodeArray<bool> m_bNode_isMarked;
    /**
     * \brief Array that contains for each BC-tree-vertex the representant of its
     * parent within the subgraph in the biconnected components graph belonging to
     * the biconnected component represented by the respective BC-tree-vertex.
     *
     * For each vertex \e vB of the BC-tree:
     * - If \e vB is representing a B-component and \e vB is the root of the
     *   BC-tree, then m_bNode_hRefNode[\e vB] is \e NULL.
     * - If \e vB is representing a B-component and \e vB is not the root of the
     *   BC-tree, then m_bNode_hRefNode[\e vB] is the very vertex of the
     *   biconnected components graph which is the duplicate of the cut-vertex
     *   represented by the parent of \e vB <em>in the copy of the B-component
     *   represented by</em> \e vB.
     * - If \e vB is representing a C-component, then m_bNode_hRefNode[\e vB]
     *   is the single isolated vertex of the biconnected components graph
     *   corresponding to the cut-vertex which the C-component consists of,
     *   irrespective of whether \e vB is the root of the BC-tree or not.
     */
    NodeArray<node> m_bNode_hRefNode;
    /**
     * \brief Array that contains for each BC-tree-vertex the representant of
     * itself within the subgraph in the biconnected components graph belonging to
     * the biconnected component represented by the parent of the respective
     * BC-tree-vertex.
     *
     * - If \e vB is the root of the BC-tree, then m_bNode_hParNode[\e vB] is
     *   \e NULL.
     * - If \e vB is representing a B-component and \e vB is not the root of the
     *   BC-tree, then m_bNode_hParNode[\e vB] is the single isolated vertex
     *   of the biconnected components graph corresponding to the very cut-vertex,
     *   which the C-component represented by <em>the parent of</em> \e vB consists
     *   of.
     * - If \e vB is representing to a C-component and \e vB is not the root of the
     *   BC-tree, then m_bNode_hParNode[\e vB] is the very vertex of the
     *   biconnected components graph, which is the duplicate of the cut-vertex,
     *   which the C-component consists of, <em>in the copy of the B-component
     *   represented by the parent of</em> \e vB.
     */
    NodeArray<node> m_bNode_hParNode;
    /**
     * \brief Array that contains for each BC-tree-vertex a linear list of the
     * edges of the biconnected components graph belonging to the biconnected
     * component represented by the respective BC-tree-vertex.
     *
     * For each vertex \e vB of the BC-tree:
     * - If \e vB is representing a B-component, then m_bNode_hEdges[\e vB] is a
     *   linear list of the edges of the biconnected components graph corresponding
     *   to the edges of the original graph belonging to the B-component.
     * - If \e vB is representing a C-component, then m_bNode_hEdges[\e vB] is an
     *   empty list.
     */
    NodeArray<SList<edge> > m_bNode_hEdges;
    /**
     * \brief Array that contains for each BC-tree-vertex the number of vertices
     * belonging to the biconnected component represented by the respective
     * BC-tree-vertex.
     *
     * For each vertex \e vB of the BC-tree:
     * - If \e vB is representing a B-component, then m_bNode_numNodes[\e vB] is
     *   the number of vertices belonging to the B-component, cut-vertices
     *   inclusive.
     * - If \e vB is representing a C-component, then m_bNode_numNodes[\e vB] is 1.
     */
    NodeArray<int> m_bNode_numNodes;

    /** @} @{
     * \brief A surjective mapping vertices(\e H) -> vertices(\e B).
     *
     * For each vertex \e vH of the biconnected components graph,
     * m_hNode_bNode[\e vH] is the very BC-tree-vertex representing the B- or
     * C-component with respect to the copy of the very block or representation
     * of a cut-vertex, which vH is belonging to.
     */
    mutable NodeArray<node> m_hNode_bNode;
    /**
     * \brief A surjective mapping edges(\e H) -> vertices(\e B).
     *
     * For each edge \e eH of the biconnected components graph,
     * m_hEdge_bNode[\e eH] is the very BC-tree-vertex representing the unambiguous
     * B-component, which \e eH is belonging to.
     */
    mutable EdgeArray<node> m_hEdge_bNode;
    /**
     * \brief A surjective mapping vertices(\e H) -> vertices(\e G).
     *
     * For each vertex \e vH of the biconnected components graph,
     * m_hNode_gNode[\e vH] is the vertex of the original graph which \e vH is
     * corresponding to.
     */
    NodeArray<node> m_hNode_gNode;
    /**
     * \brief A bijective mapping edges(\e H) -> edges(\e G).
     *
     * For each edge \e eH of the biconnected components graph,
     * m_hEdge_gEdge[\e eH] is the edge of the original graph which \e eH is
     * corresponding to.
     */
    EdgeArray<edge> m_hEdge_gEdge;

    /** @} @{
     * \brief Temporary variable.
     *
     * It is needed for the generation of the BC-tree by DFS method. It has to be a
     * member of class BCTree due to recursive calls to biComp().
     */
    int m_count;
    /**
     * \brief Temporary array.
     *
     * It is needed for the generation of the BC-tree by DFS method. It has to be a
     * member of class BCTree due to recursive calls to biComp().
    */
    NodeArray<int> m_number;
    /**
     * \brief Temporary array.
     *
     * It is needed for the generation of the BC-tree by DFS method. It has to be a
     * member of class BCTree due to recursive calls to biComp().
     */
    NodeArray<int> m_lowpt;
    /**
     * \brief Temporary stack.
     *
     * It is needed for the generation of the BC-tree by DFS method. It has to be a
     * member of class BCTree due to recursive calls to biComp().
     */
    BoundedStack<adjEntry> m_eStack;
    /**
     * \brief Temporary array.
     *
     * It is needed for the generation of the BC-tree by DFS method. It has to be a
     * member of class BCTree due to recursive calls to biComp().
     */
    NodeArray<node> m_gtoh;
    /**
     * \brief Temporary list.
     *
     * It is needed for the generation of the BC-tree by DFS method. It has to be a
     * member of class BCTree due to recursive calls to biComp().
     */
    SList<node> m_nodes;

    /** @}
     * \brief Initialization.
     *
     * initializes all data structures and generates the BC-tree and the
     * biconnected components graph by call to biComp().
     * \param vG is the vertex of the original graph which the DFS algorithm starts
     * with.
     */
    void init(node vG);

    /** @}
     * \brief Initialization for not connected graphs
     *
     * initializes all data structures and generates a forest of BC-trees and the
     * biconnected components graph by call to biComp().
     * \param vG is the vertex of the original graph which the DFS algorithm starts
     * first with.
     */
    void initNotConnected(node vG);
    /**
     * \brief generates the BC-tree and the biconnected components graph
     * recursively.
     *
     * The DFS algorithm is based on J. Hopcroft and R. E. Tarjan: Algorithm 447:
     * Efficient algorithms for graph manipulation. <em>Comm. ACM</em>, 16:372-378
     * (1973).
     */
    void biComp(adjEntry adjuG, node vG);

    /** @{
     * \brief returns the parent of a given BC-tree-vertex.
     * \param vB is a vertex of the BC-tree or \e NULL.
     * \return the parent of \a vB in the BC-tree structure, if \a vB is not the
     * root of the BC-tree, and \e NULL, if \a vB is \e NULL or the root of the
     * BC-tree.
     */
    virtual node parent(node vB) const;
    /**
     * \brief calculates the nearest common ancestor of two vertices of the
     * BC-tree.
     * \param uB is a vertex of the BC-tree.
     * \param vB is a vertex of the BC-tree.
     * \return the nearest common ancestor of \a uB and \a vB.
     */
    node findNCA(node uB, node vB) const;

public:

    /** @}
     * \brief A constructor.
     *
     * This constructor does only call init() or initNotConnected().
     * BCTree(\a G) is equivalent to BCTree(<em>G</em>,<em>G</em>.firstNode()).
     * \param G is the original graph.
     * \param callInitConnected decides which init is called, default call is init()
     */
    BCTree(Graph & G, bool callInitConnected = false) : m_G(G), m_eStack(G.numberOfEdges())
    {
        if(!callInitConnected)
            init(G.firstNode());
        else initNotConnected(G.firstNode());
    }

    /**
     * \brief A constructor.
     *
     * This constructor does only call init() or initNotConnected().
     * \param G is the original graph.
     * \param vG is the vertex of the original graph which the DFS algorithm starts
     * \param callInitConnected decides which init is called, default call is init()
     */
    BCTree(Graph & G, node vG, bool callInitConnected = false) : m_G(G), m_eStack(G.numberOfEdges())
    {
        if(!callInitConnected)
            init(vG);
        else initNotConnected(vG);
    }

    /**
     * \brief Virtual destructor.
     */
    virtual ~BCTree() { }

    /** @{
     * \brief returns the original graph.
     * \return the original graph.
     */
    const Graph & originalGraph() const
    {
        return m_G;
    }
    /**
     * \brief returns the BC-tree graph.
     * \return the BC-tree graph.
     */
    const Graph & bcTree() const
    {
        return m_B;
    }
    /**
     * \brief returns the biconnected components graph.
     * \return the biconnected components graph.
     */
    const Graph & auxiliaryGraph() const
    {
        return m_H;
    }

    /** @} @{
     * \brief returns the number of B-components.
     * \return the number of B-components.
     */
    int numberOfBComps() const
    {
        return m_numB;
    }
    /**
     * \brief returns the number of C-components.
     * \return the number of C-components.
     */
    int numberOfCComps() const
    {
        return m_numC;
    }

    /** @} @{
     * \brief returns the type of a vertex of the original graph.
     * \param vG is a vertex of the original graph.
     * \return the type of \a vG.
     */
    GNodeType typeOfGNode(node vG) const
    {
        return m_bNode_type[m_hNode_bNode[m_gNode_hNode[vG]]] == BComp ? Normal : CutVertex;
    }
    /**
     * \brief returns a BC-tree-vertex representing a biconnected component which a
     * given vertex of the original graph is belonging to.
     * \param vG is a vertex of the original graph.
     * \return a vertex of the BC-tree:
     * - If \a vG is not a cut-vertex, then typeOfGNode(\a vG) returns the very
     *   vertex of the BC-tree representing the unambiguous B-component which \a vG
     *   is belonging to.
     * - If \a vG is a cut-vertex, then typeOfGNode(\a vG) returns the very vertex
     *   of the BC-tree representing the unambiguous C-component which \a vG is
     *   belonging to.
     */
    virtual node bcproper(node vG) const
    {
        return m_hNode_bNode[m_gNode_hNode[vG]];
    }
    /**
     * \brief returns the BC-tree-vertex representing the biconnected component
     * which a given edge of the original graph is belonging to.
     * \param eG is an edge of the original graph.
     * \return the vertex of the BC-tree representing the B-component which \a eG
     * is belonging to.
     */
    virtual node bcproper(edge eG) const
    {
        return m_hEdge_bNode[m_gEdge_hEdge[eG]];
    }
    /**
     * \brief returns a vertex of the biconnected components graph corresponding to
     * a given vertex of the original graph.
     * \param vG is a vertex of the original graph.
     * \return a vertex of the biconnected components graph:
     * - If \a vG is not a cut-vertex, then rep(\a vG) returns the very vertex of
     *   the biconnected components graph corresponding to \a vG.
     * - If \a vG is a cut-vertex, then rep(\a vG) returns the very vertex of the
     *   biconnected components graph representing the C-component which \a vG is
     *   belonging to.
     */
    node rep(node vG) const
    {
        return m_gNode_hNode[vG];
    }
    /**
     * \brief returns the edge of the biconnected components graph corresponding to
     * a given edge of the original graph.
     * \param eG is an edge of the original graph.
     * \return the edge of the biconnected components graph corresponding to \a eG.
     */
    edge rep(edge eG) const
    {
        return m_gEdge_hEdge[eG];
    }

    /** @} @{
     * \brief returns the vertex of the original graph which a given vertex of the
     * biconnected components graph is corresponding to.
     * \param vH is a vertex of the biconnected components graph.
     * \return the vertex of the original graph which \a vH is corresponding to.
     */
    node original(node vH)
    {
        return m_hNode_gNode[vH];
    }
    /**
     * \brief returns the edge of the original graph which a given edge of the
     * biconnected components graph is corresponding to.
     * \param eH is an edge of the biconnected components graph.
     * \return the edge of the original graph which \a eH is corresponding to.
     */
    edge original(edge eH) const
    {
        return m_hEdge_gEdge[eH];
    }

    /** @} @{
     * \brief returns the type of the biconnected component represented by a given
     * BC-tree-vertex.
     * \param vB is a vertex of the BC-tree.
     * \return the type of the biconnected component represented by \a vB.
     */
    BNodeType typeOfBNode(node vB) const
    {
        return m_bNode_type[vB];
    }
    /**
     * \brief returns a linear list of the edges of the biconnected components
     * graph belonging to the biconnected component represented by a given
     * BC-tree-vertex.
     * \param vB is a vertex of the BC-tree.
     * \return a linear list of edges of the biconnected components graph:
     * - If \a vB is representing a B-component, then the edges in the list are the
     *   copies of the edges belonging to the B-component.
     * - If \a vB is representing a C-component, then the list is empty.
     */
    const SList<edge> & hEdges(node vB) const
    {
        return m_bNode_hEdges[vB];
    }
    /**
     * \brief returns the number of edges belonging to the biconnected component
     * represented by a given BC-tree-vertex.
     * \param vB is a vertex of the BC-tree.
     * \return the number of edges belonging to the B- or C-component represented
     * by \a vB, particularly 0 if it is a C-component.
     */
    int numberOfEdges(node vB) const
    {
        return m_bNode_hEdges[vB].size();
    }
    /**
     * \brief returns the number of vertices belonging to the biconnected component
     * represented by a given BC-tree-vertex.
     * \param vB is a vertex of the BC-tree.
     * \return the number of vertices belonging to the B- or C-component
     * represented by \a vB, cut-vertices inclusive, particularly 1 if it is a
     * C-component.
     */
    int numberOfNodes(node vB) const
    {
        return m_bNode_numNodes[vB];
    }

    /** @} @{
     * \brief returns the BC-tree-vertex representing the B-component which two
     * given vertices of the original graph are belonging to.
     * \param uG is a vertex of the original graph.
     * \param vG is a vertex of the original graph.
     * \return If \a uG and \a vG are belonging to the same B-component, the very
     * vertex of the BC-tree representing this B-component is returned. Otherwise,
     * \e NULL is returned. This member function returns the representant of the
     * correct B-component even if \a uG or \a vG or either are cut-vertices and
     * are therefore belonging to C-components, too.
     */
    node bComponent(node uG, node vG) const;

    /**
     * \brief calculates a path in the BC-tree.
     * \param sG is a vertex of the original graph.
     * \param tG is a vertex of the original graph.
     * \return the path from bcproper(\a sG) to bcproper(\a tG) in the BC-tree as a
     * linear list of vertices.
     * \post <b>The SList<node> instance is created by this function and has to be
     * destructed by the caller!</b>
     */
    SList<node> & findPath(node sG, node tG) const;

    /**
     * \brief calculates a path in the BC-tree.
     * \param sB is a vertex of the BC-tree.
     * \param tB is a vertex of the BC-tree.
     * \return the path from (\a sB) to bcproper(\a tB) in the BC-tree as a
     * linear list of vertices.
     * \post <b>The SList<node> instance is created by this function and has to be
     * destructed by the caller!</b>
     */
    SList<node>* findPathBCTree(node sB, node tB) const;

    /**
     * \brief returns a vertex of the biconnected components graph corresponding to
     * a given vertex of the original graph and belonging to the representation of
     * a certain biconnected component given by a vertex of the BC-tree.
     * \param uG is a vertex of the original graph.
     * \param vB is a vertex of the BC-tree.
     * \return a vertex of the biconnected components graph:
     * - If \a uG is belonging to the biconnected component represented by \a vB,
     *   then repVertex(\a uG,\a vB) returns the very vertex of the biconnected
     *   components graph corresponding to \a uG within the representation of
     *   \a vB.
     * - Otherwise, repVertex(\a uG,\a vB) returns \e NULL.
     */
    virtual node repVertex(node uG, node vB) const;
    /**
     * \brief returns the copy of a cut-vertex in the biconnected components graph
     * which belongs to a certain B-component and leads to another B-component.
     *
     * If two BC-tree-vertices are neighbours, then the biconnected components
     * represented by them have exactly one cut-vertex in common. But there are
     * several copies of this cut-vertex in the biconnected components graph,
     * namely one copy for each biconnected component which the cut-vertex is
     * belonging to. The member function rep() had been designed for returning the
     * very copy of the cut-vertex belonging to the copy of the unambiguous
     * C-component which it is belonging to, whereas this member function is
     * designed to return the very copy of the cut-vertex connecting two
     * biconnected components which belongs to the copy of the second one.
     * \param uB is a vertex of the BC-tree.
     * \param vB is a vertex of the BC-tree.
     * \return a vertex of the biconnected components graph:
     * - If \a uB == \a vB and they are representing a B-component, then
     *   cutVertex(\a uB,\a vB) returns \e NULL.
     * - If \a uB == \a vB and they are representing a C-component, then
     *   cutVertex(\a uB,\a vB) returns the single isolated vertex of the
     *   biconnected components graph which is the copy of the C-component.
     * - If \a uB and \a vB are \e neighbours in the BC-tree, then there exists
     *   a cut-vertex leading from the biconnected component represented by \a vB
     *   to the biconnected component represented by \a uB. cutVertex(\a uB,\a vB)
     *   returns the very copy of this vertex within the biconnected components
     *   graph which belongs to the copy of the biconnected component represented
     *   by \a vB.
     * - Otherwise, cutVertex(\a uB,\a vB) returns \e NULL.
     */
    virtual node cutVertex(node uB, node vB) const;

    /** @}
     */
private:
    // avoid automatic creation of assignment operator

    //! Copy constructor is undefined!
    BCTree(const BCTree &);

    //! Assignment operator is undefined!
    BCTree & operator=(const BCTree &);
};

}

#endif
