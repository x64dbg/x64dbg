/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Derived class of GraphObserver providing additional functionality
 * to handle clustered graphs.
 *
 * \author Sebastian Leipert, Karsten Klein
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


#ifndef OGDF_CLUSTER_GRAPH_H
#define OGDF_CLUSTER_GRAPH_H

#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/Stack.h>
#include <ogdf/basic/GraphObserver.h>


namespace ogdf
{

class OGDF_EXPORT ClusterGraph;
class OGDF_EXPORT ClusterGraphObserver;

//! Representation of clusters in a clustered graph.
/**
 * \see ClusterGraph
 */
class OGDF_EXPORT ClusterElement : private GraphElement
{

    friend class OGDF_EXPORT ClusterGraph;
    friend class GraphList<ClusterElement>;

    int                     m_id;         //!< The index of this cluster.
    int                     m_depth;      //!< The depth of this cluster in the cluster tree.
    List<node>              m_entries;    //!< The nodes in this cluster.
    List<ClusterElement*>   m_children;   //!< The child clusters of this cluster.
    ClusterElement*          m_parent;    //!< The parent of this cluster.
    ClusterElement*          m_pPrev;     //!< The postorder predecessor of this cluster.
    ClusterElement*          m_pNext;     //!< The postorder successor of this cluster.
    ListIterator<ClusterElement*> m_it;   //!< The position of this cluster within children list of its parent.

    List<adjEntry>          m_adjEntries;   //!< The adjacency list.
    // Don't use a GraphList !
    // This messes with the adjacency
    // list of the underlying graph

#ifdef OGDF_DEBUG
    // we store the graph containing this cluster for debugging purposes
    const ClusterGraph* m_pClusterGraph;
#endif


    void init(List<node> & nodes)
    {
        while(!nodes.empty())
            m_entries.pushBack(nodes.popFrontRet());
    }

    List<ClusterElement*> & getChildren()
    {
        return m_children;
    }

    List<node> & getNodes()
    {
        return m_entries;
    }

    //! Traverses the inclusion tree and adds nodes to \a clusterNodes.
    /**
     * Invoked by public function getClusterNodes(List<node> &clusterNodes).
     */
    void getClusterInducedNodes(List<node> & clusterNodes);
    void getClusterInducedNodes(NodeArray<bool> & clusterNode, int & num);


public:

    //! Creates a new cluster element.
#ifdef OGDF_DEBUG
    ClusterElement(const ClusterGraph* pClusterGraph, int id):
        m_id(id), m_depth(0), m_parent(0), m_pPrev(0), m_pNext(0), m_it(0),
        m_pClusterGraph(pClusterGraph) { }
#else
    ClusterElement(int id):
        m_id(id), m_depth(0), m_parent(0), m_pPrev(0), m_pNext(0), m_it(0) { }
#endif


#ifdef OGDF_DEBUG
    const ClusterGraph* graphOf() const
    {
        return m_pClusterGraph;
    }
#endif


    //! Returns the (unique) index of the cluster.
    int index() const
    {
        return m_id;
    }
    //! Returns the depth of the cluster in the cluster tree.
    int depth() const
    {
        return m_depth;
    }
    int & depth()
    {
        return m_depth;
    }
    //! Returns the successor of the cluster in the list of all clusters.
    ClusterElement* succ() const
    {
        return (ClusterElement*)m_next;
    }
    //! Returns the predecessor of the cluster in the list of all clusters.
    ClusterElement* pred() const
    {
        return (ClusterElement*)m_prev;
    }

    //! Returns the postorder successor of the cluster in the list of all clusters.
    ClusterElement* pSucc() const
    {
        return m_pNext;
    }
    //! Returns the postorder predecessor of the cluster in the list of all clusters.
    ClusterElement* pPred() const
    {
        return m_pPrev;
    }

    // Iteration over tree structures.

