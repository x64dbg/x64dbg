/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of CombinatorialEmbedding and face.
 *
 * Enriches graph by the notion of faces
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

#ifndef OGDF_COMBINATORIAL_EMBEDDING_H
#define OGDF_COMBINATORIAL_EMBEDDING_H


#include <ogdf/basic/AdjEntryArray.h>


namespace ogdf
{

class OGDF_EXPORT ConstCombinatorialEmbedding;

typedef FaceElement* face;

/**
 * \brief Faces in a combinatorial embedding.
 */
class OGDF_EXPORT FaceElement : private GraphElement
{
    friend class ConstCombinatorialEmbedding;
    friend class CombinatorialEmbedding;
    friend class GraphList<FaceElement>;

    adjEntry m_adjFirst; //!< The first adjacency element in the face.
    int m_id;   //!< The index of the face.
    int m_size; //!< The size of the face.

#ifdef OGDF_DEBUG
    const ConstCombinatorialEmbedding* m_pEmbedding;
#endif

    // constructor
#ifdef OGDF_DEBUG
    FaceElement(const ConstCombinatorialEmbedding* pEmbedding,
                adjEntry adjFirst,
                int id) :
        m_adjFirst(adjFirst), m_id(id), m_size(0), m_pEmbedding(pEmbedding) { }
#else
    //! Creates a face with given first adjacency element \a adjFirst and face index \a id.
    FaceElement(adjEntry adjFirst, int id) :
        m_adjFirst(adjFirst), m_id(id), m_size(0) { }
#endif

public:
    //! Returns the index of the face.
    int index() const
    {
        return m_id;
    }

    //! Returns the first adjacency element in the face.
    adjEntry firstAdj() const
    {
        return m_adjFirst;
    }

    //! Returns the size of the face, i.e., the number of edges in the face.
    int size() const
    {
        return m_size;
    }

    //! Returns the successor in the list of all faces.
    face succ() const
    {
        return (face)m_next;
    }

    //! Returns the predecessor in the list of all faces.
    face pred() const
    {
        return (face)m_prev;
    }

    //! Returns the successor of \a adj in the list of all adjacency elements in the face.
    adjEntry nextFaceEdge(adjEntry adj) const
    {
        adj = adj->faceCycleSucc();
        return (adj != m_adjFirst) ? adj : 0;
    }

#ifdef OGDF_DEBUG
    const ConstCombinatorialEmbedding* embeddingOf() const
    {
        return m_pEmbedding;
    }
#endif

    OGDF_NEW_DELETE
}; // class FaceElement


class FaceArrayBase;
template<class T>class FaceArray;


/**
 * \brief Combinatorial embeddings of planar graphs.
 *
 * Maintains a combinatorial embedding of an embedded graph, i.e., the set of
 * faces. A combinatorial embedding is defined by the (cyclic) order of the
 * adjacency entries around a vertex; more precisely, the adjacency list
 * gives the cyclic order of the adjacency entries in clockwise order.
 * Each adjacency entry \a adj is contained in exactly one face, the face
 * to the right of \a adj. The list of adjacency entries defining a face is given
 * in clockwise order for internal faces, and in counter-clockwise order for the
 * external face.
 *
 * \see CombinatorialEmbedding provides additional functionality for modifying
 *      the embedding.
 */
class OGDF_EXPORT ConstCombinatorialEmbedding
{
protected:
    const Graph* m_cpGraph; //!< The associated graph.

    GraphList<FaceElement> m_faces; //!< The list of all faces.
    int m_nFaces; //!< The number of faces.
    int m_faceIdCount; //!< The index assigned to the next created face.
    int m_faceArrayTableSize; //!< The current table size of face arrays.

    AdjEntryArray<face> m_rightFace; //!< The face to which an adjacency entry belongs.
    face m_externalFace; //! The external face.

    mutable ListPure<FaceArrayBase*> m_regFaceArrays; //!< The registered face arrays.

public:
    /** @{
     * \brief Creates a combinatorial embedding associated with no graph.
     */
    ConstCombinatorialEmbedding();

    /**
     * \brief Creates a combinatorial embedding of graph \a G.
     *
     * \pre Graph \a G must be embedded, i.e., the adjacency lists of its nodes
     *      must define an embedding.
     */
    explicit ConstCombinatorialEmbedding(const Graph & G);


    //! Copy constructor.
    ConstCombinatorialEmbedding(const ConstCombinatorialEmbedding & C);

    //! Assignment operator.
    ConstCombinatorialEmbedding & operator=(const ConstCombinatorialEmbedding & C);

