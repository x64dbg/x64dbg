/*
 * $Revision: 2585 $
 *
 * last checkin:
 *   $Author: klein $
 *   $Date: 2012-07-12 03:46:50 +0200 (Do, 12. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class GraphAttributes which extends a Graph
 *        by additional attributes.
 *
 * \author Carsten Gutwenger
 *         Karsten Klein
 *         Joachim Kupke
 *         Sebastian Leipert
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

#ifndef OGDF_ATTRIBUTED_GRAPH_H
#define OGDF_ATTRIBUTED_GRAPH_H

#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/String.h>
#include <ogdf/basic/geometry.h>

namespace ogdf
{

//---------------------------------------------------------
// GraphAttributes
// graph topology + graphical attributes
//---------------------------------------------------------
//! Stores additional attributes of a graph (like layout information).
/**
 * It is frequently necessary to associate additional attributes with a graph.
 * The class GraphAttributes provides various such attributes and is the
 * central place were such attributes are stored.
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
 * Attributes can be initialized by the constructor GraphAttributes(const Graph &,long)
 * or the function initAttributes(); attributes can also be deinitialized by
 * calling destroyAttributes().
 */

class OGDF_EXPORT GraphAttributes
{
public:
    //! Types for edge arrows.
    enum EdgeArrow
    {
        none, //!< no edge arrows
        last, //!< edge arrow at target node of the edge
        first, //!< edge arrow at source node of the edge
        both, //!< edge arrow at target and source node of the edge
        undefined
    };

    //! Types for line styles.
    /**
     * The line styles are preliminary the same as in QT.
     */
    enum EdgeStyle
    {
        esNoPen = 0, //!< no line
        esSolid = 1, //!< solid line
        esDash = 2, //!< dashed line
        esDot = 3, //!< dotted line
        esDashdot = 4, //!< line style "dash dot dash dot ..."
        esDashdotdot = 5
                       //!< line style "dash dot dot dash dot dot ..."
    };

    //! Converts integer \a i to edge style.
    static EdgeStyle intToStyle(int i)
    {
        switch(i)
        {
        case 0:
            return esNoPen;
        case 1:
            return esSolid;
        case 2:
            return esDash;
        case 3:
            return esDot;
        case 4:
            return esDashdot;
        case 5:
            return esDashdotdot;
        default:
            return esNoPen;

        }
    }

    //! Types for object brush patterns.
    /**
     * The brush patterns are currently the same as the GDE project.
     */
    enum BrushPattern
    {
        bpNone = 0,
        bpSolid = 1,
        bpDense1 = 2,
        bpDense2 = 3,
        bpDense3 = 4,
        bpDense4 = 5,
        bpDense5 = 6,
        bpDense6 = 7,
        bpDense7 = 8,
        bpHorizontal = 9,
        bpVertical = 10,
        bpCross = 11,
        BackwardDiagonal = 12,
        ForwardDiagonal = 13,
        DiagonalCross = 14
    };

    //! Converts integer \a i to brush pattern.
    static BrushPattern intToPattern(int i)
    {
        switch(i)
        {
        case 0:
            return bpNone;
            break;
        case 1:
            return bpSolid;
            break;
        case 2:
            return bpDense1;
            break;
        case 3:
            return bpDense2;
            break;
        case 4:
            return bpDense3;
            break;
        case 5:
            return bpDense4;
            break;
        case 6:
            return bpDense5;
            break;
        case 7:
            return bpDense6;
            break;
        case 8:
            return bpDense7;
            break;
        case 9:
            return bpHorizontal;
            break;
        case 10:
            return bpVertical;
            break;
        case 11:
            return bpCross;
            break;
        case 12:
            return BackwardDiagonal;
            break;
        case 13:
            return ForwardDiagonal;
            break;
        case 14:
            return DiagonalCross;
            break;
        default:
            return bpNone;
            break;
        }
    }

    //! Specifies scaling of images.
    enum ImageStyle
    {
        FreeScale = 0, FixScale = 1
    };
    //! Specifies image alignment.
    enum ImageAlignment
    {
        TopLeft = 0, TopCenter, TopRight, CenterLeft, Center, CenterRight, BottomLeft, BottomCenter, BottomRight
    };

