/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class IncNodeInserter.
 *
 * This class represents the base class for strategies
 * for the incremental drawing approach to insert nodes
 * (having no layout fixation) into the fixed part of
 * a PlanRep.
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


#ifndef OGDF_INCNODEINSERTER_H
#define OGDF_INCNODEINSERTER_H


#include <ogdf/planarity/PlanRepInc.h>
#include <ogdf/basic/UMLGraph.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/GraphObserver.h>

namespace ogdf
{


//===============================================
//main function(s):
//
// insertcopyNode insert a node into a face
//
//===============================================


class OGDF_EXPORT IncNodeInserter
{
public:
    //creates inserter on PG
    IncNodeInserter(PlanRepInc & PG) : m_planRep(&PG) { }

    //insert copy in m_planRep for original node v
    virtual void insertCopyNode(node v, CombinatorialEmbedding & E,
                                Graph::NodeType vTyp) = 0;

protected:
    //returns a face to insert a copy of v and a list of
    //adjacency entries corresponding to the insertion adjEntries
    //for the adjacent edges
    virtual face getInsertionFace(node v, CombinatorialEmbedding & E) = 0;

    PlanRepInc* m_planRep; //the PlanRep that is changed
}; //incnodeinserter

} //end namespace ogdf

#endif