    //! Returns the first element in the list of child clusters.
    ListConstIterator<ClusterElement*> cBegin() const
    {
        return m_children.begin();
    }
    //! Returns the last element in the list of child clusters.
    ListConstIterator<ClusterElement*> crBegin() const
    {
        return m_children.rbegin();
    }
    //! Returns the number of child clusters.
    int cCount()
    {
        return m_children.size();
    }
    //! Returns the first element in list of child nodes.
    ListIterator<node> nBegin()
    {
        return m_entries.begin();
    }
    //! Returns the first element in list of child nodes.
    ListConstIterator<node> nBegin() const
    {
        return m_entries.begin();
    }
    //! Returns the number of child nodes.
    int nCount()
    {
        return m_entries.size();
    }

    //! Returns the parent of the cluster.
    ClusterElement* parent()
    {
        return m_parent;
    }


    //! Returns the first adjacency entry in the list of outgoing edges.
    ListConstIterator<adjEntry> firstAdj() const
    {
        return m_adjEntries.begin();
    }
    //! Returns the first adjacency entry in the list of outgoing edges.
    ListIterator<adjEntry> firstAdj()
    {
        return m_adjEntries.begin();
    }
    //! Returns the last adjacency entry in the list of outgoing edges.
    ListConstIterator<adjEntry> lastAdj() const
    {
        return m_adjEntries.rbegin();
    }
    //! Returns the last adjacency entry in the list of outgoing edges.
    ListIterator<adjEntry> lastAdj()
    {
        return m_adjEntries.rbegin();
    }

    //! Returns the list of nodes in the cluster, i.e., all nodes in the subtree rooted at this cluster.
    /**
     * Recursively traverses the cluster tree starting at this cluster.
     */
    void getClusterNodes(List<node> & clusterNodes);
    //! Sets the entry for each node v to true if v is a member of
    //! the subgraph induced by the ClusterElement.
    //! All other entries remain unchanged!
    //! Returns the number of entries set to true.
    //! Precondition: clusterNode is a NodeArray initialized on the clustergraph
    //! the ClusterElement belongs to.
    int getClusterNodes(NodeArray<bool> & clusterNode);

    OGDF_NEW_DELETE

};// class ClusterElement




typedef ClusterElement* cluster; //!< The type of clusters.


#define forall_cluster_adj(adj,c)\
for(ogdf::ListIterator<adjEntry> ogdf_loop_var=(c)->firstAdj();\
    ogdf::test_forall_adj_entries_of_cluster(ogdf_loop_var,(adj));\
    ogdf_loop_var=ogdf_loop_var.succ())

#define forall_cluster_rev_adj(adj,c)\
for(ogdf::ListIterator<adjEntry> ogdf_loop_var=(c)->lastAdj();\
    ogdf::test_forall_adj_entries_of_cluster(ogdf_loop_var,(adj));\
    ogdf_loop_var=ogdf_loop_var.pred())

#define forall_cluster_adj_edges(e,c)\
for(ogdf::ListIterator<adjEntry> ogdf_loop_var=(c)->firstAdj();\
    ogdf::test_forall_adj_edges_of_cluster(ogdf_loop_var,(e));\
    ogdf_loop_var=ogdf_loop_var.succ())



inline bool test_forall_adj_entries_of_cluster(ListIterator<adjEntry> & it, adjEntry & adj)
{
    if(it.valid())
    {
        adj = (*it);
        return true;
    }
    else return false;
}

inline bool test_forall_adj_edges_of_cluster(ListIterator<adjEntry> & it, edge & e)
{
    adjEntry adj = (*it);
    if(adj)
    {
        e = adj->theEdge();
        return true;
    }
    else return false;
}

inline bool test_forall_adj_edges_of_cluster(adjEntry & adj, edge & e)
{
    if(adj)
    {
        e = adj->theEdge();
        return true;
    }
    else return false;
}


class ClusterArrayBase;
template<class T>class ClusterArray;

//---------------------------------------------------------
// iteration macros
//---------------------------------------------------------

//! Iteration over all clusters \a c of cluster graph \a C.
#define forall_clusters(c,C) for((c)=(C).firstCluster(); (c); (c)=(c)->succ())
//! Iteration over all clusters \a c of cluster graph \a C (in postorder).
#define forall_postOrderClusters(c,C)\
for((c)=(C).firstPostOrderCluster(); (c); (c)=(c)->pSucc())




//! Representation of clustered graphs.
/**
 * This class is derived from GraphObserver and handles hierarchical
 * clustering of the nodes in a graph, providing additional functionality.
 */
class OGDF_EXPORT ClusterGraph : public GraphObserver
{
    GraphList<ClusterElement> m_clusters; //!< The list of all clusters.

