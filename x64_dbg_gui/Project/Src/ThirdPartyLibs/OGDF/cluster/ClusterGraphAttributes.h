/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declares ClusterGraphAttributes, an extension of class
 * GraphAttributes,  to store clustergraph layout informations
 * like cluster cage positions and sizes that can be accessed
 * over the cluster/cluster ID
 *
 * \author Karsten Klein
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

#ifndef OGDF_CLUSTER_GRAPH_ATTRIBUTES_H
#define OGDF_CLUSTER_GRAPH_ATTRIBUTES_H

#include <ogdf/basic/HashArray.h>
#include <ogdf/cluster/ClusterArray.h>

#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/basic/GraphAttributes.h>


namespace ogdf
{

class GmlParser;

/**
 * \brief Stores information associated with a cluster.
 *
 */
class OGDF_EXPORT ClusterInfo
{
public:
    ClusterInfo()
    {
        //&m_fillPattern = GraphAttributes::bpNone; //NoBrush
        m_lineStyle = GraphAttributes::esSolid;
    }

    double m_x, m_y; //position of lower left corner
    double m_w, m_h; //width and height

    double m_lineWidth; //width of rectangle border line

    String m_color;  //color of rectangle
    String m_fillColor;  //color of fill area
    String m_backColor;  //background color
    GraphAttributes::EdgeStyle m_lineStyle;  //rectangle line style
    GraphAttributes::BrushPattern m_fillPattern; //brush pattern of fill area
    String m_label;  //name label

    int m_clusterID; //the ID of the cluster of which the info is stored
};


/**
 * \brief Stores additional attributes of a clustered graph (like layout information).
 *
 * Attributes are simply stored in node or edge arrays; for memory consumption
 * reasons, only a subset of these arrays is in fact initialized for the graph;
 * non-initialized arrays require only a few bytes of extra memory.
 *
 * Which arrays are initialized is specified by a bit vector; each bit in this
 * bit vector corresponds to one or more attributes. E.g., \a #nodeGraphics
 * corresponds to the attributes \a #m_x, \a #m_y, \a #m_width, and \a #m_height;
 * whereas \a #edgeDoubleWeight only corresponds to the attribute \a #m_doubleWeight.
 *
 */
class OGDF_EXPORT ClusterGraphAttributes : public GraphAttributes
{
private:
    ClusterArray<String> m_clusterTemplate; //!< Name of cluster template.

public:
    //! Initializes new instance of class ClusterGraphAttributes.
    ClusterGraphAttributes() : GraphAttributes(), m_pClusterGraph(0) { }

    //! Initializes new instance of class ClusterGraphAttributes.
    //! Uses \a initAttributes, which are enriched by node and edge
    //! graphics.
    ClusterGraphAttributes(ClusterGraph & cg, long initAttributes = 0);

    virtual ~ClusterGraphAttributes() { }

    //! Initializes the instance with ClusterGraph \a cg.
    //! Sets the attributes to \a initAttributes
    virtual void init(ClusterGraph & cg, long initAttributes = 0);

    //! Initializes the attributes according to \a initAttributes.
    virtual void initAtt(long initAttributes = 0)
    {
        GraphAttributes::initAttributes(initAttributes);
    }

    //          operator const ClusterGraph& () const {return *m_pClusterGraph;}
    //          operator const Graph& () const {return m_pClusterGraph->getGraph();}

    //! Returns the ClusterGraph.
    const ClusterGraph & constClusterGraph() const
    {
        return *m_pClusterGraph;
    }

    //! Returns the index of the parent cluster of node \a v.
    int clusterID(node v)
    {
        return m_pClusterGraph->clusterOf(v)->index();
    }

    //! Returns the parent cluster of node \a v.
    cluster clusterOf(node v)
    {
        return m_pClusterGraph->clusterOf(v);
    }

    //! Returns the maximum cluster index used.
    int maxClusterID() const
    {
        return m_pClusterGraph->clusterIdCount() - 1;
    }

    //! Updates positions of cluster boundaries wrt to children and child clusters
    void updateClusterPositions(double boundaryDist = 1.0);

    //*****************************************************************
    //data access by ID

    //! Returns x position of the cluster cages lower left corner.
    double clusterXPos(int clusterID) const
    {
        return m_clusterInfo[clusterID].m_x;
    }

    //! Returns x position of the cluster cages lower left corner.
    double & clusterXPos(int clusterID)
    {
        return m_clusterInfo[clusterID].m_x;
    }

    //! Returns y position of the cluster cages lower left corner.
    double clusterYPos(int clusterID) const
    {
        return m_clusterInfo[clusterID].m_y;
    }

    //! Returns y position of the cluster cages lower left corner.
    double & clusterYPos(int clusterID)
    {
        return m_clusterInfo[clusterID].m_y;
    }

