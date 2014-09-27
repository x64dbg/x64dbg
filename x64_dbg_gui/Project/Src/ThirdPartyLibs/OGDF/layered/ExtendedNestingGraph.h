/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of ExtendedNestingGraph
 *
 * Manages access on copy of an attributed graph.
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

#ifndef OGDF_EXTENDED_NESTING_GRAPH_H
#define OGDF_EXTENDED_NESTING_GRAPH_H



#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/basic/EdgeArray.h>
#include <ogdf/cluster/ClusterArray.h>
#include <limits.h>


namespace ogdf
{


//---------------------------------------------------------
// RCCrossings
//---------------------------------------------------------
struct OGDF_EXPORT RCCrossings
{
    RCCrossings()
    {
        m_cnClusters = 0;
        m_cnEdges    = 0;
    }

    RCCrossings(int cnClusters, int cnEdges)
    {
        m_cnClusters = cnClusters;
        m_cnEdges    = cnEdges;
    }

    void incEdges(int cn)
    {
        m_cnEdges += cn;
    }

    void incClusters()
    {
        ++m_cnClusters;
    }

    RCCrossings & operator+=(const RCCrossings & cr)
    {
        m_cnClusters += cr.m_cnClusters;
        m_cnEdges    += cr.m_cnEdges;
        return *this;
    }

    RCCrossings operator+(const RCCrossings & cr) const
    {
        return RCCrossings(m_cnClusters + cr.m_cnClusters, m_cnEdges + cr.m_cnEdges);
    }

    RCCrossings operator-(const RCCrossings & cr) const
    {
        return RCCrossings(m_cnClusters - cr.m_cnClusters, m_cnEdges - cr.m_cnEdges);
    }

    bool operator<=(const RCCrossings & cr) const
    {
        if(m_cnClusters == cr.m_cnClusters)
            return (m_cnEdges <= cr.m_cnEdges);
        else
            return (m_cnClusters <= cr.m_cnClusters);
    }

    bool operator<(const RCCrossings & cr) const
    {
        if(m_cnClusters == cr.m_cnClusters)
            return (m_cnEdges < cr.m_cnEdges);
        else
            return (m_cnClusters < cr.m_cnClusters);
    }

    bool isZero() const
    {
        return m_cnClusters == 0 && m_cnEdges == 0;
    }

    RCCrossings & setInfinity()
    {
        m_cnClusters = m_cnEdges = INT_MAX;
        return *this;
    }

    static int compare(const RCCrossings & x, const RCCrossings & y)
    {
        int dc = y.m_cnClusters - x.m_cnClusters;
        if(dc != 0)
            return dc;
        return y.m_cnEdges - x.m_cnEdges;
    }

    int m_cnClusters;
    int m_cnEdges;
};

OGDF_EXPORT ostream & operator<<(ostream & os, const RCCrossings & cr);


//---------------------------------------------------------
// LHTreeNode
//---------------------------------------------------------
class OGDF_EXPORT LHTreeNode
{
public:
    enum Type { Compound, Node, AuxNode };

    struct Adjacency
    {
        Adjacency()
        {
            m_u = 0;
            m_v = 0;
            m_weight = 0;
        }
        Adjacency(node u, LHTreeNode* vNode, int weight = 1)
        {
            m_u      = u;
            m_v      = vNode;
            m_weight = weight;
        }

        node          m_u;
        LHTreeNode*   m_v;
        int           m_weight;

        OGDF_NEW_DELETE
    };

    struct ClusterCrossing
    {
        ClusterCrossing()
        {
            m_uc = 0;
            m_u = 0;
            m_cNode = 0;
            m_uNode = 0;
        }
        ClusterCrossing(node uc, LHTreeNode* cNode, node u, LHTreeNode* uNode, edge e)
        {
            m_uc = uc;
            m_u  = u;
            m_cNode = cNode;
            m_uNode = uNode;

            m_edge = e;
        }

        node m_uc;
        node m_u;
        LHTreeNode* m_cNode;
        LHTreeNode* m_uNode;

        edge m_edge;
    };

    // Construction
    LHTreeNode(cluster c, LHTreeNode* up)
    {
        m_parent      = 0;
        m_origCluster = c;
        m_node        = 0;
        m_type        = Compound;
        m_down        = 0;

        m_up = up;
        if(up)
            up->m_down = this;
    }

    LHTreeNode(LHTreeNode* parent, node v, Type t = Node)
    {
        m_parent      = parent;
        m_origCluster = 0;
        m_node        = v;
        m_type        = t;
        m_up          = 0;
        m_down        = 0;
    }