    //! Helper function mapping int values to image styles
    static ImageStyle intToImageStyle(int i)
    {
        switch(i)
        {
        case 0:
            return FreeScale;
            break;
        case 1:
            return FixScale;
            break;
        default:
            return FreeScale;
        }//switch
    }//intToStyle

    //! Helper function mapping int values to image alignment
    static ImageAlignment intToImageAlignment(int i)
    {
        switch(i)
        {
        case 0:
            return TopLeft;
            break;
        case 1:
            return TopCenter;
            break;
        case 2:
            return TopRight;
            break;
        case 3:
            return CenterLeft;
            break;
        case 4:
            return Center;
            break;
        case 5:
            return CenterRight;
            break;
        case 6:
            return BottomLeft;
            break;
        case 7:
            return BottomCenter;
            break;
        case 8:
            return BottomRight;
            break;
        default:
            return TopLeft;
        }//switch
    }//intToAlignment

protected:
    /**
     * Writes string \a str into a GML file such that line length limits
     * are respected and characters '\', '"' are correctly escaped.
     */
    void writeLongString(ostream & os, const String & str) const;

    /* Methods for OGML serialization */

    //! Static helper method for mapping edge styles to ogml.
    static const char* edgeStyleToOGML(const GraphAttributes::EdgeStyle & edgeStyle);

    //! Static helper method for mapping image alignments to ogml.
    static const char* imageAlignmentToOGML(const GraphAttributes::ImageAlignment & imgAlign);

    //! Static helper method for mapping image style to ogml.
    static const char* imageStyleToOGML(const GraphAttributes::ImageStyle & imgStyle);

    //! Static helper method for mapping brush patterns styles to ogml.
    static const char* brushPatternToOGML(const GraphAttributes::BrushPattern & brushPattern);

    //static void generateIndent(char ** indent, const int & indentSize);

    //! Static helper method for exchanging X(HT)ML-tag specific chars.
    String formatLabel(const String & labelText);

    /* End methods for OGML serialization */

    const Graph* m_pGraph; //!< associated graph

    bool m_directed; //!< whether or not the graph is directed

    // graphical representation of nodes
    NodeArray<double> m_x; //!< x-coordinate of a node
    NodeArray<double> m_y; //!< y-coordinate pf a node
    NodeArray<double> m_width; //!< width of a node's bounding box
    NodeArray<double> m_height; //!< height of a nodes's bounding box
    NodeArray<String> m_nodeLabel; //!< label of a node
    NodeArray<String> m_nodeColor; //!< color of a node
    NodeArray<String> m_nodeLine; //!< line color of a node
    NodeArray<int> m_nodeShape; //!< shape of a node
    NodeArray<double> m_nodeLineWidth; //!< line width of a node
    NodeArray<BrushPattern> m_nodePattern; //!< brush pattern of a node
    NodeArray<EdgeStyle> m_nodeStyle; //!< line style of a node
    NodeArray<String> m_nodeTemplate; //!< name of template of a node

    // images
    NodeArray<String> m_imageUri;
    NodeArray<ImageStyle> m_imageStyle;
    NodeArray<ImageAlignment> m_imageAlign;
    NodeArray<bool> m_imageDrawLine;
    NodeArray<double> m_imageWidth;
    NodeArray<double> m_imageHeight;

    // other node attributes
    NodeArray<int> m_nodeId; //!< user ID of a node
    NodeArray<int> m_level; //!< level of a node
    NodeArray<int> m_nodeIntWeight; //!< (integer) weight of a node
    NodeArray<Graph::NodeType> m_vType; // type (vertex, dummy, generalizationMerger)

    // graphical representation of edges
    EdgeArray<DPolyline> m_bends; //!< list of bend points of an edge
    EdgeArray<String> m_edgeLabel; //!< label of an edge
    EdgeArray<EdgeArrow> m_edgeArrow; //!< arrow type of an edge
    EdgeArray<EdgeStyle> m_edgeStyle; //!< line style of an edge
    EdgeArray<String> m_edgeColor; //!< line color of an edge
    EdgeArray<double> m_edgeWidth; //!< line width of an edge
    EdgeArray<Graph::EdgeType> m_eType; //!< type of an edge (association or generalization)

