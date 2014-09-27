/*
 * $Revision: 2573 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-10 18:48:33 +0200 (Di, 10. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Computes the orthogonal representation of a planar
 *        representation of a UML graph using the simple flow
 *        approach.
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


#ifndef OGDF_ORTHO_FORMER_GENERIC_H
#define OGDF_ORTHO_FORMER_GENERIC_H


#include <ogdf/orthogonal/OrthoRep.h>
#include <ogdf/planarity/PlanRepUML.h>


namespace ogdf
{


class OGDF_EXPORT OrthoShaper
{
public:

    enum n_type { low, high, inner, outer }; //types of network nodes: nodes and faces

    OrthoShaper()
    {
        setDefaultSettings();
    }

    ~OrthoShaper() { }

    // Given a planar representation for a UML graph and its planar
    // combinatorial embedding, call() produces an orthogonal
    // representation using Tamassias bend minimization algorithm
    // with a flow network where every flow unit defines 90 degree angle
    // in traditional mode.

    void call(PlanRepUML & PG,
              CombinatorialEmbedding & E,
              OrthoRep & OR,
              bool fourPlanar = true);

    //sets the default settings used in the standard constructor
    void setDefaultSettings()
    {
        m_distributeEdges = true; // true;  //try to distribute edges to all node sides
        m_fourPlanar      = true;  //do not allow zero degree angles at high degree
        m_allowLowZero    = false; //do allow zero degree at low degree nodes
        m_multiAlign      = true;//true;  //start/end side of multi edges match
        m_traditional     = true;//true;  //prefer 3/1 flow at degree 2 (false: 2/2)
        m_deg4free        = false; //allow free angle assignment at degree four
        m_align           = false; //align nodes on same hierarchy level
        m_startBoundBendsPerEdge = 0; //don't use bound on bend number per edge
    }

    // returns option distributeEdges
    bool distributeEdges()
    {
        return m_distributeEdges;
    }
    // sets option distributeEdges to b
    void distributeEdges(bool b)
    {
        m_distributeEdges = b;
    }

    // returns option multiAlign
    bool multiAlign()
    {
        return m_multiAlign;
    }
    // sets option multiAlign to b
    void multiAlign(bool b)
    {
        m_multiAlign = b;
    }

    // returns option traditional
    bool traditional()
    {
        return m_traditional;
    }
    // sets option traditional to b
    void traditional(bool b)
    {
        m_traditional = b;
    }

    //returns option deg4free
    bool fixDegreeFourAngles()
    {
        return m_deg4free;
    }
    //sets option deg4free
    void fixDegreeFourAngles(bool b)
    {
        m_deg4free = b;
    }

    //alignment of brothers in hierarchies
    void align(bool al)
    {
        m_align = al;
    }
    bool align()
    {
        return m_align;
    }

    //! Set bound for number of bends per edge (none if set to 0). If shape
    //! flow computation is unsuccessful, the bound is increased iteratively.
    void setBendBound(int i)
    {
        OGDF_ASSERT(i >= 0);
        m_startBoundBendsPerEdge = i;
    }
    int getBendBound()
    {
        return m_startBoundBendsPerEdge;
    }

private:
    bool m_distributeEdges; // distribute edges among all sides if degree > 4
    bool m_fourPlanar;      // should the input graph be four planar
    // (no zero degree)
    bool m_allowLowZero;    // allow low degree nodes zero degree
    // (to low for zero...)
    bool m_multiAlign;      // multi edges aligned on the same side
    bool m_deg4free;        // allow degree four nodes free angle assignment
    bool m_traditional;     // do not prefer 180 degree angles,
    // traditional is not tamassia,
    // traditional is a kandinsky - ILP - like network with node supply 4,
    // not traditional interprets angle flow zero as 180 degree, "flow
    // through the node"
    bool m_align;           //try to achieve an alignment in hierarchy levels
    // A maximum number of bends per edge can be specified in
    // m_startBoundBendsPerEdge. If the algorithm is not successful in
    // producing a bend minimal representation subject to
    // startBoundBendsPerEdge, it successively enhances the bound by
    // one trying to compute an orthogonal representation.
    //
    // Using m_startBoundBendsPerEdge may not produce a bend minimal
    // representation in general.
    int m_startBoundBendsPerEdge;   //!< bound on the number of bends per edge for flow
    //!< if == 0, no bound is used

    //set angle boundary
    //warning: sets upper AND lower bounds, therefore may interfere with existing bounds
    void setAngleBound(
        edge netArc,
        int angle,
        EdgeArray<int> & lowB,
        EdgeArray<int> & upB,
        EdgeArray<edge> & aTwin,
        bool maxBound = true)
    {
        //vorlaeufig
        OGDF_ASSERT(!m_traditional);
        if(m_traditional)
        {
            switch(angle)
            {
            case 0:
            case 90:
            case 180:
                break;
                OGDF_NODEFAULT
            }//switch
        }//trad
        else
        {
            switch(angle)
            {
            case 0:
                if(maxBound)
                {
                    upB[netArc] = lowB[netArc] = 2;
                    edge e2 = aTwin[netArc];
                    if(e2)
                    {
                        upB[e2] = lowB[e2] = 0;
                    }
                }
                else
                {
                    upB[netArc] = 2;
                    lowB[netArc] = 0;
                    edge e2 = aTwin[netArc];
                    if(e2)
                    {
                        upB[e2] = 2;
                        lowB[e2] = 0;
                    }

                }
                break;
            case 90:
                if(maxBound)
                {
                    lowB[netArc] = 1;
                    upB[netArc] = 2;
                    edge e2 = aTwin[netArc];
                    if(e2)
                    {
                        upB[e2] = lowB[e2] = 0;
                    }
                }
                else
                {
                    upB[netArc] = 1;
                    lowB[netArc] = 0;
                    edge e2 = aTwin[netArc];
                    if(e2)
                    {
                        upB[e2] = 2;
                        lowB[e2] = 0;
                    }

                }
                break;
            case 180:
                if(maxBound)
                {
                    lowB[netArc] = 0;
                    upB[netArc] = 2;
                    edge e2 = aTwin[netArc];
                    if(e2)
                    {
                        upB[e2] = lowB[e2] = 0;
                    }
                }
                else
                {
                    upB[netArc] = 0;
                    lowB[netArc] = 0;
                    edge e2 = aTwin[netArc];
                    if(e2)
                    {
                        upB[e2] = 2;
                        lowB[e2] = 0;
                    }

                }
                break;
                OGDF_NODEFAULT // wrong bound
            }//switch
        }//progressive

    }//setAngle
};


} // end namespace ogdf


#endif