    const Graph* m_pGraph;            //!< The associated graph.

    int     m_nClusters;              //!< The number of clusters.
    int     m_clusterIdCount;         //!< The index assigned to the next created cluster.
    int     m_clusterArrayTableSize;  //!< The current table size of cluster arrays.

    mutable cluster m_postOrderStart; //!< The first cluster in postorder.
    cluster m_rootCluster;            //!< The root cluster.

    bool    m_adjAvailable;       //! True if the adjacency list for each cluster is available.
    bool    m_allowEmptyClusters; //! Defines if empty clusters are deleted immediately if generated by operations.

    NodeArray<cluster> m_nodeMap; //!< Stores the cluster of each node.
    //! Stories for every node its position within the children list of its cluster.
    NodeArray<ListIterator<node> >  m_itMap;

    mutable ListPure<ClusterArrayBase*> m_regClusterArrays; //!< The registered cluster arrays.
    mutable ListPure<ClusterGraphObserver*> m_regObservers; //!< The registered graph observers.

public:

    //! Creates a cluster graph associated with no graph.
    ClusterGraph();

    //! Creates a cluster graph associated with graph \a G.
    /**
     * All nodes in \a G are assigned to the root cluster.
     */
    ClusterGraph(const Graph & G);

    //! Copy constructor.
    ClusterGraph(const ClusterGraph & C);

    //! Constructs a clustered graph that is a copy of clustered graph C.
    /**
     * The underlying graph \a G is made a copy of C.getGraph().
     */
    ClusterGraph(const ClusterGraph & C, Graph & G);

    //! Constructs a clustered graph that is a copy of clustered graph C.
    /**
     * The underlying graph \a G is made a copy of C.getGraph(). Stores the
     * new copies of the original nodes and clusters in the arrays
     * \a originalNodeTable and \a originalClusterTable.
     */
    ClusterGraph(
        const ClusterGraph & C,
        Graph & G,
        ClusterArray<cluster> & originalClusterTable,
        NodeArray<node> & originalNodeTable);

    //! Constructs a clustered graph that is a copy of clustered graph C.
    /**
     * The underlying graph \a G is made a copy of C.getGraph(). Stores the
     * new copies of the original nodes, edges, and clusters in the arrays
     * \a originalNodeTable, \a edgeCopy, and \a originalClusterTable.
     */
    ClusterGraph(
        const ClusterGraph & C,
        Graph & G,
        ClusterArray<cluster> & originalClusterTable,
        NodeArray<node> & originalNodeTable,
        EdgeArray<edge> & edgeCopy);

    virtual ~ClusterGraph();

    //! Returns the maximal used cluster index.
    int maxClusterIndex() const
    {
        return m_clusterIdCount - 1;
    }

    //! Clears all cluster data.
    void clear();

    //! Clears all data but does not delete root cluster.
    void semiClear();

    //! Clears all cluster data and then reinitializes the instance with underlying graph \a G.
    void init(const Graph & G);

    //! Conversion to const Graph reference.
    operator const Graph & () const
    {
        return *m_pGraph;
    }

    //! Assignment operator.
    ClusterGraph & operator=(const ClusterGraph & C);

    //! Removes all clusters from the cluster subtree rooted at cluster C except for cluster C itself.
    void clearClusterTree(cluster C);

    //! Returns a reference to the underlying graph.
    //TODO should be named getConstGraph
    const Graph & getGraph() const
    {
        return *m_pGraph;
    }

    //! Inserts a new cluster; makes it a child of the cluster \a parent.
    cluster newCluster(cluster parent, int id = -1);

