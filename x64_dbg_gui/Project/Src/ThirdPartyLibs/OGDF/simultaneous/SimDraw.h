/*
 * $Revision: 2528 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-03 23:05:08 +0200 (Tue, 03 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Base class for simultaneous drawing.
 *
 * \author Michael Schulz and Daniel Lueckerath
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


#ifndef OGDF_SIM_DRAW_H
#define OGDF_SIM_DRAW_H

#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/GraphCopy.h>

namespace ogdf
{

//! The Base class for simultaneous graph drawing.
/**
* This class provides functions for simultaneous graph drawing,
* such as adding new subgraphs.
*
* It is possible to store up to 32 basicgraphs in one instance of the class.
* The basic graph membership for all edges is stored via
* GraphAttributes::edgeSubgraph.
* Several functions are outsourced in corresponding manipulator modules.
*/

class OGDF_EXPORT SimDraw
{
    friend class SimDrawManipulatorModule;
    friend class SimDrawCaller;
    friend class SimDrawColorizer;
    friend class SimDrawCreator;
    friend class SimDrawCreatorSimple;

public:
    //! Types for node comparison
    enum CompareBy
    {
        index,                  //!< nodes are compared by their indices
        label                   //!< nodes are compared by their labels
    };

private:
    Graph m_G;                  //!< the underlying graph
    GraphAttributes m_GA;       //!< the underlying graphattributes
    CompareBy m_compareBy;      //!< compare mode
    NodeArray<bool> m_isDummy;  //!< dummy nodes may be colored differently


public:
    //! constructs empty simdraw instance
    /**
    * GraphAttributes::edgeSubGraph is activated.
    * No other attributes are active.
    */
    SimDraw();


    //! returns graph
    const Graph & constGraph() const
    {
        return m_G;
    }
    //! returns graph
    Graph & constGraph()
    {
        return m_G;
    }
    //! returns graphattributes
    const GraphAttributes & constGraphAttributes() const
    {
        return m_GA;
    }
    //! returns graphattributes
    GraphAttributes & constGraphAttributes()
    {
        return m_GA;
    }

    //! empty graph
    void clear()
    {
        m_G.clear();
    }

    //! returns compare mode
    const CompareBy & compareBy() const
    {
        return m_compareBy;
    }
    //! returns compare mode
    /*
    * The usage of comparison by label makes only sense if the
    * attribute nodeLabel is activated and labels are set properly.
    */
    CompareBy & compareBy()
    {
        return m_compareBy;
    }

    //! returns true if node \a v is marked as dummy
    /**
    * All dummy node features are introduced for usage when running
    * callSubgraphPlanarizer of SimDrawCaller.
    */
    const bool & isDummy(node v) const
    {
        return m_isDummy[v];
    }
    //! returns true if node \a v is marked as dummy
    bool & isDummy(node v)
    {
        return m_isDummy[v];
    }
    //! returns true if node \a v is a cost zero dummy node
    bool isPhantomDummy(node v) const
    {
        return((isDummy(v)) && (!isProperDummy(v)));
    }
    //! returns true if node \a v is a cost greater zero dummy node
    bool isProperDummy(node v) const;

    //! returns number of nodes
    int numberOfNodes() const
    {
        return m_G.numberOfNodes();
    }
    //! returns number of dummy nodes
    int numberOfDummyNodes() const;
    //! returns number of phantom dummy nodes
    int numberOfPhantomDummyNodes() const;
    //! returns number of proper dummy nodes
    int numberOfProperDummyNodes() const;

    //! checks whether instance is a consistent SimDraw instance
    bool consistencyCheck() const;

    //! calculates maximum number of input graphs
    /**
    * Subgraphs are numbered from 0 to 31.
    * This method returns the number of the maximal used subgraph.
    * If the graph is empty, the function returns -1.
    */
    int maxSubGraph() const;

    //! returns number of BasicGraphs in m_G
    /**
    * This function uses maxSubGraph to return the number of
    * basic graphs contained in m_G.
    * If the graph is empty, the function returns 0.
    */
    int numberOfBasicGraphs() const;

    //! calls GraphAttributes::readGML
    void readGML(const char* fileName)
    {
        m_GA.readGML(m_G, fileName);
    }
    //! calls GraphAttributes::writeGML
    void writeGML(const char* fileName) const
    {
        m_GA.writeGML(fileName);
    }

    //! returns graph consisting of all edges and nodes from SubGraph \a i
    const Graph getBasicGraph(int i) const;
    //! returns graphattributes associated with basic graph \a i
    /**
    * Supported attributes are:
    * nodeGraphics, edgeGraphics, edgeLabel, nodeLabel, nodeId,
    * edgeIntWeight and edgeColor.
    */
    void getBasicGraphAttributes(int i, GraphAttributes & GA, Graph & G);

    //! adds new GraphAttributes to m_G
    /**
    * If the number of subgraphs in m_G is less than 32, this
    * function will add the new GraphAttributes \a GA to m_G
    * and return true.
    * Otherwise this function returns false.
    * The function uses the current compare mode.
    */
    bool addGraphAttributes(const GraphAttributes & GA);

    //! adds the graph g to the instance m_G
    /**
    * If the number of subgraphs in m_G is less than 32 and
    * m_compareBy is set to index, this function will add graph
    * \a G to m_G and return true.
    * Otherwise this function returns false.
    */
    bool addGraph(const Graph & G);

    //! gives access to new attribute if not already given
    void addAttribute(long attr)
    {
        if(!(m_GA.attributes() & attr))
            m_GA.initAttributes(attr);
    }

private:
    //! compares two nodes \a v and \a w by their ids
    bool compareById(node v, node w) const
    {
        return (v->index() == w->index());
    }

    //! compares two nodes \a v and \a w by their labels
    /**
    * This method only works, if attribute nodeLabel is activated
    * and set properly.
    * Otherwise it is recommended to use compareById.
    */
    bool compareByLabel(const GraphAttributes & vGA, node v,
                        const GraphAttributes & wGA, node w) const
    {
        return(vGA.labelNode(v) == wGA.labelNode(w));
    }

    //! compares two nodes \a v and \a w by compare mode stored in m_compareBy
    /**
    * This method checks whether m_compareBy was set to index or label and
    * uses the corresponding compare method.
    */
    bool compare(const GraphAttributes & vGA, node v,
                 const GraphAttributes & wGA, node w) const;

};

}

#endif