    //! Returns cluster cage height.
    double clusterHeight(int clusterID) const
    {
        return m_clusterInfo[clusterID].m_h;
    }

    //! Returns cluster cage height.
    double & clusterHeight(int clusterID)
    {
        return m_clusterInfo[clusterID].m_h;
    }

    //! Returns cluster cage width.
    double clusterWidth(int clusterID) const
    {
        return m_clusterInfo[clusterID].m_w;
    }

    //! Returns cluster cage width.
    double & clusterWidth(int clusterID)
    {
        return m_clusterInfo[clusterID].m_w;
    }

    //! Returns cluster line width.
    double clusterLineWidth(int clusterID) const
    {
        return m_clusterInfo[clusterID].m_lineWidth;
    }

    //! Returns cluster line width.
    double & clusterLineWidth(int clusterID)
    {
        return m_clusterInfo[clusterID].m_lineWidth;
    }

    //! Returns cluster fill color.
    const String & clusterFillColor(int clusterID) const
    {
        return m_clusterInfo[clusterID].m_fillColor;
    }

    //! Returns cluster fill color.
    String & clusterFillColor(int clusterID)
    {
        return m_clusterInfo[clusterID].m_fillColor;
    }

    //! Returns cluster fill pattern.
    GraphAttributes::BrushPattern clusterFillPattern(int clusterID) const
    {
        return m_clusterInfo[clusterID].m_fillPattern;
    }

    //! Returns cluster fill pattern.
    GraphAttributes::BrushPattern & clusterFillPattern(int clusterID)
    {
        return m_clusterInfo[clusterID].m_fillPattern;
    }

    //! Returns label of cluster c.
    const String & clusterLabel(int clusterID) const
    {
        return m_clusterInfo[clusterID].m_label;
    }

    //! Returns label of cluster c.
    String & clusterLabel(int clusterID)
    {
        return m_clusterInfo[clusterID].m_label;
    }

    //! Returns structure containing information on cluster with ID \a clusterID.
    const ClusterInfo & clusterInfo(int clusterID) const
    {
        return m_clusterInfo[clusterID];
    }

    //! Returns structure containing information on cluster with ID \a clusterID.
    ClusterInfo & clusterInfo(int clusterID)
    {
        return m_clusterInfo[clusterID];
    }


    //*****************************************************************
    //data access by cluster

    //! Returns x position of the cluster cages lower left corner.
    double clusterXPos(cluster c) const
    {
        return m_clusterInfo[c->index()].m_x;
    }

    //! Returns x position of the cluster cages lower left corner.
    double & clusterXPos(cluster c)
    {
        return m_clusterInfo[c->index()].m_x;
    }

    //! Returns y position of the cluster cages lower left corner.
    double clusterYPos(cluster c) const
    {
        return m_clusterInfo[c->index()].m_y;
    }

    //! Returns y position of the cluster cages lower left corner.
    double & clusterYPos(cluster c)
    {
        return m_clusterInfo[c->index()].m_y;
    }

    //! Returns cluster cage height.
    double clusterHeight(cluster c) const
    {
        return m_clusterInfo[c->index()].m_h;
    }

    //! Returns cluster cage height.
    double & clusterHeight(cluster c)
    {
        return m_clusterInfo[c->index()].m_h;
    }

    //! Returns cluster cage width.
    double clusterWidth(cluster c) const
    {
        return m_clusterInfo[c->index()].m_w;
    }

    //! Returns cluster cage width.
    double & clusterWidth(cluster c)
    {
        return m_clusterInfo[c->index()].m_w;
    }

    //! Returns label of cluster c.
    const String & clusterLabel(cluster c) const
    {
        return m_clusterInfo[c->index()].m_label;
    }

    //! Returns label of cluster c.
    String & clusterLabel(cluster c)
    {
        return m_clusterInfo[c->index()].m_label;
    }

    //! Returns const reference to template of cluster c.
    const String & templateCluster(cluster c) const
    {
        return m_clusterTemplate[c];
    }

    //! Returns reference to template of cluster c.
    String & templateCluster(cluster c)
    {
        return m_clusterTemplate[c];
    }

    //! Returns const reference to structure containing information on cluster \a c.
    const ClusterInfo & clusterInfo(cluster c) const
    {
        return m_clusterInfo[c->index()];
    }

    //! Returns reference to structure containing information on cluster \a c.
    ClusterInfo & clusterInfo(cluster c)
    {
        return m_clusterInfo[c->index()];
    }

    //! Returns line color stored for cluster \a c in string format.
    const String & clusterColor(cluster c) const
    {
        return  m_clusterInfo[c->index()].m_color;
    }

    //! Returns line color of cluster \a c in string format.
    String & clusterColor(cluster c)
    {
        return  m_clusterInfo[c->index()].m_color;
    }

