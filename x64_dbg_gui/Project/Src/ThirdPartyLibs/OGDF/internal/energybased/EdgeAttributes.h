/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class EdgeAttributes.
 *
 * \author Stefan Hachul
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

#ifndef OGDF_EDGE_ATTRIBUTES_H
#define OGDF_EDGE_ATTRIBUTES_H

#include <ogdf/basic/geometry.h>
#include <ogdf/basic/Graph.h>

namespace ogdf
{


class OGDF_EXPORT EdgeAttributes
{
    //helping data structure that stores the graphical attributes of an edge
    //that are needed for the force-directed  algorithms.

    //outputstream for EdgeAttributes
    friend ostream & operator<< (ostream &, const EdgeAttributes &);

    //inputstream for EdgeAttributes
    friend istream & operator>> (istream &, EdgeAttributes &);

public:

    EdgeAttributes();       //constructor
    ~EdgeAttributes() { }   //destructor

    void set_EdgeAttributes(double l, edge e_orig, edge e_sub)
    {
        length = l;
        e_original = e_orig;
        e_subgraph = e_sub;
    }

    void set_length(double l)
    {
        length = l;
    }
    double get_length() const
    {
        return length;
    }


    //needed for the divide et impera step in FMMM

    void set_original_edge(edge e)
    {
        e_original = e;
    }
    void set_subgraph_edge(edge e)
    {
        e_subgraph = e;
    }
    edge get_original_edge() const
    {
        return e_original;
    }
    edge get_subgraph_edge() const
    {
        return e_subgraph;
    }

    //needed for the preprocessing step in FMMM (set/get_original_edge are needed, too)

    void set_copy_edge(edge e)
    {
        e_subgraph = e;
    }
    edge get_copy_edge() const
    {
        return e_subgraph;
    }

    //needed for multilevel step

    void set_higher_level_edge(edge e)
    {
        e_subgraph = e;
    }
    edge get_higher_level_edge() const
    {
        return e_subgraph;
    }
    bool is_moon_edge() const
    {
        return moon_edge;
    }
    void make_moon_edge()
    {
        moon_edge = true;
    }
    bool is_extra_edge() const
    {
        return extra_edge;
    }
    void make_extra_edge()
    {
        extra_edge = true;
    }
    void mark_as_normal_edge()
    {
        extra_edge = false;
    }
    void init_mult_values()
    {
        e_subgraph = NULL;
        moon_edge = false;
    }

private:
    double length;
    edge e_original;
    edge e_subgraph;
    bool moon_edge; //indicates if this edge is associasted with a moon node
    bool extra_edge;//indicates if this edge is an extra edge that is added to
    //enforce few edge crossings
};

}//namespace ogdf
#endif

