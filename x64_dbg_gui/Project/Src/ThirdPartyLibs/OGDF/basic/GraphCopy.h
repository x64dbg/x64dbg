/*
 * $Revision: 2583 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-12 01:02:21 +0200 (Do, 12. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of graph copy classes.
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

#ifndef OGDF_GRAPH_COPY_H
#define OGDF_GRAPH_COPY_H


#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/SList.h>
#include <ogdf/basic/CombinatorialEmbedding.h>


namespace ogdf
{

class FaceSetPure;


//---------------------------------------------------------
// GraphCopySimple
// simple graph copies (no support for edge splitting)
//---------------------------------------------------------
/**
 * \brief Copies of graphs with mapping between nodes and edges
 *
 * The class GraphCopySimple represents a copy of a graph and
 * maintains a mapping between the nodes and edges of the original
 * graph to the copy and vice versa.
 *
 * New nodes and edges can be added to the copy; the counterpart
 * of those nodes and edges is 0 indicating that there is no counterpart.
 * This class <b>does not</b> support splitting of edges in such a way
 * that both edges resulting from the split are mapped to the same
 * original edge; this feature is provided by GraphCopy.
 */
class OGDF_EXPORT GraphCopySimple : public Graph
{
    const Graph* m_pGraph;   //!< The original graph.
    NodeArray<node> m_vOrig; //!< The corresponding node in the original graph.
    NodeArray<node> m_vCopy; //!< The corresponding node in the graph copy.
    EdgeArray<edge> m_eOrig; //!< The corresponding edge in the original graph.
    EdgeArray<edge> m_eCopy; //!< The corresponding edge in the graph copy.

public:
    // construction

    //! Constructs a copy of graph \a G.
    GraphCopySimple(const Graph & G);

    //! Copy constructor.
    GraphCopySimple(const GraphCopySimple & GC);

    virtual ~GraphCopySimple() { }

    //! Returns a reference to the original graph.
    const Graph & original() const
    {
        return *m_pGraph;
    }

    /**
     * \brief Returns the node in the original graph corresponding to \a v.
     * @param v is a node in the graph copy.
     * \return the corresponding node in the original graph, or 0 if no
     *         such node exists.
     */
    node original(node v) const
    {
        return m_vOrig[v];
    }

    /**
     * \brief Returns the edge in the original graph corresponding to \a e.
     * @param e is an edge in the graph copy.
     * \return the corresponding edge in the original graph, or 0 if no
     *         such edge exists.
     */
    edge original(edge e) const
    {
        return m_eOrig[e];
    }

    /**
     * \brief Returns the node in the graph copy corresponding to \a v.
     * @param v is a node in the original graph.
     * \return the corresponding node in the graph copy.
     */
    node copy(node v) const
    {
        return m_vCopy[v];
    }

    /**
     * \brief Returns the edge in the graph copy corresponding to \a e.
     * @param e is an edge in the original graph.
     * \return the corresponding edge in the graph copy.
     */
    edge copy(edge e) const
    {
        return m_eCopy[e];
    }

    /**
     * \brief Returns true iff \a v has no corresponding node in the original graph.
     * @param v is a node in the graph copy.
     */
    bool isDummy(node v) const
    {
        return (m_vOrig[v] == 0);
    }

    /**
     * \brief Returns true iff \a e has no corresponding edge in the original graph.
     * @param e is an edge in the graph copy.
     */
    bool isDummy(edge e) const
    {
        return (m_eOrig[e] == 0);
    }

    //! Assignment operator.
    GraphCopySimple & operator=(const GraphCopySimple & GC);


    //! Creates a new node in the graph copy.
    node newNode()
    {
        return Graph::newNode();
    }

    /**
     * \brief Creates a new node in the graph copy with original node \a vOrig.
     * \warning You have to make sure that the original node makes sense, in
     *   particular that \a vOrig is not the original node of another node in the copy.
     */
    node newNode(node vOrig)
    {
        OGDF_ASSERT(vOrig != 0 && vOrig->graphOf() == m_pGraph);
        node v = Graph::newNode();
        m_vCopy[m_vOrig[v] = vOrig] = v;
        return v;
    }

    //! Creates a new edge from \a v to \a w in the graph copy.
    edge newEdge(node v, node w)
    {
        return Graph::newEdge(v, w);
    }

