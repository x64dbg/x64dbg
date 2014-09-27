/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class OrthoLayout which represents an
 *        orthogonal planar drawing algorithm for mixed-upward
 *        embedded graphs.
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


#ifndef OGDF_UML_ORTHO_LAYOUT_H
#define OGDF_UML_ORTHO_LAYOUT_H


#include <ogdf/module/LayoutPlanRepModule.h>
#include <ogdf/orthogonal/OrthoRep.h>


namespace ogdf
{

enum OptionProfile { standard, minBendsperEdge, fullService }; //just to think about it...

enum OrthoDir;


//---------------------------------------------------------
// OrthoLayout
// represents planar orthogonal drawing algorithm for
// mixed-upward planar embedded graphs (UML-diagrams)
//---------------------------------------------------------
class OGDF_EXPORT OrthoLayout : public LayoutPlanRepModule
{
public:
    // constructor
    OrthoLayout();


    // calls planar UML layout algorithm. Input is a planarized representation
    // PG of a connected component of the graph, output is a layout of the
    // (modified) planarized representation in drawing
    void call(PlanRepUML & PG, adjEntry adjExternal, Layout & drawing);

    //
    // options

    // the minimum distance between edges and vertices
    double separation() const
    {
        return m_separation;
    }

    void separation(double sep)
    {
        m_separation = sep;
    }

    // cOverhang * separation is the minimum distance between the glue point
    // of an edge and a corner of the vertex boundary
    double cOverhang() const
    {
        return m_cOverhang;
    }

    void cOverhang(double c)
    {
        m_cOverhang = c;
    }


    // the distance from the tight bounding box to the boundary of the drawing
    double margin() const
    {
        return m_margin;
    }

    void margin(double m)
    {
        m_margin = m;
    }


    // the preferred direction of generalizations
    OrthoDir preferedDir() const
    {
        return m_preferedDir;
    }

    void preferedDir(OrthoDir dir)
    {
        m_preferedDir = dir;
    }

    // cost of associations
    int costAssoc() const
    {
        return m_costAssoc;
    }

    void costAssoc(int c)
    {
        m_costAssoc = c;
    }

    // cost of generalizations
    int costGen() const
    {
        return m_costGen;
    }

    void costGen(int c)
    {
        m_costGen = c;
    }

    //! Set the option profile, thereby fixing a set of drawing options
    void optionProfile(int i)
    {
        m_optionProfile = i;
    }

    //! Set alignment option
    void align(bool b)
    {
        m_align = b;
    }

    //! Set scaling compaction
    void scaling(bool b)
    {
        m_useScalingCompaction = b;
    }

    //! Set bound on the number of bends
    void setBendBound(int i)
    {
        OGDF_ASSERT(i >= 0);
        m_bendBound = i;
    }

    //in planarlayout
    //enum LayoutOptions {umloptAlignment = 1, optScaling = 2, optProgressive = 4}
    //set generic options by setting field bits,
    //necessary to allow setting over base class pointer
    //bit 0 = alignment
    //bit 1 = scaling
    //bit 2 = progressive/traditional
    //=> 0 is standard
    virtual void setOptions(int optionField)
    {
        if(optionField & umlOpAlign) m_align = true;
        else m_align = false;
        if(optionField & umlOpScale) m_useScalingCompaction = true;
        else m_useScalingCompaction = false;
        if(optionField & umlOpProg) m_orthoStyle = 1;
        else m_orthoStyle = 0; //traditional
    }//setOptions

    virtual int getOptions()
    {
        int result = 0;
        if(m_align) result = umlOpAlign;
        if(m_useScalingCompaction) result += umlOpScale;
        if(m_orthoStyle == 1) result += umlOpProg;

        return result;
    }//getOptions


protected:
    void classifyEdges(PlanRepUML & PG, adjEntry & adjExternal);

private:
    // compute bounding box and move final drawing such that it is 0 aligned
    // respecting margins
    void computeBoundingBox(const PlanRepUML & PG, Layout & drawing);


    // options
    double m_separation;
    double m_cOverhang;
    double m_margin;
    OrthoDir m_preferedDir;
    int m_optionProfile;
    int m_costAssoc;
    int m_costGen;
    //align merger sons on same level
    bool m_align;
    //settings for scaling compaction
    bool m_useScalingCompaction;
    int m_scalingSteps;
    //mainly used for OrthoShaper traditional/progressive
    int m_orthoStyle;
    int m_bendBound; //!< bounds number of bends per edge in ortho shaper
};


} // end namespace ogdf


#endif
