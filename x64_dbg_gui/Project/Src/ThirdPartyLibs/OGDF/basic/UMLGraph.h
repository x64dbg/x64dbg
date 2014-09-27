/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class UMLGraph.
 *
 * \author Carsten Gutwenger and Sebastian Leipert
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

#ifndef OGDF_UML_GRAPH_H
#define OGDF_UML_GRAPH_H


#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/AdjEntryArray.h>
#include <ogdf/basic/SList.h>


namespace ogdf
{

class OGDF_EXPORT UMLGraph : public GraphAttributes
{
public:
    // construction

    // dummy default constructor (place-holder)
    UMLGraph() : GraphAttributes(), m_pG(0) { }

    // creates UML graph in which each edge is an association
    explicit UMLGraph(Graph & G, long initAttributes = 0);

    // destruction
    virtual ~UMLGraph();

    //(re)initialization
    virtual void init(Graph & G, long initAttr)
    {
        m_pG = &G;
        GraphAttributes::init(G, initAttr);
        m_hierarchyParent.init(constGraph(), 0);
        m_upwardEdge.init(constGraph(), false);
    }

    operator const Graph & () const
    {
        return *m_pGraph;
    }

    //----------------------------------------------------------------------------
    //structural changes
    //merge generalizations at a common superclass
    void insertGenMergers();
    //insert mergers per node with given edges
    node doInsertMergers(node v, SList<edge> & inGens);
    void undoGenMergers();

    //replace (dense) subgraphs given in list clique by
    //inserting a center node connected to each node (=>star)
    //and deleting all edges between nodes in clique
    //returns center node
    void replaceByStar(List< List<node> > & cliques);

    //undo clique replacements
    void undoStars();
    //boolean switches restore of all hidden edges in single clique call
    void undoStar(node center, bool restoreAllEdges);

    //returns the size of a circular drawing for a clique around center v
    DRect cliqueRect(node v)
    {
        return m_cliqueCircleSize[v];
    }
    DPoint cliquePos(node v)
    {
        return m_cliqueCirclePos[v];
    }

    //compute circle positions for all nodes around center
    //using the ordering given in this UMLGraph, calls
    //ccP(List...)
    //rectMin is a temporary solution until compaction with constraints allows stretching
    //of rect to clique size, it gives the min(w,h) of the given fixed size rect around the clique
    void computeCliquePosition(node center, double rectMin);//, const adjEntry &startAdj);
    //compute positions for the nodes in adjNodes on a circle
    //tries to keep the relative placement of the nodes in the clique
    //rectangle (left, right,...) to avoid clique crossings of outgoing edges
    void computeCliquePosition(List<node> & adjNodes, node center, double rectMin = -1.0);

    //allow change, but should not be declared const
    Graph & pureGraph() const
    {
        return *m_pG;
    }

    //set status value
    //void setAlign(edge e, bool b) {m_alignEdge[e] = b;}
    //set status of edges to be specially embedded (if alignment)
    void setUpwards(adjEntry a, bool b)
    {
        m_upwardEdge[a] = b;
    }
    bool upwards(adjEntry a) const
    {
        return m_upwardEdge[a];
    }

    // writes attributed graph in GML format to file fileName
    void writeGML(const char* fileName);

    // writes attributed graph in GML format to output stream os
    void writeGML(ostream & os);

    //adjust the parent field for all nodes after insertion of
    //mergers. If insertion is done per node via doinsert, adjust
    //has to be called afterwards. Otherwise, insertgenmergers calls it.
    void adjustHierarchyParents();

    //use the node position and bend position information to
    //derive an ordering of the edges around each node
    //this does not need to result in a correct combinatorial embedding
    void sortEdgesFromLayout();

    //-------------------
    //status retrieval
    //returns true if edge was inserted during clique replacement
    //TODO: check here how to guarantee that value is defined,
    //edgearray is only valid if there are cliques replaced
    bool isReplacement(edge e)
    {
        return m_replacementEdge[e];
    }

    const SListPure<node> & centerNodes()
    {
        return m_centerNodes;
    }

    //default size of inserted clique replacement center nodes
    void setDefaultCliqueCenterSize(double i)
    {
        m_cliqueCenterSize = max(i, 1.0);
    }
    double getDefaultCliqueCenterSize()
    {
        return m_cliqueCenterSize;
    }

    //-------------------------------------------------------------------------
    //modelling of association classes
    class AssociationClass
    {
    public:
        AssociationClass(edge e, double width = 1.0, double height = 1.0,
                         double x = 0.0, double y = 0.0)
            : m_width(width), m_height(height), m_x(x), m_y(y), m_edge(e), m_node(0)
        { }

