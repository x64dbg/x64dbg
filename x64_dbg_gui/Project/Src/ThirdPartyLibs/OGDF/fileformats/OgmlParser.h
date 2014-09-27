/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of auxiliary classes OgmlAttributeValue,
 *        OgmlAttribute and OgmlTag.
 *
 * \author Christian Wolf and Bernd Zey
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

//KK: Commented out the constraint stuff using //o
//CG: compound graph stuff has been removed with commit 2465

#ifndef OGDF_OGML_PARSER_H
#define OGDF_OGML_PARSER_H

#include <ogdf/fileformats/Ogml.h>
#include <ogdf/fileformats/DinoXmlParser.h>
#include <ogdf/basic/Hashing.h>
#include <ogdf/basic/String.h>
#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/cluster/ClusterGraphAttributes.h>

// constraints
//o#include <ogdf/Constraints.h>


namespace ogdf
{

//
// ---------- O g m l P a r s e r ------------------------
//

/**Objects of this class represent a validating parser for files in Ogml.
*/
class OgmlParser
{
private:

    // struct definitions for mapping of templates
    struct OgmlNodeTemplate;
    struct OgmlEdgeTemplate;
    //struct OgmlLabelTemplate;

    struct OgmlSegment;

    class OgmlAttributeValue;
    class OgmlAttribute;
    class OgmlTag;

    friend ostream & operator<<(ostream & os, const OgmlParser::OgmlAttribute & oa);
    friend ostream & operator<<(ostream & os, const OgmlParser::OgmlTag & ot);

    static Hashing<int, OgmlTag>*            s_tags;       //!< Hashtable for saving all ogml tags.
    static Hashing<int, OgmlAttribute>*      s_attributes; //!< Hashtable for saving all ogml attributes.
    static Hashing<int, OgmlAttributeValue>* s_attValues;  //!< Hashtable for saving all values of ogml attributes.


    //! Builds hashtables for tags and attributes.
    static void buildHashTables();

    mutable Ogml::GraphType m_graphType; //!< Saves a graph type. Is set by checkGraphType.

    Hashing<String, const XmlTagObject*> m_ids; //!< Saves all ids of an ogml-file.

    /**
     * Checks if all tags (XmlTagObject), their attributes (XmlAttributeObject) and
     * their values are valid (are tags expected, do they own the rigth attributes...)
     * and sets a valid flag to these. Furthermore it checks if ids of tags are
     * unique and if id references are valid.
     * See OgmlTag.h for semantics of the encodings.
     * Returns the validity state of the current processed tag.
     */
    int validate(const XmlTagObject* xmlTag, int ogmlTag);

    /**
     * Wrapper method for validate method above.
     * Returns true when validation is successfull, false otherwise.
     */
    //bool validate(const char* fileName);

    //! Prints some useful information about un-/successful validation.
    void printValidityInfo(const OgmlTag & ot,
                           const XmlTagObject & xto,
                           int valStatus,
                           int line);

    /**
     * Finds the OGML-tag in the parse tree with the specified id,
     * stores the tag in xmlTag
     * recTag is the tag for recursive calls
     * returns false if something goes wrong
     */
    //bool getXmlTagObjectById(XmlTagObject *recTag, String id, XmlTagObject *&xmlTag);

    /**
     * Checks the graph type and stores it in the member variable m_graphType
     * xmlTag has to be the root or the graph or the structure Ogml-tag
     * returns false if something goes wrong
     */
    bool checkGraphType(const XmlTagObject* xmlTag) const;

    //! Returns true iff subgraph is an hierarchical graph.
    bool isGraphHierarchical(const XmlTagObject* xmlTag) const;

    //! Returns true iff node contains other nodes.
    bool isNodeHierarchical(const XmlTagObject* xmlTag) const;

    Ogml::GraphType getGraphType()
    {
        return m_graphType;
    };


    // id hash tables
    // required variables for building
    // hash table with id from file and node
    Hashing<String, node> m_nodes;
    Hashing<String, edge> m_edges;
    Hashing<String, cluster> m_clusters;
    // hash table for bend-points
    Hashing<String, DPoint> m_points;