    //Returns const reference to fill color of cluster \a c.
    const String & clusterFillColor(cluster c) const
    {
        return  m_clusterInfo[c->index()].m_fillColor;
    }

    //Returns reference to fill color of cluster \a c.
    String & clusterFillColor(cluster c)
    {
        return  m_clusterInfo[c->index()].m_fillColor;
    }

    //Returns const reference to background color of cluster \a c.
    const String & clusterBackColor(cluster c) const
    {
        return  m_clusterInfo[c->index()].m_backColor;
    }

    //Returns reference to background color of cluster \a c.
    String & clusterBackColor(cluster c)
    {
        return  m_clusterInfo[c->index()].m_backColor;
    }


    //pen and brush styles

    //! Returns edge style of cluster \a c.
    const GraphAttributes::EdgeStyle & clusterLineStyle(cluster c) const
    {
        return  m_clusterInfo[c->index()].m_lineStyle;
    }

    //! Returns line style of cluster \a c.
    GraphAttributes::EdgeStyle & clusterLineStyle(cluster c)
    {
        return  m_clusterInfo[c->index()].m_lineStyle;
    }

    //! Returns brush pattern of cluster \a c.
    const GraphAttributes::BrushPattern & clusterFillPattern(cluster c) const
    {
        return  m_clusterInfo[c->index()].m_fillPattern;
    }

    //! Returns brush pattern of cluster c.
    GraphAttributes::BrushPattern & clusterFillPattern(cluster c)
    {
        return  m_clusterInfo[c->index()].m_fillPattern;
    }

    //! Returns line width of cluster \a c.
    const double & clusterLineWidth(cluster c) const
    {
        return  m_clusterInfo[c->index()].m_lineWidth;
    }

    //! Returns line width of cluster c.
    double & clusterLineWidth(cluster c)
    {
        return  m_clusterInfo[c->index()].m_lineWidth;
    }

    //! Set fill pattern \a i for cluster \a c.
    void setClusterFillPattern(cluster c, int i)
    {
        m_clusterInfo[c->index()].m_fillPattern = intToPattern(i);
    }

    //! Set style \a i for cluster \a c.
    void setClusterLineStyle(cluster c, int i)
    {
        m_clusterInfo[c->index()].m_lineStyle = intToStyle(i);
    }

    //! Returns bounding box.
    const DRect boundingBox() const;

    //! Writes attributed clustergraph in GML format to file fileName.
    void writeGML(const char* fileName);

    //! Writes attributed clustergraph in GML format to output stream \a os.
    void writeGML(ostream & os);

    //we don't have GraphConstraints yet
    //! Writes attributed clustergraph in OGML format to file fileName
    void writeOGML(const char* fileName); //, GraphConstraints & GC);

    //! Writes attributed clustergraph in OGML format to output stream \a os.
    void writeOGML(ostream & os);//, GraphConstraints & GC);

    //! Reads attributed clustergraph in GML format from file fileName.
    bool readClusterGML(
        const char* fileName,
        ClusterGraph & CG,
        Graph & G);

    //! Reads attributed clustergraph in GML format from input stream \a is.
    bool readClusterGML(
        istream & is,
        ClusterGraph & CG,
        Graph & G);

    //! Reads clustered graph from OGML-file.
    bool readClusterGraphOGML(
        const char* fileName,
        ClusterGraph & CG,
        Graph & G);

protected:
    const ClusterGraph* m_pClusterGraph;//!< Only points to existing graphs.

private:
    //! Information on the cluster positions, index is cluster ID.
    HashArray<int, ClusterInfo> m_clusterInfo;

    //! Reads Cluster Graph with Attributes, base graph \a G, from \a fileName
    //! with a given gmlparser \a gml (has objecttree).
    //! Input stream given by parser.
    bool readClusterGraphGML(
        const char* fileName,
        ClusterGraph & CG,
        Graph & G,
        GmlParser & gml);

    //! Reads clustered graph from input stream of GmlParser.
    bool readClusterGraphGML(
        ClusterGraph & CG,
        Graph & G,
        GmlParser & gml);

    //! Recursively writes the cluster structure in GML format into output stream \a os.
    void writeCluster(
        ostream & os,
        NodeArray<int> & nId,
        ClusterArray<int> & cId,
        int & nextId,
        cluster c,
        String indent);

    //! Recursively writes the cluster structure in GraphWin GML format.
    void writeGraphWinCluster(
        ostream & os,
        NodeArray<int> & nId,
        int & nextId,
        cluster c,
        String indent);

    //! Recursively writes the cluster structure in OGML.
    void writeClusterOGML(
        ostream & os,
        std::ostringstream & osS, // string stream for styles block
        int & nextLabelId,
        cluster cluster,
        int & indentDepth, // indent depth for structure block
        int indentDepthS); // indent depth for styles block

};


} // end namespace ogdf


#endif