    //! Creates an empty cluster with index \a clusterId and parent \a parent.
    cluster createEmptyCluster(const cluster parent = 0, int clusterId = -1);

    //! Creates a new cluster containing the nodes given by \a nodes; makes it a child of the cluster \a parent.
    /**
     * The nodes are reassigned to the new cluster. If you turn off
     * \a m_allowEmptyclusters, an emptied cluster is deleted except if all
     * nodes are put into the same cluster.
     * @param nodes are the nodes that will be reassigned to the new cluster.
     * @param parent is the parent of the new cluster.
     * \return the created cluster.
     */
    cluster createCluster(SList<node> & nodes, const cluster parent = 0);

    //! Deletes cluster \a c.
    /**
     * All subclusters become children of parent cluster of \a c.
     * \pre \a c is not the root cluster.
     */
    void delCluster(cluster c);

    //! Returns the root cluster.
    cluster rootCluster() const
    {
        return m_rootCluster;
    }

    //! Returns the cluster to which a node belongs.
    inline cluster clusterOf(node v) const
    {
        OGDF_ASSERT(v->graphOf() == m_pGraph)
        return m_nodeMap[v];
    }

    //! Returns number of clusters.
    int numberOfClusters() const
    {
        return m_nClusters;
    }
    //! Returns upper bound for cluster indices.
    int clusterIdCount() const
    {
        return m_clusterIdCount;
    }

    //! Returns table size of cluster arrays associated with this graph.
    int clusterArrayTableSize() const
    {
        return m_clusterArrayTableSize;
    }

    //! Moves cluster \a c to a new parent \a newParent.
    void moveCluster(cluster c, cluster newParent);


    //! Reassigns node \a v to cluster \ c.
    void reassignNode(node v, cluster c);

    //! Clear cluster info structure, reinitializes with underlying graph \a G.
    //inserted mainly for use in gmlparser.
    void reInit(Graph & G)
    {
        reinitGraph(G);
    }

    //---------------------------
    //tree queries / depth issues

    //! Turns automatic update of node depth values on or off.
    void setUpdateDepth(bool b) const
    {
        m_updateDepth = b;
        //make sure that depth cant be used anymore
        //(even if it may still be valid a little while)
        if(!b) m_depthUpToDate = false;
    }

    //! Updates depth information in subtree after delCluster.
    void pullUpSubTree(cluster c);

    //! Computes depth of cluster tree, running time O(C).
    //maybe later we should provide a permanent depth member update
    int treeDepth() const
    {
        //initialize depth at first call
        if(m_updateDepth && !m_depthUpToDate)
            computeSubTreeDepth(rootCluster());
        if(!m_updateDepth) OGDF_THROW(AlgorithmFailureException);
        int l_depth = 1;
        cluster c;
        forall_clusters(c, *this)
        {
            if(c->depth() > l_depth) l_depth = c->depth();
        }

        return l_depth;
    }
    //! Computes depth of cluster tree hanging at \a c.
    void computeSubTreeDepth(cluster c) const;
    //! Returns depth of cluster c in cluster tree, starting with root depth 1.
    //should be called instead of direct c->depth call to assure
    //valid depth
    int & clusterDepth(cluster c) const
    {
        // updateDepth must be set to true if depth info shall be used
        OGDF_ASSERT(m_updateDepth);

        //initialize depth at first call
        if(!m_depthUpToDate)
            computeSubTreeDepth(rootCluster());
        OGDF_ASSERT(c->depth() != 0)
        return c->depth();
    }

    //! Returns lowest common cluster of nodes in list \a nodes.
    cluster commonCluster(SList<node> & nodes);

    //! Returns the lowest common cluster of \a v and \a w in the cluster tree
    /**
     * \pre \a v  and \a w are nodes in the graph.
     */
    cluster commonCluster(node v, node w) const;

    //! Returns the lowest common cluster lca and the highest ancestors on the path to lca.
    cluster commonClusterLastAncestors(
        node v,
        node w,
        cluster & c1,
        cluster & c2) const;
    //! Returns lca of \a v and \a w and stores corresponding path in \a eL.

