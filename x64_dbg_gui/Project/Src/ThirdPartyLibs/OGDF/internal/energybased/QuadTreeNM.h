/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class QuadTreeNM.
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

#ifndef OGDF_QUAD_TREE_NM_H
#define OGDF_QUAD_TREE_NM_H

#include <ogdf/internal/energybased/QuadTreeNodeNM.h>
#include "ParticleInfo.h"


namespace ogdf
{

class QuadTreeNM
{
    //Helping data structure that stores the information needed to represent
    //the modified quadtree in the New Multipole Merthod (NMM)

public:
    QuadTreeNM();      //constructor
    ~QuadTreeNM() { }  //destructor

    //Deletes the tree starting at node_ptr.
    void delete_tree(QuadTreeNodeNM* node_ptr);

    //Deletes the tree starting at node_ptr and counts the nodes of the subtree.
    void delete_tree_and_count_nodes(QuadTreeNodeNM* node_ptr, int & nodecounter);

    //Pre_order traversal of the tree rooted at node_ptr (with or without
    //output of the M,L-lists from 0 to precision).
    void cout_preorder(QuadTreeNodeNM* node_ptr);
    void cout_preorder(QuadTreeNodeNM* node_ptr, int precision);

    //Creates the root node and lets act_ptr and root_ptr point to the root node.
    void init_tree()
    {
        root_ptr = new QuadTreeNodeNM();
        act_ptr = root_ptr;
    }

    //Sets act_ptr to the root_ptr.
    void start_at_root()
    {
        act_ptr = root_ptr;
    }

    //Sets act_ptr to the father_ptr.
    void go_to_father()
    {
        if(act_ptr->get_father_ptr() != NULL)
            act_ptr = act_ptr->get_father_ptr();
        else
            cout << "Error QuadTreeNM: No father Node exists";
    }

    //Sets act_ptr to the left_top_child_ptr.
    void go_to_lt_child()
    {
        act_ptr = act_ptr->get_child_lt_ptr();
    }

    //Sets act_ptr to the right_top_child_ptr.
    void go_to_rt_child()
    {
        act_ptr = act_ptr->get_child_rt_ptr();
    }

    //Sets act_ptr to the left_bottom_child_ptr.
    void go_to_lb_child()
    {
        act_ptr = act_ptr->get_child_lb_ptr();
    }

    //Sets act_ptr to the right_bottom_child_ptr.
    void go_to_rb_child()
    {
        act_ptr = act_ptr->get_child_rb_ptr();
    }

    //Creates a new left_top_child of the actual node (importing L_x(y)_ptr).
    void create_new_lt_child(List<ParticleInfo>* L_x_ptr, List<ParticleInfo>* L_y_ptr);
    void create_new_lt_child();

    //Creates a new right_top_child of the actual node (importing L_x(y)_ptr).
    void create_new_rt_child(List<ParticleInfo>* L_x_ptr, List<ParticleInfo>* L_y_ptr);
    void create_new_rt_child();

    //Creates a new left_bottom_child of the actual node (importing L_x(y)_ptr).
    void create_new_lb_child(List<ParticleInfo>* L_x_ptr, List<ParticleInfo>* L_y_ptr);
    void create_new_lb_child();

    //Creates a new right_bottom_child of the actual node(importing L_x(y)_ptr).
    void create_new_rb_child(List<ParticleInfo>* L_x_ptr, List<ParticleInfo>* L_y_ptr);
    void create_new_rb_child();

    //Returns the actual/root node pointer of the tree.
    QuadTreeNodeNM*  get_act_ptr()
    {
        return act_ptr;
    }
    QuadTreeNodeNM*  get_root_ptr()
    {
        return root_ptr;
    }

    //Sets root_ptr to r_ptr.
    void set_root_ptr(QuadTreeNodeNM* r_ptr)
    {
        root_ptr = r_ptr;
    }

    //Sets act_ptr to a_ptr.
    void set_act_ptr(QuadTreeNodeNM* a_ptr)
    {
        act_ptr = a_ptr;
    }

    //Sets the content of *root_ptr to r.
    void set_root_node(QuadTreeNodeNM & r)
    {
        *root_ptr = r;
    }

private:
    QuadTreeNodeNM* root_ptr; //points to the root node
    QuadTreeNodeNM* act_ptr;  //points to the actual node

};

}//namespace ogdf
#endif