    /**
     * \brief Creates a new edge in the graph copy with original edge \a eOrig.
     * \warning You have to make sure that the original edge makes sense, in
     *   particular that \a eOrig is not the original edge of another edge in the copy.
     */
    edge newEdge(edge eOrig)
    {
        OGDF_ASSERT(eOrig != 0 && eOrig->graphOf() == m_pGraph);
        edge e = Graph::newEdge(m_vCopy[eOrig->source()], m_vCopy[eOrig->target()]);
        m_eCopy[m_eOrig[e] = eOrig] = e;
        return e;
    }

private:
    void initGC(const GraphCopySimple & GC, NodeArray<node> & vCopy,
                EdgeArray<edge> & eCopy);
}; // class GraphCopySimple


//---------------------------------------------------------
// GraphCopy
// graph copies (support for edge splitting)
//---------------------------------------------------------
/**
 * \brief Copies of graphs supporting edge splitting
 *
 * The class GraphCopy represents a copy of a graph and
 * maintains a mapping between the nodes and edges of the original
 * graph to the copy and vice versa.
 *
 * New nodes and edges can be added to the copy; the counterpart
 * of those nodes and edges is 0 indicating that there is no counterpart.
 * GraphCopy also support splitting of edges in such a way
 * that both edges resulting from the split are mapped to the same
 * original edge, and each edge of the original graph is mapped to a list
 * of edges. Furthermore, it is allowed to reverse edges in the graph copy.
 *
 * <h3>Do's and Dont's</h3>
 * Here is a short summary, what can be done with GraphCopy, and what should not
 * be done. The following operations are safely supported:
 *   - Splitting of edges such that an original edge is represented by a path
 *     in the copy (split(), unsplit()).
 *   - Reversing edges in the copy (Graph::reverseEdge(), Graph::reverseAllEdges()).
 *   - Reinsertion of original edges such that they are represented by a path
 *     in the copy (insertEdgePath(), insertEdgePathEmbedded(), removeEdgePath(),
 *     removeEdgePathEmbedded()).
 *   - Inserting and removing dummy edges in the copy which are not associated
 *     with edges in the original graph.
 *
 * The following operations are <b>not supported</b> and are thus dangerous:
 *   - Any modifications on the original graph, since the copy will not be
 *     notified.
 *   - Moving the source or target node of an edge in the copy to a different node.
 *   - Removing edges in the graph copy that belong to a path representing an
 *     original edge.
 *   - ... (better think first!)
 */
class OGDF_EXPORT GraphCopy : public Graph
{
protected:

    const Graph* m_pGraph;   //!< The original graph.
    NodeArray<node> m_vOrig; //!< The corresponding node in the original graph.
    EdgeArray<edge> m_eOrig; //!< The corresponding edge in the original graph.
    EdgeArray<ListIterator<edge> > m_eIterator; //!< The position of copy edge in the list.

    NodeArray<node> m_vCopy; //!< The corresponding node in the graph copy.
    EdgeArray<List<edge> > m_eCopy; //!< The corresponding list of edges in the graph copy.

public:
    //! Creates a graph copy of \a G.
    /**
     * The constructor assures that the adjacency lists of nodes in the
     * constructed copy are in the same order as the adjacency lists in \a G.
     * This is in particular important when dealing with embedded graphs.
     */
    GraphCopy(const Graph & G);

    //! Default constructor (does nothing!).
    GraphCopy() : Graph() { }

    //! Copy constructor.
    /**
     * Creates a graph copy that is a copy of \a GC and represents a graph
     * copy of the original graph of \a GC.
     */
    GraphCopy(const GraphCopy & GC);

    virtual ~GraphCopy() { }


    /**
     * @name Mapping between original graph and copy
     */
    //@{

    //! Returns a reference to the original graph.
    const Graph & original() const
    {
        return *m_pGraph;
    }

    /**
     * \brief Returns the node in the original graph corresponding to \a v.
     * @param v is a node in the graph copy.
     * \return the corresponding node in the original graph, or 0 if no
     *         such node exists.
     */
    node original(node v) const
    {
        return m_vOrig[v];
    }

    /**
     * \brief Returns the edge in the original graph corresponding to \a e.
     * @param e is an edge in the graph copy.
     * \return the corresponding edge in the original graph, or 0 if no
     *         such edge exists.
     */
    edge original(edge e) const
    {
        return m_eOrig[e];
    }

    /**
     * \brief Returns the node in the graph copy corresponding to \a v.
     * @param v is a node in the original graph.
     * \return the corresponding node in the graph copy.
     */
    node copy(node v) const
    {
        return m_vCopy[v];
    }