    // other edge attributes
    EdgeArray<int> m_intWeight; //!< (integer) weight of an edge
    EdgeArray<double> m_doubleWeight; //!< (real number) weight of an edge
    EdgeArray<unsigned int> m_subGraph; //!< is element of subgraphs given by bitvector

    long m_attributes; //!< bit vector of currently used attributes

public:
    //! Bits for specifying attributes.
    enum
    {
        nodeGraphics = 0x00001, //!< node attributes m_x, m_y, m_width, m_height, m_nodeShape
        edgeGraphics = 0x00002, //!< edge attribute m_bends
        nodeLevel = 0x00004, //!< node attribute m_level
        edgeIntWeight = 0x00008, //!< edge attribute m_intWeight
        edgeDoubleWeight = 0x00010, //!< edge attribute m_doubleWeight
        edgeLabel = 0x00020, //!< edge attribute m_edgeLabel
        nodeLabel = 0x00040, //!< node attribute m_nodeLabel
        edgeType = 0x00080, //!< edge attribute m_eType
        nodeType = 0x00100, //!< node attribute m_vType
        nodeColor = 0x00200, //!< node attribute m_nodeColor, m_nodeLine
        nodeId = 0x00400, //!< node attribute m_nodeId
        edgeArrow = 0x00800, //!< edge attribute m_edgeArrow
        edgeColor = 0x01000, //!< edge attribute m_edgeColor
        edgeStyle = 0x02000, //!< edge attribute m_edgeStyle, m_edgeWidth
        nodeStyle = 0x04000, //!< node attributes m_nodePattern, m_nodeStyle, m_nodeLineWidth;
        //!< experimental: m_imageUri, m_imageStyle, m_imageAlign,
        //!< m_imageDrawLine, m_imageWidth, m_imageHeight
        nodeTemplate = 0x08000, //!< node attribute m_nodeTemplate
        edgeSubGraph = 0x10000, //!< edge attribute m_subGraph
        nodeWeight = 0x20000
                     //!< node attribute m_nodeIntWeight
    };

    //! Bits for specifying node shapes.
    enum
    {
        oval = 0x8001, rectangle = 0x8002
    };

    //! Constructs graph attributes for no associated graph (default constructor).
    /**
     * The associated graph can be set later with the init() function.
     */
    GraphAttributes();

    //! Constructs graph attributes associated with the graph \a G.
    /**
     * @param G is the associated graph.
     * @param initAttributes specifies the set of attributes that can be accessed.
     */
    GraphAttributes(const Graph & G, long initAttributes = nodeGraphics | edgeGraphics);

    virtual ~GraphAttributes()
    {
    }

    //! Initializes the graph attributes for graph \a G.
    /**
     * @param G is the new associated graph.
     * @param initAttr specifies the set of attributes that can be accessed.
     *
     * \warning All attributes that were allocated before are destroyed by this function!
     *  If you wish to extend the set of allocated attributes, use initAttributes().
     */
    virtual void init(const Graph & G, long initAttr);

    //! Returns currently accessible attributes.
    long attributes() const
    {
        return m_attributes;
    }

    //! Initializes attributes in \a attr for usage.
    void initAttributes(long attr);

    //! Destroys attributes in attr.
    void destroyAttributes(long attr);

    //! Returns a reference to the associated graph
    const Graph & constGraph() const
    {
        return *m_pGraph;
    }

    //! Returns if the graph is directed.
    bool directed()
    {
        return m_directed;
    }

    //! Sets if the graph is directed to \a directed.
    void directed(bool directed)
    {
        m_directed = directed;
    }

    //! Returns the template name of node \a v.
    const String & templateNode(node v) const
    {
        return m_nodeTemplate[v];
    }
    //! Returns the template name of node \a v.
    String & templateNode(node v)
    {
        return m_nodeTemplate[v];
    }

    //! Returns the x-coordinate of node \a v.
    const double & x(node v) const
    {
        return m_x[v];
    }
    //! Returns the x-coordinate of node \a v.
    double & x(node v)
    {
        return m_x[v];
    }

    //! Returns the y-coordinate of node \a v.
    const double & y(node v) const
    {
        return m_y[v];
    }
    //! Returns the y-coordinate of node \a v.
    double & y(node v)
    {
        return m_y[v];
    }

    //! Returns a reference to the NodeArray \a m_width.
    const NodeArray<double> & width() const
    {
        return m_width;
    }
    //! Returns a refeence to the NodeArray \a m_width.
    NodeArray<double> & width()
    {
        return m_width;
    }