    // hash table for checking uniqueness of ids
    // (key:) int = id in the created graph
    // (info:) String = id in the ogml file
    Hashing<int, String> m_nodeIds;
    Hashing<int, String> m_edgeIds;
    Hashing<int, String> m_clusterIds;

    // build methods

    //! Builds a graph; ignores nodes which have hierarchical structure.
    bool buildGraph(Graph & G);

    //! Builds a cluster graph.
    bool buildCluster(
        const XmlTagObject* rootTag,
        Graph & G,
        ClusterGraph & CG);

    //! Recursive part of buildCluster.
    bool buildClusterRecursive(
        const XmlTagObject* xmlTag,
        cluster parent,
        Graph & G,
        ClusterGraph & CG);

    //! Build a cluster graph with style/layout attributes.
    bool buildAttributedClusterGraph(
        Graph & G,
        ClusterGraphAttributes & CGA,
        const XmlTagObject* root);

    //! Method for setting labels of clusters.
    bool setLabelsRecursive(
        Graph & G,
        ClusterGraphAttributes & CGA,
        XmlTagObject* root);

    // helping pointer for constraints-loading
    // this pointer is set in the building methods
    // so we don't have to traverse the tree in buildConstraints
    XmlTagObject* m_constraintsTag;

    // hashing lists for templates
    //  string = id
    Hashing<String, OgmlNodeTemplate*> m_ogmlNodeTemplates;
    Hashing<String, OgmlEdgeTemplate*> m_ogmlEdgeTemplates;
    //Hashing<String, OgmlLabelTemplate> m_ogmlLabelTemplates;

    // auxiliary methods for mapping graph attributes

    //! Returns int value for the pattern.
    int getBrushPatternAsInt(String s);

    //! Returns the shape as an integer value.
    int getShapeAsInt(String s);

    //! Maps the OGML attribute values to corresponding GDE values.
    String getNodeTemplateFromOgmlValue(String s);

    //! Returns the line type as an integer value.
    int getLineTypeAsInt(String s);

    //! Returns the image style as an integer value.
    int getImageStyleAsInt(String s);

    //! Returns the alignment of image as an integer value.
    int getImageAlignmentAsInt(String s);

    // arrow style, actually a "boolean" function
    // because it returns only 0 or 1 according to GDE
    // sot <=> source or target
    int getArrowStyleAsInt(String s, String sot);

    // the matching method to getArrowStyleAsInt
    GraphAttributes::EdgeArrow getArrowStyle(int i);

    // function that operates on a string
    // the input string contains "&lt;" instead of "<"
    //  and "&gt;" instead of ">"
    //  to disable interpreting the string as xml-tags (by DinoXmlParser)
    // so this function substitutes  "<" for "&lt;"
    String getLabelCaptionFromString(String str);

    //! Returns the integer value of the id at the end of the string (if it exists).
    bool getIdFromString(String str, int & id);

    //! Validiation method.
    void validate(const char* fileName);

public:

    //! Constructs an OGML parser.
    OgmlParser() { }

    ~OgmlParser() { }


    //! Reads a cluster graph \a CG from file \a fileName in OGML format.
    /**
     * @param fileName is the name of the file to be parsed as OGML file.
     * @param G is the graph to be build from the OGML file; must be the graph associated with \a CG.
     * @param CG is the cluster graph to be build from the OGML file.
     * @return true if succesfull, false otherwise.
     */
    bool read(
        const char* fileName,
        Graph & G,
        ClusterGraph & CG);

    //! Reads a cluster graph \a CG with attributes \a CGA from file \a fileName in OGML format.
    /**
     * @param fileName is the name of the file to be parsed as OGML file.
     * @param G is the graph to be build from the OGML file; must be the graph associated with \a CG.
     * @param CG is the cluster graph to be build from the OGML file.
     * @param CGA are the cluster graph attributes (associated with CG) in which layout and style information are stored.
     * @return true if succesfull, false otherwise.
     */
    bool read(
        const char* fileName,
        Graph & G,
        ClusterGraph & CG,
        ClusterGraphAttributes & CGA);

};//end class OGMLParser

}//end namespace ogdf

#endif