    // Access functions
    bool isCompound() const
    {
        return m_type == Compound;
    }

    int numberOfChildren() const
    {
        return m_child.size();
    }

    const LHTreeNode* parent() const
    {
        return m_parent;
    }
    const LHTreeNode* child(int i) const
    {
        return m_child[i];
    }

    cluster originalCluster() const
    {
        return m_origCluster;
    }
    node    getNode()         const
    {
        return m_node;
    }

    const LHTreeNode* up() const
    {
        return m_up;
    }
    const LHTreeNode* down() const
    {
        return m_down;
    }

    int pos() const
    {
        return m_pos;
    }


    // Modification functions
    LHTreeNode* parent()
    {
        return m_parent;
    }
    void setParent(LHTreeNode* p)
    {
        m_parent = p;
    }

    LHTreeNode* child(int i)
    {
        return m_child[i];
    }
    void initChild(int n)
    {
        m_child.init(n);
    }
    void setChild(int i, LHTreeNode* p)
    {
        m_child[i] = p;
    }

    void setPos();

    void store()
    {
        m_storedChild = m_child;
    }
    void restore()
    {
        m_child = m_storedChild;
    }
    void permute()
    {
        m_child.permute();
    }

    void removeAuxChildren();

    List<Adjacency> m_upperAdj;
    List<Adjacency> m_lowerAdj;
    List<ClusterCrossing> m_upperClusterCrossing;
    List<ClusterCrossing> m_lowerClusterCrossing;

private:
    LHTreeNode*        m_parent;

    cluster            m_origCluster;
    node               m_node;
    Type               m_type;

    Array<LHTreeNode*> m_child;
    Array<LHTreeNode*> m_storedChild;

    LHTreeNode*        m_up;
    LHTreeNode*        m_down;
    int                m_pos;

    OGDF_NEW_DELETE
};


//---------------------------------------------------------
// ENGLayer
//---------------------------------------------------------
class OGDF_EXPORT ENGLayer
{
public:
    ENGLayer()
    {
        m_root = 0;
    }
    ~ENGLayer();

    const LHTreeNode* root() const
    {
        return m_root;
    }
    LHTreeNode* root()
    {
        return m_root;
    }

    void setRoot(LHTreeNode* r)
    {
        m_root = r;
    }

    void store();
    void restore();
    void permute();

    void simplifyAdjacencies();
    void removeAuxNodes();

private:
    void simplifyAdjacencies(List<LHTreeNode::Adjacency> & adjs);

    LHTreeNode* m_root;
};


//---------------------------------------------------------
// ClusterGraphCopy
//---------------------------------------------------------
class OGDF_EXPORT ExtendedNestingGraph;

class OGDF_EXPORT ClusterGraphCopy : public ClusterGraph
{
public:

    ClusterGraphCopy();
    ClusterGraphCopy(const ExtendedNestingGraph & H, const ClusterGraph & CG);

    void init(const ExtendedNestingGraph & H, const ClusterGraph & CG);

    const ClusterGraph & getOriginalClusterGraph() const
    {
        return *m_pCG;
    }

    cluster copy(cluster cOrig) const
    {
        return m_copy[cOrig];
    }
    cluster original(cluster cCopy) const
    {
        return m_original[cCopy];
    }

    void setParent(node v, cluster c);

private:
    void createClusterTree(cluster cOrig);

    const ClusterGraph*         m_pCG;
    const ExtendedNestingGraph* m_pH;

    ClusterArray<cluster> m_copy;
    ClusterArray<cluster> m_original;
};


//---------------------------------------------------------
// ExtendedNestingGraph
//---------------------------------------------------------
class OGDF_EXPORT ExtendedNestingGraph : public Graph
{
public:
    // the type of a node in this copy
    enum NodeType { ntNode, ntClusterTop, ntClusterBottom, ntDummy, ntClusterTopBottom };

    ExtendedNestingGraph(const ClusterGraph & CG);

    const ClusterGraphCopy & getClusterGraph() const
    {
        return m_CGC;
    }
    const ClusterGraph & getOriginalClusterGraph() const
    {
        return m_CGC.getOriginalClusterGraph();
    }

    node copy(node v)    const
    {
        return m_copy[v];
    }
    node top(cluster cOrig) const
    {
        return m_topNode[cOrig];
    }
    node bottom(cluster cOrig) const
    {
        return m_bottomNode[cOrig];
    }

