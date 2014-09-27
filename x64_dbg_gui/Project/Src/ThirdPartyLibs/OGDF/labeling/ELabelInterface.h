/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Provide an interface for edge label information
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

#ifndef OGDF_E_LABEL_INTERFACE_H
#define OGDF_E_LABEL_INTERFACE_H

#include <ogdf/orthogonal/OrthoLayout.h>
#include <ogdf/basic/GridLayout.h>
#include <ogdf/basic/GridLayoutMapped.h>
#include <ogdf/planarity/PlanRepUML.h>


namespace ogdf
{

//********************************************************
// the available labels
// the five basic labels are not allowed to be changed,
// cause they have a special meaning/position, insert
// other labels between mult1/End2

enum eLabelType
{
    elEnd1 = 0,
    elMult1,
    elName,
    elEnd2,
    elMult2,
    elNumLabels  //!< the number of available labels at an edge
};

enum eUsedLabels
{
    lEnd1  = (1 << elEnd1),         //  1
    lMult1 = (1 << elMult1),        //  2
    lName  = (1 << elName),         //  4
    lEnd2  = (1 << elEnd2),         //  8
    lMult2 = (1 << elMult2),        // 16
    lAll   = (1 << elNumLabels) - 1, // 31
};


//*************************************
// the basic single label defining class
// holds info about all labels for one edge
template <class coordType>
class OGDF_EXPORT EdgeLabel
{
public:

    //construction and destruction
    EdgeLabel()
    {
        m_edge = 0;
        m_usedLabels = 0;
    }

    //bit pattern 2^labelenumpos bitwise
    EdgeLabel(edge e, int usedLabels = lAll) : m_usedLabels(usedLabels), m_edge(e)
    {
        for(int i = 0; i < elNumLabels; i++)
        {
            //zu testzwecken randoms
            m_xSize[i] = double(randomNumber(5, 13)) / 50.0; //1
            m_ySize[i] = double(randomNumber(3, 7)) / 50.0; //1

            m_xPos[i] = 0;
            m_yPos[i] = 0;
        }
    }

    // Construction with specification of label sizes in arrays of length labelnum
    EdgeLabel(edge e, coordType w[], coordType h[], int usedLabels = lAll) : m_usedLabels(usedLabels), m_edge(e)
    {
        for(int i = 0; i < elNumLabels; i++)
        {
            m_xSize[i] = w[i];
            m_ySize[i] = h[i];
            m_xPos[i] = 0;
            m_yPos[i] = 0;
        }
    }

    EdgeLabel(edge e, coordType w, coordType h, int usedLabels) : m_usedLabels(usedLabels), m_edge(e)
    {
        for(int i = 0; i < elNumLabels; i++)
            if(m_usedLabels & (1 << i))
            {
                m_xPos[i] = 0.0;
                m_yPos[i] = 0.0;
                m_xSize[i] = w;
                m_ySize[i] = h;
            }
    }

    //copy constructor
    EdgeLabel(const EdgeLabel & rhs) : m_usedLabels(rhs.m_usedLabels), m_edge(rhs.m_edge)
    {
        for(int i = 0; i < elNumLabels; i++)
        {
            m_xPos[i] = rhs.m_xPos[i];
            m_yPos[i] = rhs.m_yPos[i];
            m_xSize[i] = rhs.m_xSize[i];
            m_ySize[i] = rhs.m_ySize[i];
        }
    }//copy con

    ~EdgeLabel() { }

    //assignment
    EdgeLabel & operator=(const EdgeLabel & rhs)
    {
        if(this != &rhs)
        {
            m_usedLabels = rhs.m_usedLabels;
            m_edge = rhs.m_edge;
            int i;
            for(i = 0; i < elNumLabels; i++)
            {
                m_xPos[i] = rhs.m_xPos[i];
                m_yPos[i] = rhs.m_yPos[i];
                m_xSize[i] = rhs.m_xSize[i];
                m_ySize[i] = rhs.m_ySize[i];
            }
        }
        return *this;
    }//assignment