    //! Returns the width of the bounding box of node \a v.
    const double & width(node v) const
    {
        return m_width[v];
    }
    //! Returns the width of the bounding box of node \a v.
    double & width(node v)
    {
        return m_width[v];
    }

    //! Returns a reference to the NodeArray \a m_height.
    const NodeArray<double> & height() const
    {
        return m_height;
    }
    //! Returns a refeence to the NodeArray \a m_height.
    NodeArray<double> & height()
    {
        return m_height;
    }

    //! Returns the height of the bounding box of node \a v.
    const double & height(node v) const
    {
        return m_height[v];
    }
    //! Returns the height of the bounding box of node \a v.
    double & height(node v)
    {
        return m_height[v];
    }

    //! Returns the level of node \a v.
    const int & level(node v) const
    {
        return m_level[v];
    }
    //! Returns the level of node \a v.
    int & level(node v)
    {
        return m_level[v];
    }

    //! Returns the weight of node \a v.
    const int & weight(node v) const
    {
        return m_nodeIntWeight[v];
    }
    //! Returns the weight of node \a v.
    int & weight(node v)
    {
        return m_nodeIntWeight[v];
    }

    //! Returns the brush pattern of node \a v.
    const BrushPattern & nodePattern(node v) const
    {
        return m_nodePattern[v];
    }
    //! Returns the brush pattern of node \a v.
    BrushPattern & nodePattern(node v)
    {
        return m_nodePattern[v];
    }

    //! Returns the line style of node \ v.
    const EdgeStyle & styleNode(node v) const
    {
        return m_nodeStyle[v];
    }
    //! Returns the line style of node \ v.
    EdgeStyle & styleNode(node v)
    {
        return m_nodeStyle[v];
    }

    //! Returns the line width of node \a v.
    const double & lineWidthNode(node v) const
    {
        return m_nodeLineWidth[v];
    }
    //! Returns the line width of node \a v.
    double & lineWidthNode(node v)
    {
        return m_nodeLineWidth[v];
    }

    //! Returns the line color of node \a v.
    const String & nodeLine(node v) const
    {
        return m_nodeLine[v];
    }
    //! Returns the line color of node \a v.
    String & nodeLine(node v)
    {
        return m_nodeLine[v];
    }

    //! Returns the list of bend points of edge \a e.
    const DPolyline & bends(edge e) const
    {
        return m_bends[e];
    }
    //! Returns the list of bend points of edge \a e.
    DPolyline & bends(edge e)
    {
        return m_bends[e];
    }

    //! Returns the (integer) weight of edge \a e.
    const int & intWeight(edge e) const
    {
        return m_intWeight[e];
    }
    //! Returns the (integer) weight of edge \a e.
    int & intWeight(edge e)
    {
        return m_intWeight[e];
    }

    //! Returns the (real number) weight of edge \a e.
    const double & doubleWeight(edge e) const
    {
        return m_doubleWeight[e];
    }
    //! Returns the (real number) weight of edge \a e.
    double & doubleWeight(edge e)
    {
        return m_doubleWeight[e];
    }

    //! Returns the line width of edge \a e.
    const double & edgeWidth(edge e) const
    {
        return m_edgeWidth[e];
    }
    //! Returns the line width of edge \a e.
    double & edgeWidth(edge e)
    {
        return m_edgeWidth[e];
    }

    //! Returns the color of node \a v.
    const String & colorNode(node v) const
    {
        return m_nodeColor[v];
    }
    //! Returns the color of node \a v.
    String & colorNode(node v)
    {
        return m_nodeColor[v];
    }

    //! Returns the shape type of node \a v.
    int shapeNode(node v) const
    {
        return m_nodeShape[v];
    }
    //! Returns the shape type of node \a v.
    int & shapeNode(node v)
    {
        return m_nodeShape[v];
    }

    //! Returns the label of node \ v.
    const String & labelNode(node v) const
    {
        return m_nodeLabel[v];
    }
    //! Returns the label of node \ v.
    String & labelNode(node v)
    {
        return m_nodeLabel[v];
    }