    /** @} @{
     * \brief Returns the associated graph of the combinatorial embedding.
     */
    const Graph & getGraph() const
    {
        return *m_cpGraph;
    }

    //! Returns associated graph
    operator const Graph & () const
    {
        return *m_cpGraph;
    }

    /** @} @{
     * \brief Returns the first face in the list of all faces.
     */
    face firstFace() const
    {
        return m_faces.begin();
    }

    //! Returns the last face in the list of all faces.
    face lastFace() const
    {
        return m_faces.rbegin();
    }

    //! Returns the number of faces.
    int numberOfFaces() const
    {
        return m_nFaces;
    }

    /** @} @{
     * \brief Returns the face to the right of \a adj, i.e., the face containing \a adj.
     * @param adj is an adjecency element in the associated graph.
     */
    face rightFace(adjEntry adj) const
    {
        return m_rightFace[adj];
    }

    /**
     * \brief Returns the face to the left of \a adj, i.e., the face containing the twin of \a adj.
     * @param adj is an adjacency element in the associated graph.
     */
    face leftFace(adjEntry adj) const
    {
        return m_rightFace[adj->twin()];
    }

    /** @} @{
     * \brief Returns the largest used face index.
     */
    int maxFaceIndex() const
    {
        return m_faceIdCount - 1;
    }

    //! Returns the table size of face arrays associated with this embedding.
    int faceArrayTableSize() const
    {
        return m_faceArrayTableSize;
    }

    /** @} @{
     * \brief Returns a random face.
     */
    face chooseFace() const;

    //! Returns a face of maximal size.
    face maximalFace() const;

    /** @} @{
     * \brief Returns the external face.
     */
    face externalFace() const
    {
        return m_externalFace;
    }

    /**
     * \brief Sets the external face to \a f.
     * @param f is a face in this embedding.
     */
    void setExternalFace(face f)
    {
        OGDF_ASSERT(f->embeddingOf() == this);
        m_externalFace = f;
    }

    bool isBridge(edge e) const
    {
        return m_rightFace[e->adjSource()] == m_rightFace[e->adjTarget()];
    }

    /** @} @{
     * \brief Initializes the embedding for graph \a G.
     *
     * \pre Graph \a G must be embedded, i.e., the adjacency lists of its nodes
     *      must define an embedding.
     */
    void init(const Graph & G);

    void init();

    //! Computes the list of faces.
    void computeFaces();


    /** @} @{
     * \brief Checks the consistency of the data structure.
     */
    bool consistencyCheck();


    /** @} @{
     * \brief Registers the face array \a pFaceArray.
     *
     * This method is only used by face arrays.
     */
    ListIterator<FaceArrayBase*> registerArray(FaceArrayBase* pFaceArray) const;

    /**
     * \brief Unregisters the face array identified by \a it.
     *
     * This method is only used by face arrays.
     */
    void unregisterArray(ListIterator<FaceArrayBase*> it) const;

    /** @} */

protected:
    //! Create a new face.
    face createFaceElement(adjEntry adjFirst);

    //! Reinitialize associated face arrays.
    void reinitArrays();

}; // class ConstCombinatorialEmbedding



/**
 * \brief Combinatorial embeddings of planar graphs with modification functionality.
 *
 * Maintains a combinatorial embedding of an embedded graph, i.e., the set of
 * faces, and provides method for modifying the embedding, e.g., by inserting edges.
 */
class OGDF_EXPORT CombinatorialEmbedding : public ConstCombinatorialEmbedding
{
    Graph* m_pGraph; //!< The associated graph.

    // the following methods are private in order to make them unusable
    // It is not clear which meaning copying of a comb. embedding should
    // have since we only store a pointer to the topology (Graph)
    CombinatorialEmbedding(const CombinatorialEmbedding &) : ConstCombinatorialEmbedding() { }
    CombinatorialEmbedding & operator=(const CombinatorialEmbedding &)
    {
        return *this;
    }

public:
    /** @{
     * \brief Creates a combinatorial embedding associated with no graph.
     */
    CombinatorialEmbedding() : ConstCombinatorialEmbedding()
    {
        m_pGraph = 0;
    }

    /**
     * \brief Creates a combinatorial embedding of graph \a G.
     *
     * \pre Graph \a G must be embedded, i.e., the adjacency lists of its nodes
     *      must define an embedding.
     */
    explicit CombinatorialEmbedding(Graph & G) : ConstCombinatorialEmbedding(G)
    {
        m_pGraph = &G;
    }

    //@}
    /**
     * @name Access to the associated graph
     */
    //@{

    /**
     * \brief Returns the associated graph.
     */
    const Graph & getGraph() const
    {
        return *m_cpGraph;
    }