    EdgeLabel & operator|=(const EdgeLabel & rhs)
    {
        if(m_edge)
        {
            OGDF_ASSERT(m_edge == rhs.m_edge);
        }
        else
            m_edge = rhs.m_edge;
        if(this != &rhs)
        {
            m_usedLabels |= rhs.m_usedLabels;
            for(int i = 0; i < elNumLabels; i++)
                if(rhs.m_usedLabels & (1 << i))
                {
                    m_xPos[i] = rhs.m_xPos[i];
                    m_yPos[i] = rhs.m_yPos[i];
                    m_xSize[i] = rhs.m_xSize[i];
                    m_ySize[i] = rhs.m_ySize[i];
                }
        }
        return *this;
    }


    //set
    void setX(eLabelType elt, coordType x)
    {
        m_xPos[elt] = x;
    }
    void setY(eLabelType elt, coordType y)
    {
        m_yPos[elt] = y;
    }
    void setHeight(eLabelType elt, coordType h)
    {
        m_ySize[elt] = h;
    }
    void setWidth(eLabelType elt, coordType w)
    {
        m_xSize[elt] = w;
    }
    void setEdge(edge e)
    {
        m_edge = e;
    }
    void addType(eLabelType elt)
    {
        m_usedLabels |= (1 << elt);
    }

    //get
    coordType getX(eLabelType elt) const
    {
        return m_xPos[elt];
    }
    coordType getY(eLabelType elt) const
    {
        return m_yPos[elt];
    }
    coordType getWidth(eLabelType elt) const
    {
        return m_xSize[elt];
    }
    coordType getHeight(eLabelType elt) const
    {
        return m_ySize[elt];
    }
    edge theEdge() const
    {
        return m_edge;
    }

    bool usedLabel(eLabelType elt) const
    {
        return ((m_usedLabels & (1 << elt)) > 0);
    }

    int & usedLabel()
    {
        return m_usedLabels;
    }


private:

    //the positions of the labels
    coordType m_xPos[elNumLabels];
    coordType m_yPos[elNumLabels];

    //the input label sizes
    coordType m_xSize[elNumLabels];
    coordType m_ySize[elNumLabels];

    //which labels have to be placed bit pattern 2^labelenumpos bitwise
    int m_usedLabels; //1 = only name, 5 = name and end2, ...

    //the edge of heaven
    edge m_edge;

    //the label text
    //String m_string;


};//edgelabel


//*********************
//Interface to algorithm
template <class coordType>
class ELabelInterface
{
public:
    //constructor
    ELabelInterface(PlanRepUML & pru)
    {
        //the PRU should not work with real world data but with
        //normalized integer values
        m_distDefault = 2;
        m_minFeatDist = 1;
        m_labels.init(pru.original());
        m_ug = 0;

        //temporary
        edge e;
        forall_edges(e, pru.original())
        setLabel(e, EdgeLabel<coordType>(e, 0));
    }

    //constructor on GraphAttributes
    ELabelInterface(GraphAttributes & uml) : m_ug(&uml)
    {
        //the GraphAttributes should work on real world data,
        //which can be floats or ints
        m_distDefault = 0.002;
        m_minFeatDist = 0.003;
        m_labels.init(uml.constGraph());

        //temporary
        edge e;
        forall_edges(e, uml.constGraph())
        setLabel(e, EdgeLabel<coordType>(e, 0));
    }

    GraphAttributes & graph()
    {
        return *m_ug;
    }

    //set new EdgeLabel
    void setLabel(const edge & e, const EdgeLabel<coordType> & el)
    {
        m_labels[e] = el;
    }

    void addLabel(const edge & e, const EdgeLabel<coordType> & el)
    {
        m_labels[e] |= el;
    }

    //get info about current EdgeLabel
    EdgeLabel<coordType> & getLabel(edge e)
    {
        return m_labels[e];
    }

    coordType getWidth(edge e, eLabelType elt)
    {
        return m_labels[e].getWidth(elt);
    }
    coordType getHeight(edge e, eLabelType elt)
    {
        return m_labels[e].getHeight(elt);
    }

    //get general information
    coordType & minFeatDist()
    {
        return m_minFeatDist;
    }
    coordType & distDefault()
    {
        return m_distDefault;
    }

private:

    EdgeArray<EdgeLabel<coordType> > m_labels; //holds all labels for original edges
    //the base graph
    GraphAttributes* m_ug;

    coordType m_distDefault; //default distance label/edge for positioner
    coordType m_minFeatDist; //min Distance label/feature in candidate posit.
};//ELabelInterface


}//end namespace

#endif
