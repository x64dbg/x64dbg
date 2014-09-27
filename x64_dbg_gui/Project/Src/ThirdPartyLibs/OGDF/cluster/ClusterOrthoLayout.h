/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declares ClusterOrthoLayout which represents an orthogonal
 * planar drawing algorithm for c-planar c-connected Clustergraphs.
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


#ifndef OGDF_CLUSTER_ORTHO_LAYOUT_H
#define OGDF_CLUSTER_ORTHO_LAYOUT_H


#include <ogdf/module/LayoutClusterPlanRepModule.h>
#include <ogdf/orthogonal/OrthoRep.h>
#include <ogdf/cluster/ClusterPlanRep.h>
#include <ogdf/cluster/CPlanarEdgeInserter.h>


namespace ogdf
{

enum OrthoDir;


//---------------------------------------------------------
// ClusterOrthoLayout
// represents planar orthogonal drawing algorithm for
// c-planar c-connected Clustergraphs
//---------------------------------------------------------
class OGDF_EXPORT ClusterOrthoLayout : public LayoutClusterPlanRepModule
{
public:
    //! Initializes an instance of class ClusterOrthoLayout
    ClusterOrthoLayout();


    /** \brief Calls planar UML layout algorithm.
     *
     * Input is a planarized representation \a PG of a connected component
     * of the graph, output is a layout of the (modified) planarized
     * representation in \a drawing.
     */
    void call(ClusterPlanRep & PG, adjEntry adjExternal, Layout & drawing);
    //! Call method for non c-planar graphs
    void call(
        ClusterPlanRep & PG,
        adjEntry adjExternal,
        Layout & drawing,
        List<NodePair> & npEdges,
        List<edge> & newEdges,
        Graph & originalGraph);

    //void call(PlanRepUML & /* PG */, adjEntry /* adjExternal */, Layout & /* drawing */) {}

    //
    // options

    //! Returns the minimum distance between edges and vertices.
    double separation() const
    {
        return m_separation;
    }
    //! Sets the minimum distance between edges and vertices.
    void separation(double sep)
    {
        m_separation = sep;
    }

    //! Returns cOverhang, where cOverhang * separation defines the minimum
    //distance between the glue point of an edge and a corner of the vertex boundary
    double cOverhang() const
    {
        return m_cOverhang;
    }
    //! Sets cOverhang value
    void cOverhang(double c)
    {
        m_cOverhang = c;
    }


    //! Returns the distance from the tight bounding box to the boundary of the drawing.
    double margin() const
    {
        return m_margin;
    }
    //! Sets the distance from the tight bounding box to the boundary of the drawing.
    void margin(double m)
    {
        m_margin = m;
    }


    //! Returns the preferred direction of generalizations
    OrthoDir preferedDir() const
    {
        return m_preferedDir;
    }
    //! Sets the preferred direction of generalizations
    void preferedDir(OrthoDir dir)
    {
        m_preferedDir = dir;
    }

    //! Returns cost of associations which is used in the compactions step.
    int costAssoc() const
    {
        return m_costAssoc;
    }
    //! Sets cost of associations which is used in the compactions step.
    void costAssoc(int c)
    {
        m_costAssoc = c;
    }

    //! Returns cost of generalizations.
    int costGen() const
    {
        return m_costGen;
    }
    //! Sets cost of generalizations.
    void costGen(int c)
    {
        m_costGen = c;
    }

    //! Sets the option profile, thereby fixing a set of drawing options.
    void optionProfile(int i)
    {
        m_optionProfile = i;
    }

    //! Sets alignment option.
    void align(bool b)
    {
        m_align = b;
    }

    //! Sets scaling option for compaction step.
    void scaling(bool b)
    {
        m_useScalingCompaction = b;
    }

    //! Sets generic options by setting field bits.
    //Necessary to allow setting over base class pointer
    //bit 0 = alignment
    //bit 1 = scaling
    //bit 2 = progressive/traditional
    //=> 0 is standard
    virtual void setOptions(int optionField)
    {
        if(optionField & 1) m_align = true;
        else m_align = false;
        if(optionField & 2) m_useScalingCompaction = true;
        else m_useScalingCompaction = false;
        if(optionField & 4) m_orthoStyle = 1;
        else m_orthoStyle = 0; //traditional
    }


private:
    // compute bounding box and move final drawing such that it is 0 aligned
    // respecting margins
    void computeBoundingBox(const ClusterPlanRep & PG, Layout & drawing);


    // options
    double m_separation;//!< Minimum distance between edges and vertices.
    double m_cOverhang; //!< Factor for minimum distance between vertex corner an adjacent edges.
    double m_margin;    //!< Distance between bounding box and drawing boundary.
    OrthoDir m_preferedDir; //!< Preferred direction of generalizations (obsolete).
    int m_optionProfile;
    int m_costAssoc;    //!< Compaction cost of association type edges.
    int m_costGen;      //!< Compaction cost of generalizations type edges.
    //align merger sons on same level
    bool m_align;       //!< Horizontal alignment option.
    //settings for scaling compaction
    bool m_useScalingCompaction; //!< Switches scaling improvement during compaction.
    int m_scalingSteps; //!< Number of scaling steps during compaction.
    int m_orthoStyle;   //!< Type of style (traditional/progressive) used for shape step.
};


} // end namespace ogdf


#endif
