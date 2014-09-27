/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief MLG is the main data structure for ModularMultilevelMixer
 *
 * \author Gereon Bartel
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

#ifndef OGDF_MULTILEVEL_GRAPH_H
#define OGDF_MULTILEVEL_GRAPH_H

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <vector>
#include <map>

namespace ogdf
{

//Stores info on merging for a refinement level
struct NodeMerge
{
    // Node/Edge IDs instead of pointers as the nodes themselves may be nonexistent.
    std::vector<int> m_deletedEdges;
    std::vector<int> m_changedEdges;
    std::map<int, double> m_doubleWeight; // for changed and deleted edges
    std::map<int, int> m_source;
    std::map<int, int> m_target;

    int m_mergedNode;
    std::vector< std::pair<int, double> > m_position; // optional information <target, distance>. mergedNode will be placed at average of relative distances to target.

    std::vector<int> m_changedNodes; // there may be placement strategies that use more than one reference-node.
    std::map<int, double> m_radius; // for changed nodes and the merged node

    int m_level;


    NodeMerge(int level) : m_level(level) { }
    ~NodeMerge() { }
};


class OGDF_EXPORT MultilevelGraph
{
private:
    bool m_createdGraph; //used in destructor, TODO: check if it is needed
    Graph* m_G;
    GraphAttributes* m_GA;  //<! Keeps layout info in replacement of information below (todo: remove them)
    std::vector<NodeMerge*> m_changes;
    NodeArray<double> m_radius;
    double m_avgRadius; //stores average node radius for scaling and random layout purposes

    EdgeArray<double> m_weight;

    // Associations to index only as the node/edge may be nonexistent
    NodeArray<int> m_nodeAssociations;
    EdgeArray<int> m_edgeAssociations;

    std::vector<node> m_reverseNodeIndex;
    std::vector<int> m_reverseNodeMergeWeight;//<! Keeps number of vertices represented by vertex with given index
    std::vector<edge> m_reverseEdgeIndex;

    MultilevelGraph* removeOneCC(std::vector<node> & componentSubArray);
    void copyFromGraph(const Graph & G, NodeArray<int> & nodeAssociations, EdgeArray<int> & edgeAssociations);
    void prepareGraphAttributes(GraphAttributes & GA) const;

    void initReverseIndizes();
    void initInternal();

public:
    ~MultilevelGraph();
    MultilevelGraph();
    MultilevelGraph(Graph & G);
    MultilevelGraph(GraphAttributes & GA);
    // if the Graph is available without const, no copy needs to be created.
    MultilevelGraph(GraphAttributes & GA, Graph & G);

    // creates MultilevelGraph directly from GML file.
    MultilevelGraph(istream & is);
    MultilevelGraph(const String & filename);

    NodeArray<double> & getRArray()
    {
        return m_radius;
    }
    EdgeArray<double> & getWArray()
    {
        return m_weight;
    }

    edge getEdge(unsigned int index);
    node getNode(unsigned int index);

    double radius(node v)
    {
        return m_radius[v];
    }
    void radius(node v, double r)
    {
        m_radius[v] = r;
    }
    double averageRadius() const
    {
        return m_avgRadius;
    }

    double x(node v)
    {
        return m_GA->x(v);
    }
    double y(node v)
    {
        return m_GA->y(v);
    }
    void x(node v, double x)
    {
        m_GA->x(v) = x;
    }
    void y(node v, double y)
    {
        m_GA->y(v) = y;
    }

    void weight(edge e, double weight)
    {
        m_weight[e] = weight;
    }
    double weight(edge e)
    {
        return m_weight[e];
    }

    //returns the merge weight, i.e. the number of nodes represented by v on the current level
    int mergeWeight(node v)
    {
        return m_reverseNodeMergeWeight[v->index()];
    }

    void moveToZero();

    int getLevel();
    Graph & getGraph()
    {
        return *m_G;
    }
    //! Returns attributes of current level graph as GraphAttributes
    GraphAttributes & getGraphAttributes() const
    {
        return *m_GA;
    }
    void exportAttributes(GraphAttributes & GA) const;
    void exportAttributesSimple(GraphAttributes & GA) const;
    void importAttributes(const GraphAttributes & GA);
    void importAttributesSimple(const GraphAttributes & GA);
    void reInsertGraph(MultilevelGraph & MLG);
    void reInsertAll(std::vector<MultilevelGraph*> components);
    void copyNodeTo(node v, MultilevelGraph & MLG, std::map<node, node> & tempNodeAssociations, bool associate, int index = -1);
    void copyEdgeTo(edge e, MultilevelGraph & MLG, std::map<node, node> & tempNodeAssociations, bool associate, int index = -1);
    void writeGML(ostream & os);
    void writeGML(const String & fileName);

    // the original graph will be cleared to save Memory
    std::vector<MultilevelGraph*> splitIntoComponents();

    bool postMerge(NodeMerge* NM, node merged);
    //\a merged is the node now represented by \a theNode
    bool changeNode(NodeMerge* NM, node theNode, double newRadius, node merged);
    bool changeEdge(NodeMerge* NM, edge theEdge, double newWeight, node newSource, node newTarget);
    bool deleteEdge(NodeMerge* NM, edge theEdge);
    std::vector<edge> moveEdgesToParent(NodeMerge* NM, node theNode, node parent, bool deleteDoubleEndges, int adjustEdgeLengths);
    NodeMerge* getLastMerge();
    node undoLastMerge();

    void updateReverseIndizes();
    //sets the merge weights back to initial values
    void updateMergeWeights();
};

} // namespace ogdf

#endif