        double m_width;
        double m_height;
        double m_x;
        double m_y;
        edge   m_edge;
        node   m_node;
    };
    const SListPure<AssociationClass*> & assClassList() const
    {
        return m_assClassList;
    }

    const AssociationClass*  assClass(edge e) const
    {
        return m_assClass[e];
    }

    //adds association class to edge e
    //void createAssociationClass(edge e, double width = 1.0, double height = 1.0)
    node createAssociationClass(edge e, double width = 1.0, double height = 1.0)
    {
        AssociationClass* ac = new AssociationClass(e, width, height);
        m_assClass[e] = ac;
        m_assClassList.pushBack(ac);
        //we already insert the node here, but not the edge
        //when we always insert this node here, we can remove the associationclass
        //class and information later on
        node v = m_pG->newNode();
        m_height[v] = ac->m_height;
        m_width[v]  = ac->m_width;
        m_associationClassModel[ac->m_edge] = v;
        ac->m_node = v;
        //guarantee correct angle at edge to edge connection
        if(m_attributes & GraphAttributes::nodeType)
        {
            m_vType[v] = Graph::associationClass;
        }
        return v;

    }
    //this modelling should only take place in the preprocessing steps
    //of the drawing algorithms?
    //insert representation for association class in underlying graph
    void modelAssociationClasses()
    {
        SListIterator<UMLGraph::AssociationClass*> it = m_assClassList.begin();
        while(it.valid())
        {
            modelAssociationClass((*it));
            it++;
        }//while
    }
    node modelAssociationClass(AssociationClass* ac)
    {
        node dummy = m_pG->split(ac->m_edge)->source();

        m_height[dummy] = 1; //just a dummy size
        m_width[dummy]  = 1;
        OGDF_ASSERT(ac->m_node)
        m_pG->newEdge(ac->m_node, dummy);

        return dummy;
    }

    void undoAssociationClasses()
    {
        SListIterator<UMLGraph::AssociationClass*> it = m_assClassList.begin();
        while(it.valid())
        {
            undoAssociationClass((*it));
            it++;
        }//while
    }
    //remove the modeling of the association class without removing the information
    void undoAssociationClass(AssociationClass* ac)
    {
        node v = m_associationClassModel[ac->m_edge];
        OGDF_ASSERT(v)
        OGDF_ASSERT(v->degree() == 1)
        if(v->degree() != 1) throw AlgorithmFailureException(afcLabel);
        //save layout information
        ac->m_x = x(v);
        ac->m_y = y(v);

        //remove node and unsplit edge

        //run around the dummy node connected to v
        adjEntry outAdj = v->firstAdj();
        adjEntry dummyAdj = outAdj->twin();

        node dummy = dummyAdj->theNode();
        OGDF_ASSERT(dummy->degree() == 3)

        //we do not delete the node if we already inserted it in create...
        //because it is a part of the graph now (in contrast to the split node)
        m_pG->delEdge(v->firstAdj()->theEdge());
        OGDF_ASSERT(v->degree() == 0)

        m_pG->unsplit(dummy);
    }//undoAssociationClass



protected:

    node replaceByStar(List<node> & clique, NodeArray<int> & cliqueNum);
    DRect circularBound(node center);

private:

    Graph* m_pG;

    //information about edges that are deleted in clique processing
    class CliqueInfo
    {
    public:
        CliqueInfo(node v, int i) : m_target(v), m_edgeIndex(i) {}
        node m_target;    //target node of deleted edge
        int  m_edgeIndex; //index of deleted edge, has to be restored
    };
    double m_cliqueCenterSize; //default size of inserted clique replacement center nodes

    SListPure<edge> m_mergeEdges;
    SListPure<node> m_centerNodes; //center nodes introduced at clique replacement
    EdgeArray<bool> m_replacementEdge;  //used to mark clique replacement edges
    //may be we can join this with edge type
    NodeArray<DRect> m_cliqueCircleSize;    //save the bounding box size of the
    //circular drawing of the clique at center
    NodeArray<DPoint> m_cliqueCirclePos;    //save the position of the node in the
    //circular drawing of the clique
    //---------------------------------------------------
    //structures for association classes
    //may be replaced later by generic structures for different types
    SListPure<AssociationClass*> m_assClassList; //saves all accociation classes
    EdgeArray<AssociationClass*> m_assClass;     //association class for list
    EdgeArray<node> m_associationClassModel;     //modelled classes are stored


    //***************************************************
    //the following arrays are only set and updated in insertgenmergers
    //used to classify edges for embedding with alignment
    AdjEntryArray<bool> m_upwardEdge;

    //used to derive edge types for alignment in PlanRepUML
    //(same hierarchyparent => edge connects (half)brothers
    //only set during insertgenmergers to avoid the extra computation
    NodeArray<node> m_hierarchyParent;

};


} // end namespace ogdf

#endif
