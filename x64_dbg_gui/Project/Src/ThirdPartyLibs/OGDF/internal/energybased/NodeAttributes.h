/*
 * $Revision: 2555 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-06 12:12:10 +0200 (Fr, 06. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class NodeAttributes.
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

#ifndef OGDF_NODE_ATTRIBUTES_H
#define OGDF_NODE_ATTRIBUTES_H

#include <ogdf/basic/geometry.h>
#include <ogdf/basic/Graph.h>
#include <ogdf/basic/List.h>

namespace ogdf
{

class OGDF_EXPORT NodeAttributes
{
    //helping data structure that stores the graphical attributes of a node
    //that are needed for the force-directed algorithms.


    //outputstream for NodeAttributes
    friend ostream & operator<< (ostream &, const NodeAttributes &);

    //inputstream for NodeAttributes
    friend istream & operator>> (istream &, NodeAttributes &);

public:

    NodeAttributes();       //constructor
    ~NodeAttributes() { }   //destructor

    void set_NodeAttributes(double w, double h, DPoint pos, node v_low, node
                            v_high)
    {
        width = w;
        height = h;
        position = pos;
        v_lower_level = v_low;
        v_higher_level = v_high;
    }

    void set_position(DPoint pos)
    {
        position = pos;
    }
    void set_width(double w)
    {
        width = w;
    }
    void set_height(double h)
    {
        height = h;
    }
    void set_x(double x)
    {
        position.m_x = x;
    }
    void set_y(double y)
    {
        position.m_y = y;
    }

    DPoint get_position() const
    {
        return position;
    }
    double get_x() const
    {
        return position.m_x;
    }
    double get_y() const
    {
        return position.m_y;
    }
    double get_width() const
    {
        return width;
    }
    double get_height() const
    {
        return height;
    }


    //for preprocessing step in FMMM

    void set_original_node(node v)
    {
        v_lower_level = v;
    }
    void set_copy_node(node v)
    {
        v_higher_level = v;
    }
    node get_original_node() const
    {
        return v_lower_level;
    }
    node get_copy_node() const
    {
        return v_higher_level;
    }

    //for divide et impera step in FMMM (set/get_original_node() are needed, too)

    void set_subgraph_node(node v)
    {
        v_higher_level = v;
    }
    node get_subgraph_node() const
    {
        return v_higher_level;
    }

    //for the multilevel step in FMMM

    void set_lower_level_node(node v)
    {
        v_lower_level = v;
    }
    void set_higher_level_node(node v)
    {
        v_higher_level = v;
    }
    node get_lower_level_node() const
    {
        return v_lower_level;
    }
    node get_higher_level_node() const
    {
        return v_higher_level;
    }
    void set_mass(int m)
    {
        mass = m;
    }
    void set_type(int t)
    {
        type = t;
    }
    void set_dedicated_sun_node(node v)
    {
        dedicated_sun_node = v;
    }
    void set_dedicated_sun_distance(double d)
    {
        dedicated_sun_distance = d;
    }
    void set_dedicated_pm_node(node v)
    {
        dedicated_pm_node = v;
    }
    void place()
    {
        placed = true;
    }
    void set_angle_1(double a)
    {
        angle_1 = a;
    }
    void set_angle_2(double a)
    {
        angle_2 = a;
    }

    int get_mass() const
    {
        return mass;
    }
    int get_type() const
    {
        return type;
    }
    node get_dedicated_sun_node() const
    {
        return dedicated_sun_node;
    }
    double get_dedicated_sun_distance() const
    {
        return dedicated_sun_distance;
    }
    node get_dedicated_pm_node() const
    {
        return dedicated_pm_node;
    }
    bool is_placed() const
    {
        return placed;
    }
    double get_angle_1() const
    {
        return angle_1;
    }
    double get_angle_2() const
    {
        return angle_2;
    }


    List<double>* get_lambda_List_ptr()
    {
        return lambda_List_ptr;
    }
    List<node>* get_neighbour_sun_node_List_ptr()
    {
        return neighbour_s_node_List_ptr;
    }
    List<node>* get_dedicated_moon_node_List_ptr()
    {
        return moon_List_ptr;
    }


    //initialzes all values needed for multilevel representations
    void init_mult_values();

private:

    DPoint position;
    double width;
    double height;

    //for the multilevel and divide et impera and preprocessing step

    node v_lower_level; //the corresponding node in the lower level graph
    node v_higher_level;//the corresponding node in the higher level graph
    //for divide et impera v_lower_level is the original graph and
    //v_higher_level is the copy of the copy of this node in the
    //maximum connected subraph

    //for the multilevel step

    int mass; //the mass (= number of previously collapsed nodes) of this node
    int type; //1 = sun node (s_node); 2 = planet node (p_node) without a dedicate moon
    //3 = planet node with dedicated moons (pm_node);4 = moon node (m_node)
    node dedicated_sun_node; //the dedicates s_node of the solar system of this node
    double dedicated_sun_distance;//the distance to the dedicated sun node of the galaxy
    //of this node
    node dedicated_pm_node;//if type == 4 the dedicated_pm_node is saved here
    List<double> lambda; //the factors lambda for scaling the length of this edge
    //relative to the pass between v's sun and the sun of a
    //neighbour solar system
    List<node> neighbour_s_node;//this is the list of the neighbour solar systems suns
    //lambda[i] corresponds to neighbour_s_node[i]
    List<double>* lambda_List_ptr; //a pointer to the lambda list
    List<node>* neighbour_s_node_List_ptr; //a pointer to to the neighbour_s_node list
    List<node>  moon_List;//the list of all dedicated moon nodes (!= nil if type == 3)
    List<node>* moon_List_ptr;//a pointer to the moon_List
    bool placed;   //indicates weather an initial position has been assigned to this
    //node or not
    double angle_1;//describes the sector where nodes that are not adjacent to other
    double angle_2;//solar systems have to be placed
};

}//namespace ogdf
#endif
