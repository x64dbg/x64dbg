/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Contains the class DinoUmlToGraphConverter...
 *
 * ...which performs all necessary steps to obtain a model graph
 * and a set of diagram graphs from the input file.
 *
 * \author Dino Ahr
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

#ifndef OGDF_DINO_UML_TO_GRAPH_CONVERTER_H
#define OGDF_DINO_UML_TO_GRAPH_CONVERTER_H

#include <ogdf/fileformats/DinoXmlParser.h>
#include <ogdf/fileformats/DinoUmlModelGraph.h>
#include <ogdf/fileformats/DinoUmlDiagramGraph.h>
#include <ogdf/basic/UMLGraph.h>

namespace ogdf
{

//---------------------------------------------------------
// D i n o U m l T o G r a p h C o n v e r t e r
//---------------------------------------------------------
/** This class performs all necessary steps to obtain a model
 *  graph and diagram graphs from the input file which contains
 *  an UML model in XML format.
 *  In particular the following is done:
 *  - Given the input file a class DinoXmlParser is created which
 *    accomplishes the parsing of the XML file; as result of the
 *    parsing procedure a parse tree representing the xml document
 *    is accessible.
 *  - The parse tree is used to extract the model graph of the contained
 *    uml model as well as a set of diagram graphs which are part of the
 *    uml model.
 *
 *  The graphs can be accessed via the functions getModelGraph() and
 *  getDiagramGraphs().
 *
 *  In debug mode warnings and the content of the model graph and the diagram
 *  graphs are written into the log file named \e umlToGraphConversionLog.txt.
 */
class OGDF_EXPORT DinoUmlToGraphConverter
{

private:

    /** The parser used for parsing the input file. */
    DinoXmlParser* m_xmlParser;

    /** The graph which represents the complete UML model. */
    DinoUmlModelGraph* m_modelGraph;

    /** The set of graphs which represent special diagrams
     *  contained in the model.
     */
    SList<DinoUmlDiagramGraph*> m_diagramGraphs;

    /** This list contains the set of graphs of the list #m_diagramGraphs
     *  in UMLGraph format. The transformation is performed in the constructor
     *  #DinoUmlToGraphConverter().
     */
    SList<UMLGraph*> m_diagramGraphsInUMLGraphFormat;

    /** Predefined info indices for known tag and attribute names. */
    enum PredefinedInfoIndex
    {
        xmi = 0,
        xmiContent,
        xmiId,
        umlModel,
        umlNamespaceOwnedElement,
        umlClass,
        name,
        umlGeneralization,
        child,
        parent,
        umlAssociation,
        umlAssociationConnection,
        umlAssociationEnd,
        type,
        umlDiagram,
        rootUmlDiagramElement,
        umlDiagramElement,
        geometry,
        subject,
        umlPackage,
        umlInterface,
        umlDependency,
        client,
        supplier,
        diagramType,
        classDiagram,
        moduleDiagram,

        nextPredefinedInfoIndex
    };

    /** Maps string info to node.
     *  We need this hash table for fast access to nodes corresponding
     *  to UML elements. For each UML Element in XMI format we have an
     *  unique identifier via the xmi.id attribute. The xmi.id attribute
     *  is saved as string in the hash table of the parser, hence we can
     *  use the info index of it. While scanning for relations between nodes
     *  we encounter the xmi.id attribute values of the involved elements referenced
     *  as type attribute in the relation. Now we can use the hash table to
     *  access the corresponding node.
     */
    Hashing<int, NodeElement*> m_idToNode;

    /** Maps string info to edge.
     *  The functionality is the same as for #m_idToNode.
     */
    Hashing<int, EdgeElement*> m_idToEdge;

    /** The log file.
     *  The log file is named \e umlToGraphConversionLog.txt and contains
     *  warnings and errors ocurred during the conversion process. Furthermore
     *  it contains the content of the model graph #m_modelGraph and of the
     *  diagram graphs #m_diagramGraphs.
     */
    ofstream* m_logFile;

public:

    /** Constructor.
     *  The constructor performs the following:
     *  - A parser object of class DinoXmlParser is created and
     *    the parse process is started via DinoXmlParser::createParseTree().
     *  - The variable #m_modelGraph is initialized with an empty object
     *    of type DinoUmlModelGraph. Then the model graph is build up with
     *    createModelGraph().
     *  - The list of diagram graphs #m_diagramGraphs is given to
     *    createDiagramGraphs() to build them up.
     *
     *  @param fileName The file name of the xml file which contains the data
     *                  of the uml model to be converted into the graph format.
     */
    DinoUmlToGraphConverter(const char* fileName);

    /** Destructor.
     *  The destructor destroys:
     *  - the diagram graphs contained in #m_diagramGraphs,
     *  - the model graph contained in #m_modelGraph,
     *  - the parser contained in #m_xmlParser.
     */
    ~DinoUmlToGraphConverter();

    /** Access to model graph.
     *  @return A const reference to the model graph.
     */
    const DinoUmlModelGraph & getModelGraph() const
    {
        return *m_modelGraph;
    }

    /** Access to diagram graphs.
     *  @return A const reference to the list of diagram graphs.
     */
    const SList<DinoUmlDiagramGraph*> & getDiagramGraphs() const
    {
        return m_diagramGraphs;
    }

    /** Access to the diagrams graphs in UMLGraph format.
     *  @return A const reference to a list of diagram graphs in UMLGraph format.
     */
    const SList<UMLGraph*> & getDiagramGraphsInUMLGraphFormat() const
    {
        return m_diagramGraphsInUMLGraphFormat;
    }