    int topRank(cluster c) const
    {
        return m_topRank[c];
    }
    int bottomRank(cluster c) const
    {
        return m_bottomRank[c];
    }


    NodeType type(node v) const
    {
        return m_type[v];
    }
    node    origNode(node v) const
    {
        return m_origNode[v];
    }
    edge    origEdge(edge e) const
    {
        return m_origEdge[e];
    }

    cluster originalCluster(node v) const
    {
        return m_CGC.original(m_CGC.clusterOf(v));
    }
    cluster parent(node v) const
    {
        return m_CGC.clusterOf(v);
    }
    cluster parent(cluster c) const
    {
        return c->parent();
    }
    bool isVirtual(cluster c) const
    {
        return m_CGC.original(c) == 0;
    }

    const List<edge> & chain(edge e) const
    {
        return m_copyEdge[e];
    }

    // is edge e reversed ?
    bool isReversed(edge e) const
    {
        return e->source() != origNode(chain(e).front()->source());
    }

    bool isLongEdgeDummy(node v) const
    {
        return (type(v) == ntDummy && v->outdeg() == 1);
    }

    bool verticalSegment(edge e) const
    {
        return m_vertical[e];
    }

    int numberOfLayers() const
    {
        return m_numLayers;
    }
    int rank(node v) const
    {
        return m_rank[v];
    }
    int pos(node v) const
    {
        return m_pos[v];
    }
    const LHTreeNode* layerHierarchyTree(int i) const
    {
        return m_layer[i].root();
    }
    const ENGLayer & layer(int i) const
    {
        return m_layer[i];
    }

    RCCrossings reduceCrossings(int i, bool dirTopDown);
    void storeCurrentPos();
    void restorePos();
    void permute();

    void removeTopBottomEdges();

    int aeLevel(node v) const
    {
        return m_aeLevel[v];
    }

protected:
    cluster lca(node u, node v) const;
    LHTreeNode* lca(
        LHTreeNode* uNode,
        LHTreeNode* vNode,
        LHTreeNode** uChild,
        LHTreeNode** vChild) const;

    edge addEdge(node u, node v, bool addAlways = false);
    void assignAeLevel(cluster c, int & count);
    bool reachable(node v, node u, SListPure<node> & successors);
    void moveDown(node v, const SListPure<node> & successors, NodeArray<int> & level);
    bool tryEdge(node u, node v, Graph & G, NodeArray<int> & level);

    RCCrossings reduceCrossings(LHTreeNode* cNode, bool dirTopDown);
    void assignPos(const LHTreeNode* vNode, int & count);

private:
    void computeRanking();
    void createDummyNodes();
    void createVirtualClusters();
    void createVirtualClusters(
        cluster c,
        NodeArray<node> & vCopy,
        ClusterArray<node> & cCopy);
    void buildLayers();
    void removeAuxNodes();

    // original graph
    //const ClusterGraph &m_CG;
    ClusterGraphCopy m_CGC;

    // mapping: nodes in CG <-> nodes in this copy
    NodeArray<node>    m_copy;
    NodeArray<node>    m_origNode;

    // mapping: clusters in CG <-> nodes in this copy
    ClusterArray<node> m_topNode;     // the node representing top-most part of cluster (min. rank)
    ClusterArray<node> m_bottomNode;  // the node representing bottom-most part of cluster (max. rank)
    ClusterArray<int> m_topRank;
    ClusterArray<int> m_bottomRank;

    // the type of a node in this copy
    NodeArray<NodeType> m_type;

    // mapping: edges in CG <-> edges in this copy
    EdgeArray<List<edge> > m_copyEdge;
    EdgeArray<edge>        m_origEdge;

    // level of each node
    NodeArray<int>     m_rank;
    int                m_numLayers;

    // the layers
    Array<ENGLayer> m_layer;
    // positions within a layer
    NodeArray<int>  m_pos;

    // can an edge segment be drawn vertically?
    EdgeArray<bool> m_vertical;

    // temporary data for "addEdge()"
    NodeArray<int>  m_aeLevel;
    NodeArray<bool> m_aeVisited;
    NodeArray<int>  m_auxDeg;

    // temporary data for "lca()"
    mutable ClusterArray<cluster> m_mark;
    mutable SListPure<cluster>    m_markedClusters;
    mutable cluster               m_secondPath;
    mutable node                  m_secondPathTo;
    mutable SListPure<cluster>    m_markedClustersTree;
    mutable ClusterArray<LHTreeNode*> m_markTree;
};


} // end namespace ogdf


#endif
