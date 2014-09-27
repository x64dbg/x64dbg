/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of functions for drawing module precondition
 *        handling.
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


#ifndef OGDF_PRECONDITION_H
#define OGDF_PRECONDITION_H


#include <ogdf/orthogonal/EdgeRouter.h>


namespace ogdf
{

//descent the hierarchy tree at "sink" v recursively
bool dfsGenTreeRec(
    UMLGraph & UG,
    EdgeArray<bool> & used,
    NodeArray<int> & hierNumber, //number of hierarchy tree
    // A node is visited if its hierNumber != 0
    int hierNum,
    node v,
    List<edge> & fakedGens, //temporary
    bool fakeTree)
{
    OGDF_ASSERT(hierNumber[v] == 0);
    hierNumber[v] = hierNum;

    bool returnValue = true;

    edge e;
    forall_adj_edges(e, v)
    {
        if(e->source() == v) continue;
        if(!(UG.type(e) == Graph::generalization)) continue;
        if(used[e]) continue;  //error ??
        used[e] = true;

        node w = e->opposite(v);

        if(hierNumber[w])
        {
            //temporarily fake trees
            //if (hierNumber[w] == hierNum) //forward search edge
            if(fakeTree)
            {
                //UG.type(e) = Graph::association;
                fakedGens.pushBack(e);
                continue;
            }
            else return false;//reached w over unused edge => no tree
        }

        returnValue = dfsGenTreeRec(UG, used, hierNumber, hierNum, w, fakedGens, fakeTree);
        //shortcut
        if(!returnValue) return false;
    }

    return returnValue;
}

edge firstOutGen(UMLGraph & UG, node v, EdgeArray<bool> & /* used */)
{
    //pruefen: kann es hier bereits ausgehende besuchte Kanten geben???
    edge e;
    forall_adj_edges(e, v)
    {
        if(e->target() == v) continue;
        if(UG.type(e) == Graph::generalization)
        {
            //OGDF_ASSERT(!used[e]);
            return e;
        }
        else continue;
    }//forall
    return 0;
}//firstOutGen

bool dfsGenTree(
    UMLGraph & UG,
    List<edge> & fakedGens,
    bool fakeTree)
{
    edge e;
    EdgeArray<bool> used(UG, false);
    //NodeArray<bool> visited(UG,false);
    NodeArray<int>  hierNumber(UG, 0);

    int hierNum = 0; //number of hierarchy tree

    const Graph & G = UG;
    forall_edges(e, G)
    {
        //descent in the hierarchy containing e
        if((!used[e]) && (UG.type(e) == Graph::generalization))
        {
            hierNum++; //current hierarchy tree
            //first we search for the sink
            node sink = e->target();
            edge sinkPath = firstOutGen(UG, e->target(), used);
            int cycleCounter = 0;
            while(sinkPath)
            {
                sink = sinkPath->target();
                sinkPath = firstOutGen(UG, sinkPath->target(), used);
                cycleCounter++;
                //if theres no sink, ?throw errGenCycle?, or convert Gens to Ass and draw
                if(cycleCounter > G.numberOfEdges())
                {
                    //versuche workaround: eigentlich werden die Typen erst spaeter
                    //gesetzt, damit es nicht zu Fehlern bei der Erkennung kommt, aber
                    //zum Abbruch wird hier bereits eine gesetzt (geht es auch ohne?)
                    UG.type(sinkPath) = Graph::association;
                    fakedGens.pushBack(sinkPath);
                    sink = sinkPath->source();
                    sinkPath = 0;
                    //throw OgdfException(errGenCycle); //vorlaeufig
                }
            }

            //now sink is the hierarchy sink

            //used is set in dfsGenTreeRec
            bool isTree = dfsGenTreeRec(UG, used, hierNumber, hierNum, sink, fakedGens, fakeTree);
            if(!isTree) return false;
        }

    }//forall_edges

    return true;
}

}//end namespace ogdf

#endif