    cluster commonClusterPath(
        node v,
        node w,
        List<cluster> & eL) const;

    //! Returns lca of \a v and \a w, stores corresponding path in \a eL and ancestors in \a c1, \a c2.
    cluster commonClusterAncestorsPath(
        node v,
        node w,
        cluster & c1,
        cluster & c2,
        List<cluster> & eL) const;

    //! Registers a cluster array.
    ListIterator<ClusterArrayBase*> registerArray(ClusterArrayBase* pClusterArray) const;

    //! Unregisters a cluster array.
    void unregisterArray(ListIterator<ClusterArrayBase*> it) const;

    //! Registers a ClusterGraphObserver.
    ListIterator<ClusterGraphObserver*> registerObserver(ClusterGraphObserver* pObserver) const;

    //! Unregisters a ClusterGraphObserver.
    void unregisterObserver(ListIterator<ClusterGraphObserver*> it) const;

    //! Returns the list of clusters that are empty or only contain empty clusters.
    /**
     * The list is constructed in an order that allows deletion and reinsertion.
     * We never set rootcluster to be one of the empty clusters!!
     * if checkClusters is given, only list elements are checked
     * to allow efficient checking in the case
     * that you know which clusters were recently changed (e.g. node reass.)
     */
    void emptyClusters(SList<cluster> & emptyCluster, SList<cluster>* checkCluster = 0);

    //! Returns true if cluster \a c has only one node and no children.
    inline bool emptyOnNodeDelete(cluster c) //virtual?
    {
        //if (!c) return false; //Allows easy use in loops
        return (c->nCount() == 1) && (c->cCount() == 0);
    }

    //! Returns true if cluster \a c has only one child and no nodes.
    inline bool emptyOnClusterDelete(cluster c) //virtual?
    {
        //if (!c) return false; //Allows easy use in loops
        return (c->nCount() == 0) && (c->cCount() == 1);
    }

    //! Returns the first cluster in the list of all clusters.
    cluster firstCluster() const
    {
        return m_clusters.begin();
    }
    //! Returns the last cluster in the list of all cluster.
    cluster lastCluster() const
    {
        return m_clusters.rbegin();
    }
    //! Returns the first cluster in the list of post ordered clusters.
    cluster firstPostOrderCluster() const
    {
        if(!m_postOrderStart) postOrder();
        return m_postOrderStart;
    }

    //! Returns the list of all clusters in \a clusters.
    template<class CLUSTERLIST>
    void allClusters(CLUSTERLIST & clusters) const
    {
        clusters.clear();
        for(cluster c = m_clusters.begin(); c; c = c->succ())
            clusters.pushBack(c);
    }

    //! Collapses all nodes in the list \a nodes to the first node; multi-edges are removed.
    template<class NODELIST>
    void collaps(NODELIST & nodes, Graph & G)
    {
        OGDF_ASSERT(&G == m_pGraph);
        m_adjAvailable = false;

        m_postOrderStart = 0;
        node v = nodes.popFrontRet();
        while(!nodes.empty())
        {
            node w = nodes.popFrontRet();
            adjEntry adj = w->firstAdj();
            while(adj != 0)
            {
                adjEntry succ = adj->succ();
                edge e = adj->theEdge();
                if(e->source() == v || e->target() == v)
                    G.delEdge(e);
                else if(e->source() == w)
                    G.moveSource(e, v);
                else
                    G.moveTarget(e, v);
                adj = succ;
            }
            //because nodes can already be unassigned (they are always
            //unassigned if deleted), we have to check this
            /*
            if (m_nodeMap[w])
            {
                cluster c = m_nodeMap[w];
                c->m_entries.del(m_itMap[w]);
            }
            */
            //removeNodeAssignment(w);
            G.delNode(w);
        }
    }