    //! Returns the label of edge \a e.
    const String & labelEdge(edge e) const
    {
        return m_edgeLabel[e];
    }
    //! Returns the label of edge \a e.
    String & labelEdge(edge e)
    {
        return m_edgeLabel[e];
    }

    //! Returns the type of edge \a e.
    Graph::EdgeType type(edge e) const
    {
        return m_eType.valid() ? m_eType[e] : Graph::association;
    }
    //! Returns the type of edge \a e.
    Graph::EdgeType & type(edge e)
    {
        return m_eType[e];
    }

    //! Returns the type of node \a v.
    Graph::NodeType type(node v) const
    {
        return m_vType.valid() ? m_vType[v] : Graph::vertex;
    }
    //! Returns the type of node \a v.
    Graph::NodeType & type(node v)
    {
        return m_vType[v];
    }

    //! Returns the user ID of node \a v.
    const int & idNode(node v) const
    {
        return m_nodeId[v];
    }
    //! Returns the user ID of node \a v.
    int & idNode(node v)
    {
        return m_nodeId[v];
    }

    //! Returns the arrow type of edge \a e.
    const EdgeArrow & arrowEdge(edge e) const
    {
        return m_edgeArrow[e];
    }
    //! Returns the arrow type of edge \a e.
    EdgeArrow & arrowEdge(edge e)
    {
        return m_edgeArrow[e];
    }

    //! Returns the line style of an edge \a e.
    const EdgeStyle & styleEdge(edge e) const
    {
        return m_edgeStyle[e];
    }
    //! Returns the line style of an edge \a e.
    EdgeStyle & styleEdge(edge e)
    {
        return m_edgeStyle[e];
    }

    //! Returns the color of node \a v.
    const String & colorEdge(edge e) const
    {
        return m_edgeColor[e];
    }
    //! Returns the color of node \a v.
    String & colorEdge(edge e)
    {
        return m_edgeColor[e];
    }

    // Images:
    //! Returns image uri of node v.
    const String & imageUriNode(node v) const
    {
        return m_imageUri[v];
    }
    //! Returns image uri of node v.
    String & imageUriNode(node v)
    {
        return m_imageUri[v];
    }
    //! Returns image style of node v.
    const ImageStyle & imageStyleNode(node v) const
    {
        return m_imageStyle[v];
    }
    //! Returns image style of node v.
    ImageStyle & imageStyleNode(node v)
    {
        return m_imageStyle[v];
    }
    // Returns image alignment of node v.
    const ImageAlignment & imageAlignmentNode(node v) const
    {
        return m_imageAlign[v];
    }
    // Returns image alignment of node v.
    ImageAlignment & imageAlignmentNode(node v)
    {
        return m_imageAlign[v];
    }
    //! Returns bool value drawLine of node v.
    const bool & imageDrawLineNode(node v) const
    {
        return m_imageDrawLine[v];
    }
    //! Returns bool value drawLine of node v.
    bool & imageDrawLineNode(node v)
    {
        return m_imageDrawLine[v];
    }
    //! Returns image width of node v.
    const double & imageWidthNode(node v) const
    {
        return m_imageWidth[v];
    }
    //! Returns image width of node v.
    double & imageWidthNode(node v)
    {
        return m_imageWidth[v];
    }
    // Returns image height of node v.
    const double & imageHeightNode(node v) const
    {
        return m_imageHeight[v];
    }
    // Returns image height of node v.
    double & imageHeightNode(node v)
    {
        return m_imageHeight[v];
    }

    //! Returns the edgesubgraph value of an edge \a e.
    const unsigned int & subGraphBits(edge e) const
    {
        return m_subGraph[e];
    }
    //! Returns the edgesubgraph value of an edge \a e.
    unsigned int & subGraphBits(edge e)
    {
        return m_subGraph[e];
    }

    //! Checks whether edge \a e belongs to basic graph \a n.
    bool inSubGraph(edge e, int n) const
    {
        OGDF_ASSERT(n >= 0 && n < 32);
        return (m_subGraph[e] & (1 << n)) != 0;
    }
    //! Addes edge \a e to basic graph \a n.
    void addSubGraph(edge e, int n)
    {
        OGDF_ASSERT(n >= 0 && n < 32);
        m_subGraph[e] |= (1 << n);
    }
    //! Removes edge \a e from basic graph \a n.
    void removeSubGraph(edge e, int n)
    {
        OGDF_ASSERT(n >= 0 && n < 32);
        m_subGraph[e] &= ~(1 << n);
    }