    /**
     * \brief Returns the list of edges coresponding to edge \a e.
     * \param e is an edge in the original graph.
     * \return the corresponding list of edges in the graph copy.
     */
    const List<edge> & chain(edge e) const
    {
        return m_eCopy[e];
    }

    // returns first edge in chain(e)
    /**
     * \brief Returns the first edge in the list of edges coresponding to edge \a e.
     * @param e is an edge in the original graph.
     * \return the first edge in the corresponding list of edges in
     * the graph copy.
     */
    edge copy(edge e) const
    {
        return m_eCopy[e].front();
    }

    /**
     * \brief Returns true iff \a v has no corresponding node in the original graph.
     * @param v is a node in the graph copy.
     */
    bool isDummy(node v) const
    {
        return (m_vOrig[v] == 0);
    }

    /**
     * \brief Returns true iff \a e has no corresponding edge in the original graph.
     * @param e is an edge in the graph copy.
     */
    bool isDummy(edge e) const
    {
        return (m_eOrig[e] == 0);
    }

    /**
     * \brief Returns true iff edge \a e has been reversed.
     * @param e is an edge in the original graph.
     */
    bool isReversed(edge e) const
    {
        return e->source() != original(copy(e)->source());
    }


    /**
     * @name Creation and deletion of nodes and edges
     */
    //@{

    //! Creates a new node in the graph copy.
    node newNode()
    {
        return Graph::newNode();
    }

    /**
     * \brief Creates a new node in the graph copy with original node \a vOrig.
     * \warning You have to make sure that the original node makes sense, in
     *   particular that \a vOrig is not the original node of another node in the copy.
     */
    node newNode(node vOrig)
    {
        OGDF_ASSERT(vOrig != 0 && vOrig->graphOf() == m_pGraph);
        node v = Graph::newNode();
        m_vCopy[m_vOrig[v] = vOrig] = v;
        return v;
    }

    /**
     * \brief Removes node \a v and all its adjacent edges cleaning-up their corresponding lists of original edges.
     *
     * \pre The corresponding lists oforiginal edges contain each only one edge.
     * \param v is a node in the graph copy.
     */
    void delCopy(node v);

    /**
     * \brief Removes edge e and clears the list of edges corresponding to \a e's original edge.
     *
     * \pre The list of edges corresponding to \a e's original edge contains only \a e.
     * \param e is an edge in the graph copy.
     */
    void delCopy(edge e);


    /**
     * \brief Splits edge \a e.
     * @param e is an edge in the graph copy.
     */
    virtual edge split(edge e);


    /**
     * \brief Undoes a previous split operation.
     * The two edges \a eIn and \a eOut are merged to a single edge \a eIn.
     * \pre The vertex \a u that was created by the previous split operation has
     *      exactly one incoming edge \a eIn and one outgoing edge \a eOut.
     * @param eIn is an edge (*,\a u) in the graph copy.
     * @param eOut is an edge (\a u,*) in the graph copy.
     */
    void unsplit(edge eIn, edge eOut);

    //! Creates a new edge (\a v,\a w) with original edge \a eOrig.
    edge newEdge(edge eOrig);

    //! Creates a new edge with original edge \a eOrig at predefined positions in the adjacency lists.
    /**
     * Let \a v be the node whose adjacency list contains \a adjSrc. Then,
     * the created edge is (\a v,\a w).
     * @param eOrig is the original edge.
     * @param adjSrc is the adjacency entry after which the new edge is inserted
     *        in the adjacency list of \a v.
     * @param w is the source node of the new edge; the edge is added at the end
     *        of the adjacency list of \a w.
     * @return the created edge.
     */
    edge newEdge(edge eOrig, adjEntry adjSrc, node w);

    //! Creates a new edge with original edge \a eOrig at predefined positions in the adjacency lists.
    /**
     * Let \a w be the node whose adjacency list contains \a adjTgt. Then,
     * the created edge is (\a v,\a w).
     * @param eOrig is the original edge.
     * @param v is the source node of the new edge; the edge is added at the end
     *        of the adjacency list of \a v.
     * @param adjTgt is the adjacency entry after which the new edge is inserted
     *        in the adjacency list of \a w.
     * @return the created edge.
     */
    edge newEdge(edge eOrig, node v, adjEntry adjTgt);

    edge newEdge(node v, node w)
    {
        return Graph::newEdge(v, w);
    }
    edge newEdge(adjEntry adjSrc, adjEntry adjTgt)
    {
        return Graph::newEdge(adjSrc, adjTgt);
    }
    edge newEdge(node v, adjEntry adjTgt)
    {
        return Graph::newEdge(v, adjTgt);
    }
    edge newEdge(adjEntry adjSrc, node w)
    {
        return Graph::newEdge(adjSrc, w);
    }