    //! Returns the list of all edges adjacent to cluster \a c in \a edges.
    template<class EDGELIST>
    void adjEdges(cluster c, EDGELIST & edges) const
    {
        edges.clear();
        edge e;
        if(m_adjAvailable)
        {
            forall_cluster_adj_edges(e, c)
            edges.pushBack(e);
        }
    }

    //! Returns the list of all adjacency entries adjacent to cluster \a c in \a entries.
    template<class ADJLIST>
    void adjEntries(cluster c, ADJLIST & entries) const
    {
        entries.clear();
        adjEntry adj;
        if(m_adjAvailable)
        {
            forall_cluster_adj(adj, c)
            entries.pushBack(adj);
        }
    }

    //! Computes the adjacency entry list for cluster \a c.
    template<class LISTITERATOR>
    void makeAdjEntries(cluster c, LISTITERATOR start)
    {
        adjEntry adj;
        c->m_adjEntries.clear();
        LISTITERATOR its;
        for(its = start; its.valid(); its++)
        {
            adj = (*its);
            c->m_adjEntries.pushBack(adj);
        }
    }

    //**************************
    //file output

    //! Writes the cluster graph in GML format to file \a fileName.
    void writeGML(const char* fileName);

    //! Writes the cluster graph in GML format to output stream \a os.
    void writeGML(ostream & os);


    //**************************
    //file input
    //! reading graph, attributes, cluster structure from file
    bool readClusterGML(const char* fileName, Graph & G);
    //! reading graph, attributes, cluster structure from stream
    bool readClusterGML(istream & is, Graph & G);

    // read Cluster Graph from OGML file
    //bool readClusterGraphOGML(const char* fileName, ClusterGraph& CG, Graph& G);

    //! Checks the consistency of the data structure.
    // (for debugging purposes only)
    bool consistencyCheck();

    //! Checks the combinatorial cluster planar embedding.
    // (for debugging purposes only)
    bool representsCombEmbedding();

    //! Sets the availability status of the adjacency entries.
    void adjAvailable(bool val)
    {
        m_adjAvailable = val;
    }

protected:
    //! Creates new cluster containing nodes in parameter list
    //! with index \a clusterid.
    cluster doCreateCluster(SList<node> & nodes,
                            const cluster parent, int clusterId = -1);
    //! Creates new cluster containing nodes in parameter list and
    //! stores resulting empty clusters in list, cluster has index \a clusterid.
    cluster doCreateCluster(SList<node> & nodes,
                            SList<cluster> & emptyCluster,
                            const cluster parent, int clusterId = -1);

    mutable ClusterArray<int>* m_lcaSearch; //!< Used to save last search run number for commoncluster.
    mutable int m_lcaNumber;//!< Used to save last search run number for commoncluster.
    mutable ClusterArray<cluster>* m_vAncestor;//!< Used to save last search run number for commoncluster.
    mutable ClusterArray<cluster>* m_wAncestor;//!< Used to save last search run number for commoncluster.

    //! Copies lowest common ancestor info to copy of clustered graph.
    void copyLCA(const ClusterGraph & C, ClusterArray<cluster>* clusterCopy = 0);
    //int m_treeDepth; //should be implemented and updated in operations?

    mutable bool m_updateDepth; //!< Depth of clusters is always updated if set to true.
    mutable bool m_depthUpToDate; //!< Status of cluster depth information.

    //! Adjusts the post order structure for moved clusters.
    //we assume that c is inserted via pushback in newparent->children
    void updatePostOrder(cluster c, cluster oldParent, cluster newParent);

    //! Computes new predecessor for SUBTREE at moved cluster c.
    //0 if c==root
    cluster postOrderPredecessor(cluster c) const;
    //! Leftmost cluster in subtree rooted at c, gets predecessor of subtree.
    cluster leftMostCluster(cluster c) const;

    //---------------------------------------
    //functions inherited from GraphObserver:
    //define how to cope with graph changes

