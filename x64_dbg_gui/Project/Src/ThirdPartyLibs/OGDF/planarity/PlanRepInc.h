/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class PlanRepInc.
 *
 * The PlanRepInc is especially suited for incremental drawing
 * modes. It derives from GraphObserver and therefore is always
 * informed about changes of the underlying graph. Keeps the
 * m_nodesInCC and m_numCC fields up-to-date.
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


#ifndef OGDF_PLAN_REP_INC_H
#define OGDF_PLAN_REP_INC_H



#include <ogdf/planarity/PlanRep.h>
#include <ogdf/planarity/PlanRepUML.h>
#include <ogdf/basic/UMLGraph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/GraphObserver.h>
#include <ogdf/basic/Array2D.h>


namespace ogdf
{


//===============================================
//main function(s):
//
//      this class is only an adaption of PlanRep
//      for the special incremental drawing case
//      As incremental layout only makes sense with
//      a given layout, this PlanRepInc copes with
//      layout information and embedding
//===============================================

class OGDF_EXPORT PlanRepInc : public PlanRepUML, public GraphObserver
{
public:
    //construction
    //constructor for interactive updates (parts added step by step)
    PlanRepInc(const UMLGraph & UG);
    //constructor for incremental updates (whole graph already given)
    //part to stay fixed has fixed value set to true
    PlanRepInc(const UMLGraph & UG, const NodeArray<bool> & fixed);

    //init a CC only with active elements
    void initActiveCC(int i);
    //but with at least one active node, makes a node active if necessary
    //and returns it. returns 0 otherwise
    node initMinActiveCC(int i);

    //in the case that the underlying incremental structure
    //changes, we update this copy
    virtual void nodeDeleted(node v);
    virtual void nodeAdded(node v);
    virtual void edgeDeleted(edge e);
    virtual void edgeAdded(edge e);
    virtual void reInit();
    virtual void cleared();//Graph cleared

    //sets activity status to true and updates the structures
    //node activation activates all adjacent edges
    void activateNode(node v);
    //TODO: auch deaktivieren
    //void activateNode(node v, bool b);
    void activateEdge(edge e);

    //handles copies of original CCs that are split into
    //unconnected parts of active nodes by connecting them
    //tree-like adding necessary edges at "external" nodes
    //of the partial CCs. Note that this only makes sense
    //when the CC parts are already correctly embedded
    bool makeTreeConnected(adjEntry adjExternal);
    //delete an edge again
    void deleteTreeConnection(int i, int j);
    void deleteTreeConnection(int i, int j, CombinatorialEmbedding & E);
    //sets a list of adjentries on "external" faces of
    //unconnected active parts of the current CC
    void getExtAdjs(List<adjEntry> & extAdjs);
    adjEntry getExtAdj(GraphCopy & GC, CombinatorialEmbedding & E);

    //component number
    int & componentNumber(node v)
    {
        return m_component[v];
    }

    bool & treeEdge(edge e)
    {
        return m_treeEdge[e];
    }
    //only valid if m_eTreeArray initialized, should be replaced later
    edge treeEdge(int i, int j) const
    {
        if(m_treeInit)
        {
            return m_eTreeArray(i, j);
        }
        return 0;
    }
    bool treeInit()
    {
        return m_treeInit;
    }

    //
    // extension of methods defined by GraphCopy/PlanRep
    //

    // splits edge e, can be removed when edge status in edgetype
    // m_treedge can be removed afterwards
    virtual edge split(edge e)
    {

        edge eNew = PlanRepUML::split(e);
        if(m_treeEdge[e]) m_treeEdge[eNew] = true;

        return eNew;

    }//split

    //debug output
#ifdef OGDF_DEBUG
    void writeGML(const char* fileName)
    {
        const GraphAttributes & AG = getUMLGraph();
        ofstream os(fileName);
        PlanRepInc::writeGML(os, AG);//getUMLGraph());//l);
    }
    void writeGML(const char* fileName, const Layout & drawing)
    {
        ofstream os(fileName);
        writeGML(os, drawing);
    }

    void writeGML(ostream & os, const GraphAttributes & AG);
    void writeGML(ostream & os, const Layout & drawing, bool colorEmbed = true);
    void writeGML(const char* fileName, GraphAttributes & AG, bool colorEmbed = true);

    //outputs a drawing if genus != 0
    int genusLayout(Layout & drawing) const;
#endif

protected:
    void initMembers(const UMLGraph & UG);
    //initialize CC with active nodes (minNode ? at least one node)
    node initActiveCCGen(int i, bool minNode);

private:
    NodeArray<bool> m_activeNodes; //stores the status of the nodes
    EdgeArray<bool> m_treeEdge; //edge inserted for connnectivity
    NodeArray<int> m_component; //number of partial component in current CC
    //used for treeConnection
    Array2D<edge> m_eTreeArray;    //used for treeConnection
    bool m_treeInit;           //check if the tree edge Array2D was initialized
};//PlanrepInc


}//namespace ogdf

#endif