    //! sets eOrig to be the corresponding original edge of eCopy and vice versa
    /**
     * @param eOrig is the original edge
     * @param eCopy is the edge copy
     */
    void setEdge(edge eOrig, edge eCopy);

    //! Re-inserts edge \a eOrig by "crossing" the edges in \a crossedEdges.
    /**
     * Let \a v and \a w be the copies of the source and target nodes of \a eOrig.
     * Each edge in \a crossedEdges is split creating a sequence
     * \f$u_1,\ldots,u_k\f$ of new nodes, and additional edges are inserted creating
     * a path \f$v,u_1,\ldots,u_k,w\f$.
     * @param eOrig is an edge in the original graph and becomes the original edge of
     *        all edges in the path \f$v,u_1,\ldots,u_k,w\f$.
     * @param crossedEdges are edges in the graph copy.
     */
    void insertEdgePath(edge eOrig, const SList<adjEntry> & crossedEdges);

    //for FixedEmbeddingUpwardEdgeInserter only
    void insertEdgePath(node srcOrig, node tgtOrig, const SList<adjEntry> & crossedEdges);


    //! Removes the complete edge path for edge \a eOrig.
    /**
     * \@param eOrig is an edge in the original graph.
     */
    void removeEdgePath(edge eOrig);

    //! Inserts crossings between two copy edges.
    /**
     * This method is used in TopologyModule.
     *
     * Let \a crossingEdge = (\a a, \a b) and \a crossedEdge = (\a v, \a w).
     * Then \a crossedEdge is split creating two edges \a crossedEdge = (\a v, \a u)
     * and (\a u, \a w), \a crossingEdge is removed and replaced by two new edges
     * \a e1  = (\a a, \a u) and \a e1 = (\a u, \a b).
     * Finally it sets \a crossingEdge to \a e2 and returns (\a u, \a w).
     *
     * @param crossingEdge is the edge that gets split.
     * @param crossedEdge is the edge that is replaced by two new edges.
     * @param topDown is used as follows: If set to true, \a crossingEdge will cross
     *        \a crossedEdge from right to left, otherwise from left to right.
    */
    edge insertCrossing(
        edge & crossingEdge,
        edge crossedEdge,
        bool topDown);


    /**
     * @name Combinatorial Embeddings
     */
    //@{

    //! Creates a new edge with original edge \a eOrig in an embedding \a E.
    /**
     * Let \a w be the node whose adjacency list contains \a adjTgt. The original
     * edge \a eOrig must connect the original nodes of \a v and \a w. If \a eOrig =
     * (original(\a v),original(\a w)), then the created edge is (\a v,\a w), otherwise
     * it is (\a w,\a v). The new edge \a e must split a face in \a E, such that \a e
     * comes after \a adj in the adjacency list of \a v and at the end of the adjacency
     * list of \a v.
     *
     * @param v is a node in the graph copy.
     * @param adj is an adjacency entry in the graph copy.
     * @param eOrig is an edge in the original graph.
     * @param E is an embedding of the graph copy.
     * @return the created edge.
     */
    edge newEdge(node v, adjEntry adj, edge eOrig, CombinatorialEmbedding & E);

    /**
     * \brief Sets the embedding of the graph copy to the embedding of the original graph.
     * \pre The graph copy has not been changed after construction, i.e., no new nodes
     *      or edges have been added and no edges have been split.
     */
    void setOriginalEmbedding();

    //! Re-inserts edge \a eOrig by "crossing" the edges in \a crossedEdges in embedding \a E.
    /**
     * Let \a v and \a w be the copies of the source and target nodes of \a eOrig,
     * and let \f$e_0,e_1,\ldots,e_k,e_{k+1}\f$ be the sequence of edges corresponding
     * to the adjacency entries in \a crossedEdges. The first edge needs to be incident
     * to \a v and the last to \a w; the edges \f$e_1,\ldots,e_k\f$ are each split
     * creating a sequence \f$u_1,\ldots,u_k\f$ of new nodes, and additional edges
     * are inserted creating a path \f$v,u_1,\ldots,u_k,w\f$.
     *
     * The following figure illustrates, which adjacency entries need to be in the list
     * \a crossedEdges. It inserts an edge connecting \a v and \a w by passing through
     * the faces \f$f_0,f_1,f_2\f$; in this case, the list \a crossedEdges must contain
     * the adjacency entries \f$adj_0,\ldots,adj_3\f$ (in this order).
     * \image html insertEdgePathEmbedded.png
     *
     * @param eOrig is an edge in the original graph and becomes the original edge of
     *        all edges in the path \f$v,u_1,\ldots,u_k,w\f$.
     * @param E is an embedding of the graph copy.
     * @param crossedEdges are a list of adjacency entries in the graph copy.
     */
    void insertEdgePathEmbedded(
        edge eOrig,
        CombinatorialEmbedding & E,
        const SList<adjEntry> & crossedEdges);