    //! Implementation of inherited method: Updates data if node deleted.
    virtual void nodeDeleted(node v)
    {
        bool cRemove = false;
        cluster c = clusterOf(v);
        if(!c) return;
        //never allow totally empty cluster
        //if ((emptyOnNodeDelete(c)) &&
        //  (c != rootCluster()) ) cRemove = true;
        unassignNode(v);
        if(cRemove && !m_allowEmptyClusters)  //parent exists
        {
            cluster nonEmpty = c->parent();
            cluster cRun = nonEmpty;
            delCluster(c);
            while((cRun != rootCluster()) && (cRun->nCount() + cRun->cCount() == 0))
            {
                nonEmpty = cRun->parent();
                delCluster(cRun);
                cRun = nonEmpty;
            }

        }
    }
    //! Implementation of inherited method: Updates data if node added.
    virtual void nodeAdded(node v)
    {
        assignNode(v, rootCluster());
    }
    //! Implementation of inherited method: Updates data if edge deleted.
    virtual void edgeDeleted(edge /* e */) { }
    //! Implementation of inherited method: Updates data if edge added.
    virtual void edgeAdded(edge /* e */)   { }
    //! Currently does nothing.
    virtual void reInit()            { }
    //! Clears cluster data without deleting root when underlying graphs' clear method is called.
    virtual void cleared()
    {
        //we don't want a complete clear, as the graph still exists
        //and can be updated from input stream
        semiClear();
    }//Graph cleared

private:
    //! Assigns node \a v to cluster \a c (\a v not yet assigned!).
    void assignNode(node v, cluster C);

    //! Unassigns node \a v from its cluster.
    void unassignNode(node v);

    //! Remove the assignment entries for nodes.
    //! Checks if node is currently not assigned.
    void removeNodeAssignment(node v)
    {
        if(m_nodeMap[v])  //iff == 0, itmap == 0 !!?
        {
            cluster C2 = m_nodeMap[v];
            C2->m_entries.del(m_itMap[v]);
            m_nodeMap[v] = 0;
            m_itMap[v] = 0;
        }
    }

    //! Performs a copy of the cluster structure of C,
    //! the underlying graph stays the same.
    void shallowCopy(const ClusterGraph & C);

    //! Perform a deep copy on C, C's underlying
    //! graph is copied into G.
    void deepCopy(const ClusterGraph & C, Graph & G);

    //! Perform a deep copy on C, C's underlying
    //! graph is copied into G. Stores associated nodes in \a originalNodeTable.

    void deepCopy(
        const ClusterGraph & C, Graph & G,
        ClusterArray<cluster> & originalClusterTable,
        NodeArray<node> & originalNodeTable);

    //! Perform a deep copy on C, C's underlying
    //! graph is copied into G.  Stores associated nodes in \a originalNodeTable
    //! and edges in \a edgeCopy.
    void deepCopy(
        const ClusterGraph & C, Graph & G,
        ClusterArray<cluster> & originalClusterTable,
        NodeArray<node> & originalNodeTable,
        EdgeArray<edge> & edgeCopy);


    void clearClusterTree(cluster c, List<node> & attached);

    void initGraph(const Graph & G);

    //! Reinitializes instance with graph \a G.
    void reinitGraph(const Graph & G);

    //! Creates new cluster with given id, precondition: id not used
    cluster newCluster(int id);
    //! Creates new cluster.
    cluster newCluster();

    //! Create postorder information in cluster tree.
    void postOrder() const;
    //! Check postorder information in cluster tree.
    void checkPostOrder() const;

    void postOrder(cluster c, SListPure<cluster> & S) const;

    void reinitArrays();


    //! Recursively write the cluster structure in GML.
    void writeCluster(
        ostream & os,
        NodeArray<int> & nId,
        ClusterArray<int> & cId,
        int & nextId,
        cluster c,
        String ttt);

    //! Recursively write the cluster structure in GraphWin GML.
    void writeGraphWinCluster(
        ostream & os,
        NodeArray<int> & nId,
        NodeArray<String> & nStr,
        ClusterArray<int> & cId,
        ClusterArray<String> & cStr,
        int & nextId,
        cluster c,
        String ttt);

};





ostream & operator<<(ostream & os, ogdf::cluster c);


} // end namespace ogdf

#endif