    /** Prints the content of each diagram to \a os.
     *  @param os The output stream where to direct the output to.
     */
    void printDiagramsInUMLGraphFormat(ofstream & os);

    /** Print hash table which maps the ids to the NodeElements.
     *  @param os The output stream where to direct the output to.
     */
    void printIdToNodeMappingTable(ofstream & os);

private:

    /** Inserts known strings for tags and attributes into the hashtable
     *  of the parser. The info elements for the hashtable are taken from
     *  enum #PredefinedInfoIndex.
     */
    void initializePredefinedInfoIndices();

    /** Converts the relevant information contained in the parse tree
     *  into the data structure of DinoUmlModelGraph. Error messages are
     *  reported in #m_logFile.
     *  @param modelGraph The model graph into which the nodes and edges
     *  corresponding to uml elements and uml relations should be inserted.
     *  @return Returns true if conversion was succesful, false otherwise.
     */
    bool createModelGraph(DinoUmlModelGraph & modelGraph);

    /** Traverses the package structure and identifies classifiers inside
     *  the parse tree (starting at \a currentRootTag) and inserts a new node
     *  for each classifier. This function will call itself recursively while
     *  traversing nested packages.
     *
     *  Valid classifiers are currently: \c class and \c interface.
     *  @param currentRootTag The tag where to start the search for classifiers.
     *  @param currentPackageName This string should contain the name of the package
     *  path corresponding to \a currentRootTag.
     *  @param modelGraph The model graph into which nodes are inserted.
     *  @return False if something went wrong, true otherwise.
     */
    bool traversePackagesAndInsertClassifierNodes(
        const XmlTagObject & currentRootTag,
        String currentPackageName,
        DinoUmlModelGraph & modelGraph);

    /** Tries to find all classifiers of type \a desiredClassifier inside the parse
     *  tree (starting at \a currentRootTag). Inserts a new node into \a modelGraph
     *  for each classifier found.
     *  @param currentRootTag The tag where to start the search for the desired
     *  classifier.
     *  @param currentPackageName This string should contain the name of the package
     *  path corresponding to \a currentRootTag.
     *  @param desiredClassifier The info index of the desired class
     *  (see enum #PredefinedInfoIndex).
     *  @param modelGraph The model graph into which nodes are inserted.
     *  @return False if something went wrong, true otherwise.
     */
    bool insertSpecificClassifierNodes(
        const XmlTagObject & currentRootTag,
        String currentPackageName,
        int desiredClassifier,
        DinoUmlModelGraph & modelGraph);

    /** Traverses the package structure and identifies associations inside
     *  the parse tree and inserts a new edge between the corresponding
     *  nodes of the involved classifiers.
     *
     *  Note that it is not possible to include this function into
     *  traversePackagesAndInsertClassifierNodes(). The reason is that it
     *  is possible that edges are specified prior to that one or both nodes
     *  have been created.
     *  @param currentRootTag The tag where to start the search for associations.
     *  @param modelGraph The model graph into which edges are inserted.
     *  @return False if something went wrong, true otherwise.
     */
    bool traversePackagesAndInsertAssociationEdges(
        const XmlTagObject & currentRootTag,
        DinoUmlModelGraph & modelGraph);

    /** Traverses the package structure and identifies generalization inside
     *  the parse tree and inserts a new edge between the corresponding
     *  nodes of the involved classifiers.
     *
     *  It does not make sense to put this function and
     *  traversePackagesAndInsertAssociationEdges() together since the generalization
     *  tags are inside the class tags, so first the classes have to be identified
     *  again in contrast to traversePackagesAndInsertAssociationEdges().
     *  @param currentRootTag The tag where to start the search for generalizations.
     *  @param modelGraph The model graph into which edges are inserted.
     *  @return False if something went wrong, true otherwise.
     */
    bool traversePackagesAndInsertGeneralizationEdges(
        const XmlTagObject & currentRootTag,
        DinoUmlModelGraph & modelGraph);


    /** Identifies dependency tags inside the parse tree and inserts a
     *  new edge between the corresponding nodes of the involved elements.
     *  @param currentRootTag The tag where to start the search for dependencies.
     *  @param modelGraph The model graph into which edges are inserted.
     *  @return False if something went wrong, true otherwise.
     */
    bool insertDependencyEdges(
        const XmlTagObject & currentRootTag,
        DinoUmlModelGraph & modelGraph);

    /** For each diagram converts the relevant information contained in the parse tree
     *  into the data structure of DinoUmlDiagramGraph. Error messages are
     *  reported in #m_logFile.
     *  @return Returns true if conversion was succesful, false otherwise.
     *  \todo Currently only class diagrams are handled. Must be extended to handle
     *  other kinds of uml diagrams.
     */
    bool createDiagramGraphs();

    /** Transforms each diagram graph contained in #m_diagramGraphs into an equivalent
     *  Error messages are reported in #m_logFile.
     *  @param diagramGraphsInUMLGraphFormat The list of diagram graphs in UMLGraph
     *  format which have been obtained from the diagram graphs.
     *  @return Returns true if conversion was successful, false otherwise.
     */
    bool createDiagramGraphsInUMLGraphFormat(SList<UMLGraph*> & diagramGraphsInUMLGraphFormat);


}; // class DinoUmlToGraphConverter


} // end namespace ogdf

#endif