    //! Returns the bounding box of the graph.
    const DRect boundingBox() const;

    /**
     * We hide the internal representation of semantic node types from
     * the user to be able to change this later (semantic node type member array).
     * We are not allowed to set association classes manually, only by calling
     * createAssociationClass().
     */
    bool isAssociationClass(node v) const
    {
        return (type(v) == Graph::associationClass);
    }

    /**
     * According to the \a mode switch add either the node center points to
     * the bends or the anchor point on the node boundary
     *   - \a mode = 0: only add node center
     *   - \a mode = 1: compute intersection with the line segment to the center
     *     and the boundary of the rectangular node
     *   - \a mode = 2: compute intersection with the first/last line segment
     *     and the boundary of the rectangular node
     */
    void addNodeCenter2Bends(int mode = 1);

    void clearAllBends();

    //! Returns a list of all inheritance hierarchies in the graph.
    /**
     * Inheritance hierarchies are identified by edges with type Graph::generalization.
     *
     * @param list is a list of all hierarchies; each hierarchie is itself a list
     *        of all nodes in this hierarchy.
     *
     * \return Returns the number of generalization hierarchies.
     */
    int hierarchyList(List<List<node>*> & list) const;

    //! Returns a list of all inheritance hierarchies in the graph.
    /**
     * Inheritance hierarchies are identified by edges with type Graph::generalization.
     *
     * @param list is a list of all hierarchies; each hierarchie is itself a list
     *        of all edges in this hierarchy.
     *
     * \return Returns the number of generalization hierarchies.
     */
    int hierarchyList(List<List<edge>*> & list) const;

    //! Sets the width of all nodes to \a w.
    void setAllWidth(double w);
    //! Sets the height of all nodes to \a h.
    void setAllHeight(double h);

    //! Reads the graph from a GML file \a fileName.
    bool readGML(Graph & G, const String & fileName);

    //! Reads the graph from a GML input stream \a is.
    bool readGML(Graph & G, istream & is);

    //! Writes the graph with its attributes in GML format to file \a fileName.
    void writeGML(const String & fileName) const;

    //! Writes the graph with its attributes in GML format to output stream \a os.
    void writeGML(ostream & os) const;

    //! Writes the graph with its attributes in SVG format to file \a fileName.
    /**
     * @param fileName filename of the SVG
     * @param fontSize size of node label (default = 3)
     * @param fontColor color of node label (default = #000000)
     */
    void writeSVG(const String & fileName, int fontSize = 3, const String & fontColor = "#000000") const;

    //! Writes the graph with its attributes in SVG format to output stream \a os.
    /**
     * @param os output stream for SVG
     * @param fontSize size of node label
     * @param fontColor color of node label
     */
    void writeSVG(ostream & os, int fontSize, const String & fontColor) const;

    //! Reads the graph and attributes from the XML file \a fileName.
    bool readXML(Graph & G, const String & fileName);

    //! Reads the graph and attributes from the XML input stream \a is.
    bool readXML(Graph & G, istream & is);

    //! Writes the graph to the XML file \a fileName.
    void writeXML(const String & fileName, const char* delimiter = "", const char* offset = "") const;

    //! Writes the graph to XML output stream \a os.
    virtual void writeXML(ostream & os, const char* delimiter = "", const char* offset = "") const;

    //! Reads a graph in Rudy format from file \a fileName.
    bool readRudy(Graph & G, const String & fileName);

    //! Reads a graph in Rudy format from input stream \a is.
    bool readRudy(Graph & G, istream & is);

    //! Writes the graph in Rudy format to file \a fileName.
    void writeRudy(const String & fileName) const;

    //! Writes the graph in Rudy format to output stream \a os.
    void writeRudy(ostream & os) const;

    //! Removes unnecessary bend points in orthogonal segements.
    /**
     * Processes all edges and removes unnecessary bend points in the bend point list
     * of the edge, i.e., bend points such that the preceding and succeeding bend point
     * form a horizontal or vertical segement containing this bend point. This function
     * is useful to remove redundant bend points in an orthogonal layout.
     */
    void removeUnnecessaryBendsHV();
};

} // end namespace ogdf


#endif