    /**
     * Removes the complete edge path for edge \a eOrig while preserving the embedding.
     * @param E is an embedding of the graph copy.
     * @param eOrig is an edge in the original graph.
     * @param newFaces is assigned the set of new faces resulting from joining faces
     *        when removing edges.
     */
    void removeEdgePathEmbedded(
        CombinatorialEmbedding & E,
        edge                    eOrig,
        FaceSetPure      &      newFaces);


    //@}
    /**
     * @name Miscellaneous
     */
    //@{

    //! Checks the consistency of the data structure (for debugging only).
    bool consistencyCheck() const;

    //! Associates the graph copy with \a G, but does not create any nodes or edges.
    /**
     * This method is used for a special creation of the graph copy.
     * The graph copy needs to be constructed with the default constructor,
     * gets associated with \a G using this method, and then is initialized
     * using either initByNodes() or initByActiveNodes().
     *
     * The following code snippet shows a typical application of this functionality:
     * \code
     *   GraphCopy GC;
     *   GC.createEmpty(G);
     *
     *   // compute connected components of G
     *   NodeArray<int> component(G);
     *   int numCC = connectedComponents(G,component);
     *
     *   // intialize the array of lists of nodes contained in a CC
     *   Array<List<node> > nodesInCC(numCC);
     *
     *   node v;
     *   forall_nodes(v,G)
     *     nodesInCC[component[v]].pushBack(v);
     *
     *   EdgeArray<edge> auxCopy(G);
     *   Array<DPoint> boundingBox(numCC);
     *
     *   for(int i = 0; i < numCC; ++i) {
     *     GC.initByNodes(nodesInCC[i],auxCopy);
     *     ...
     *   }
     * \endcode
     * @param G is the graph of which this graph copy shall be a copy.
     */
    void createEmpty(const Graph & G);

    //! Initializes the graph copy for the nodes in a component.
    /**
     * Creates copies of all nodes in \a nodes and their incident edges.
     * Any nodes and edges allocated before are removed.
     *
     * The order of entries in the adjacency lists is preserved, i.e., if
     * the original graph is embedded, its embedding induces the embedding
     * of the created copy.
     *
     * It is important that \a nodes is the complete list of nodes in
     * a connected component. If you wish to initialize the graph copy for an
     * arbitrary set of nodes, use the method initByActiveNodes().
     * \see createEmpty() for an example.
     * @param nodes is the list of nodes in the original graph for which
     *        copies are created in the graph copy.
     * @param eCopy is assigned the copy of each original edge.
     */
    void initByNodes(const List<node> & nodes, EdgeArray<edge> & eCopy);

    //! Initializes the graph copy for the nodes in \a nodes.
    /**
     * Creates copies of all nodes in \a nodes and edges between two nodes
     * which are both contained in \a nodes.
     * Any nodes and edges allocated before are destroyed.
     *
     * \see createEmpty()
     * @param nodes is the list of nodes in the original graph for which
     *        copies are created in the graph copy.
     * @param activeNodes must be true for every node in \a nodes, false
     *        otherwise.
     * @param eCopy is assigned the copy of each original edge.
     */
    void initByActiveNodes(const List<node> & nodes,
                           const NodeArray<bool> & activeNodes, EdgeArray<edge> & eCopy);

    //@}
    /**
     * @name Operators
     */
    //@{

    //! Assignment operator.
    /**
     * Creates a graph copy that is a copy of \a GC and represents a graph
     * copy of the original graph of \a GC.
     *
     * The constructor assures that the adjacency lists of nodes in the
     * constructed graph are in the same order as the adjacency lists in \a G.
     * This is in particular important when dealing with embedded graphs.
     */
    GraphCopy & operator=(const GraphCopy & GC);


    //@}

private:
    void initGC(const GraphCopy & GC,
                NodeArray<node> & vCopy, EdgeArray<edge> & eCopy);

}; // class GraphCopy


} // end namespace ogdf

#endif