    Graph & getGraph()
    {
        return *m_pGraph;
    }

    operator const Graph & () const
    {
        return *m_cpGraph;
    }

    operator Graph & ()
    {
        return *m_pGraph;
    }


    //@}
    /**
     * @name Initialization
     */
    //@{

    /**
     * \brief Initializes the embedding for graph \a G.
     *
     * \pre Graph \a G must be embedded, i.e., the adjacency lists of its nodes
     *      must define an embedding.
     */
    void init(Graph & G)
    {
        ConstCombinatorialEmbedding::init(G);
        m_pGraph = &G;
    }

    /**
     * \brief Removes all nodes, edges, and faces from the graph and the embedding.
     */
    void clear();


    //@}
    /**
     * @name Update of embedding
     */
    //@{

    /**
     * \brief Splits edge \a e=(\a v,\a w) into \a e=(\a v,\a u) and \a e'=(\a u,\a w) creating a new node \a u.
     * @param e is the edge to be split; \a e is modified by the split.
     * \return the edge \a e'.
     */
    edge split(edge e);

    /**
     * \brief Undoes a split operation.
     * @param eIn is the edge (\a v,\a u).
     * @param eOut is the edge (\a u,\a w).
     */
    void unsplit(edge eIn, edge eOut);

    /**
     * \brief Splits a node while preserving the order of adjacency entries.
     *
     * This method splits a node \a v into two nodes \a vl and \a vr. Node
     * \a vl receives all adjacent edges of \a v from \a adjStartLeft until
     * the edge preceding \a adjStartRight, and \a vr the remaining nodes
     * (thus \a adjStartRight is the first edge that goes to \a vr). The
     * order of adjacency entries is preserved. Additionally, a new edge
     * (\a vl,\a vr) is created, such that this edge is inserted before
     * \a adjStartLeft and \a adjStartRight in the the adjacency lists of
     * \a vl and \a vr.
     *
     * Node \a v is modified to become node \a vl, and node \a vr is returned.
     *
     * @param adjStartLeft is the first entry that goes to the left node.
     * @param adjStartRight is the first entry that goes to the right node.
     * \return the newly created node.
     */
    node splitNode(adjEntry adjStartLeft, adjEntry adjStartRight);

    /**
     * \brief Contracts edge \a e.
     * @param e is an edge is the associated graph.
     * @return the node resulting from the contraction.
     */
    node contract(edge e);

    /**
     * \brief Splits a face by inserting a new edge.
     *
     * This operation introduces a new edge \a e from the node to which \a adjSrc
     * belongs to the node to which \a adjTgt belongs.
     * \pre \a adjSrc and \a adjTgt belong to the same face.
     * \return the new edge \a e.
     */
    edge splitFace(adjEntry adjSrc, adjEntry adjTgt);

    // incremental stuff

    //special version of the above function doing a pushback of the new edge
    //on the adjacency list of v making it possible to insert new degree 0
    //nodes into a face
    edge splitFace(node v, adjEntry adjTgt);
    edge splitFace(adjEntry adjSrc, node v);

    /**
     * \brief Removes edge e and joins the two faces adjacent to \a e.
     * @param e is an edge in the associated graph.
     * \return the resulting (joined) face.
     */
    face joinFaces(edge e);

    //! Reverses edges \a e and updates embedding.
    void reverseEdge(edge e);

    void moveBridge(adjEntry adjBridge, adjEntry adjBefore);

    void removeDeg1(node v);

    //! Update face information after inserting a merger in a copy graph.
    void updateMerger(edge e, face fRight, face fLeft);


    /** @} */

}; // class CombinatorialEmbedding


//---------------------------------------------------------
// iteration macros
//---------------------------------------------------------

//! Iteration over all faces \a f of the combinatorial embedding \a E.
#define forall_faces(f,E) for((f)=(E).firstFace(); (f); (f)=(f)->succ())


//! Iteration over all faces \a f of the combinatorial embedding \a E (in reverse order).
#define forall_rev_faces(f,E) for((f)=(E).lastFace(); (f); (f)=(f)->pred())

/**
 * \brief Iteration over all adjacency entries \a adj of the face \a f.
 *
 * A faster version for this iteration demonstrates the following code snippet:
 * \code
 *   adjEntry adj1 = f->firstAdj(), adj = adj1;
 *   do {
 *     ...
 *     adj = adj->faceCycleSucc();
 *   } while (adj != adj1);
 * \endcode
 */
#define forall_face_adj(adj,f) for((adj)=(f)->firstAdj(); (adj); (adj)=(f)->nextFaceEdge(adj))


} // end namespace ogdf


#endif
