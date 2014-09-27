/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class NMM (New Multipole Method).
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

#ifndef OGDF_NMM_H
#define OGDF_NMM_H

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/List.h>
#include <ogdf/basic/Array2D.h>
#include <ogdf/basic/geometry.h>
#include <ogdf/internal/energybased/NodeAttributes.h>
#include <ogdf/internal/energybased/EdgeAttributes.h>
#include <ogdf/internal/energybased/QuadTreeNM.h>
#include <ogdf/internal/energybased/ParticleInfo.h>
#include <ogdf/internal/energybased/FruchtermanReingold.h>
#include <complex>


namespace ogdf
{

class OGDF_EXPORT NMM
{
public:
    NMM();          //constructor
    ~NMM() { }      //destructor

    //Calculate rep. forces for each node.
    void calculate_repulsive_forces(const Graph & G,
                                    NodeArray<NodeAttributes> & A,
                                    NodeArray<DPoint> & F_rep);

    //Make all initialisations that are needed for New Multipole Method (NMM)
    void make_initialisations(const Graph & G,
                              double boxlength,
                              DPoint down_left_corner,
                              int particles_in_leaves,
                              int precision,
                              int tree_construction_way,
                              int find_small_cell);

    //Dynamically allocated memory is freed here.
    void deallocate_memory();

    //Import updated information of the drawing area.
    void update_boxlength_and_cornercoordinate(double b_l, DPoint d_l_c);

private:
    int MIN_NODE_NUMBER; //The minimum number of nodes for which the forces are
    //calculated using NMM (for lower values the exact
    //calculation is used).
    bool using_NMM; //Indicates whether the exact method or NMM is used for force
    //calculation (value depends on MIN_NODE_NUMBER)
    FruchtermanReingold ExactMethod; //needed in case that using_NMM == false

    int _tree_construction_way;//1 = pathwise;2 = subtreewise
    int _find_small_cell;//0 = iterative; 1= Aluru
    int _particles_in_leaves;//max. number of particles for leaves of the quadtree
    int _precision;  //precision for p-term multipole expansion

    double boxlength;//length of drawing box
    DPoint down_left_corner;//down left corner of drawing box

    int* power_of_2; //holds the powers of 2 (for speed reasons to calculate the
    //maximal boxindex (index is from 0 to max_power_of_2_index)
    int max_power_of_2_index;//holds max. index for power_of_2 (= 30)
    double** BK;  //holds the binomial coefficients
    List<DPoint> rep_forces;    //stores the rep. forces of the last iteration
    //(needed for error calculation)

    //private helping functions

    //The array power_of_2 is calculated for values from 0 to max_power_of_2_index
    //which is set here to 30.
    void init_power_of_2_array();

    //The space of power_of_2 is freed.
    void free_power_of_2_array();

    //Returns power_of_2[i] for values <= max_power_of_2_index else it returns
    //pow(2,i).
    int power_of_two(int i);

    //Returns the maximal index of a box in level i.
    int maxboxindex(int level);

    //Use NMM for force calculation (used for large Graphs (|V| > MIN_NODE_NUMBER)).
    void  calculate_repulsive_forces_by_NMM(const Graph & G, NodeArray
                                            <NodeAttributes> & A, NodeArray<DPoint> & F_rep);

    //Use the exact method for force calculation (used for small Graphs (|V| <=
    //MIN_NODE_NUMBER) for speed reasons).
    void  calculate_repulsive_forces_by_exact_method(const Graph & G,
            NodeArray<NodeAttributes> & A,
            NodeArray<DPoint> & F_rep);

    // *********Functions needed for path by path tree construction***********

    //The reduced quadtree is build up path by path (the Lists LE,ME, the
    //centers, D1, D2, M, and quad_tree_leaves are not calculated here.
    void  build_up_red_quad_tree_path_by_path(const Graph & G,
            NodeArray<NodeAttributes> & A,
            QuadTreeNM & T);

    //Makes L_x(y)_copy a copy of L_x(y)_orig and sets p.copy_item for each element in
    //L_x(y)_orig to the ListIterator of the corresponding element in L_x(y)_copy;
    //Furthermore, the p.cross_ref_items in L_x(y)_copy are set and p.subList_ptr and
    //p.tmp_cross_ref_item is reset to NULL in both lists.
    void make_copy_and_init_Lists(List<ParticleInfo> & L_x_orig,
                                  List<ParticleInfo> & L_x_copy,
                                  List<ParticleInfo> & L_y_orig,
                                  List<ParticleInfo> & L_y_copy);

    //The root node of T is constructed.
    void build_up_root_node(const Graph & G, NodeArray<NodeAttributes> & A, QuadTreeNM & T);

    //The sorted and linked Lists L_x and L_y for the root node are created.
    void create_sorted_coordinate_Lists(const Graph & G, NodeArray<NodeAttributes> & A,
                                        List<ParticleInfo> & L_x, List<ParticleInfo> & L_y);

    //T is extended by a subtree T1 rooted at the T.get_act_node().
    //The boxlength and down_left_corner of the actual node is reduced if it is
    //not the minimal subquad that contains all the particles in the represented area.
    void  decompose_subtreenode(QuadTreeNM & T,
                                List<ParticleInfo> & act_x_List_copy,
                                List<ParticleInfo> & act_y_List_copy,
                                List<QuadTreeNodeNM*> & new_leaf_List);

    //The extreme coordinates of the particles contained in *act_ptr are calculated.
    void calculate_boundaries_of_act_node(QuadTreeNodeNM* act_ptr,
                                          double & x_min,
                                          double & x_max,
                                          double & y_min,
                                          double & y_max);

    //Returns true if the rectangle defined by x_min,...,y_max lies within the
    //left(right)_top(bottom) quad of the small cell of *act_ptr.
    bool in_lt_quad(QuadTreeNodeNM* act_ptr, double x_min, double x_max, double y_min, double y_max);
    bool in_rt_quad(QuadTreeNodeNM* act_ptr, double x_min, double x_max, double y_min, double y_max);
    bool in_lb_quad(QuadTreeNodeNM* act_ptr, double x_min, double x_max, double y_min, double y_max);
    bool in_rb_quad(QuadTreeNodeNM* act_ptr, double x_min, double x_max, double y_min, double y_max);

    //The Lists *act_ptr->get_x(y)_List_ptr() are split into two sublists containing
    //the particles in the left and right half of the actual quad. The list that is
    //larger is constructed from *act_ptr->get_x(y)_List_ptr() by deleting the other
    //elements; The smaller List stays empty at this point, but the corresponding
    //elements in L_x(y)_copy contain a pointer to the x(y) List, where they belong to.
    void split_in_x_direction(
        QuadTreeNodeNM* act_ptr,
        List<ParticleInfo>* & L_x_left_ptr,
        List<ParticleInfo>* & L_y_left_ptr,
        List<ParticleInfo>* & L_x_right_ptr,
        List <ParticleInfo>* & L_y_right_ptr);

    //The Lists *act_ptr->get_x(y)_List_ptr() are split into two subLists containing
    //the particles in the top /bottom half ...
    void split_in_y_direction(
        QuadTreeNodeNM* act_ptr,
        List<ParticleInfo>* & L_x_bottom_ptr,
        List<ParticleInfo>* & L_y_bottom_ptr,
        List<ParticleInfo>* & L_x_top_ptr,
        List<ParticleInfo>* & L_y_top_ptr);


    //The Lists *L_x(y)_left_ptr are constructed from *act_ptr->get_x(y)_List_ptr()
    //by deleting all elements right from last_left_item in *act_ptr->get_x_List_ptr()
    //the corresponding values in  *act_ptr->get_y_List_ptr() are deleted as well.
    //The corresponding List-elements of the deleted elements in the Lists L_x(y)_copy
    //hold the information, that they belong to the Lists *L_x(y)_left_ptr.
    void x_delete_right_subLists(
        QuadTreeNodeNM* act_ptr,
        List<ParticleInfo>* & L_x_left_ptr,
        List <ParticleInfo>* & L_y_left_ptr,
        List<ParticleInfo>* & L_x_right_ptr,
        List <ParticleInfo>* & L_y_right_ptr,
        ListIterator<ParticleInfo> last_left_item);

    //Analogue as above.
    void x_delete_left_subLists(
        QuadTreeNodeNM* act_ptr,
        List<ParticleInfo>* & L_x_left_ptr,
        List <ParticleInfo>* & L_y_left_ptr,
        List<ParticleInfo>* & L_x_right_ptr,
        List <ParticleInfo>* & L_y_right_ptr,
        ListIterator<ParticleInfo> last_left_item);

    //The Lists *L_x(y)_left_ptr are constructed from *act_ptr->get_x(y)_List_ptr()
    //by deleting all elements right from last_left_item in *act_ptr->get_y_List_ptr()
    //the ...
    void y_delete_right_subLists(
        QuadTreeNodeNM* act_ptr,
        List<ParticleInfo>* & L_x_left_ptr,
        List<ParticleInfo>* & L_y_left_ptr,
        List<ParticleInfo>* & L_x_right_ptr,
        List <ParticleInfo>* & L_y_right_ptr,
        ListIterator<ParticleInfo> last_left_item);

    //Analogue as above.
    void y_delete_left_subLists(
        QuadTreeNodeNM* act_ptr,
        List<ParticleInfo>* & L_x_left_ptr,
        List<ParticleInfo>* & L_y_left_ptr,
        List<ParticleInfo>* & L_x_right_ptr,
        List <ParticleInfo>* & L_y_right_ptr,
        ListIterator<ParticleInfo> last_left_item);


    //The Lists *L_x(y)_b_ptr and *L_x(y)_t_ptr are constructed from the Lists
    // *L_x(y)_ptr.
    void split_in_y_direction(
        QuadTreeNodeNM* act_ptr,
        List<ParticleInfo>* & L_x_ptr,
        List<ParticleInfo>* & L_x_b_ptr,
        List<ParticleInfo>* & L_x_t_ptr,
        List<ParticleInfo>* & L_y_ptr,
        List<ParticleInfo>* & L_y_b_ptr,
        List<ParticleInfo>* & L_y_t_ptr);

    //The Lists *L_x(y)_b(t)_ptr are constructed from the Lists *L_x(y)_ptr by
    //moving all List elements from *L_x(y)_ptr that belong to *L_x(y)_l_ptr
    //to this List. the L_x(y)_right_ptr point  to the reduced Lists L_x(y)_ptr
    //afterwards.
    void y_move_left_subLists(List<ParticleInfo>* & L_x_ptr,
                              List<ParticleInfo>* & L_x_b_ptr,
                              List<ParticleInfo>* & L_x_t_ptr,
                              List<ParticleInfo>* & L_y_ptr,
                              List <ParticleInfo>* & L_y_b_ptr,
                              List<ParticleInfo>* & L_y_t_ptr,
                              ListIterator<ParticleInfo> last_left_item);

    //Same as above but the elements that belong to *&L_x(y)_right_ptr are moved.
    void y_move_right_subLists(List<ParticleInfo>* & L_x_ptr,
                               List<ParticleInfo>* & L_x_b_ptr,
                               List<ParticleInfo>* & L_x_t_ptr,
                               List<ParticleInfo>* & L_y_ptr,
                               List <ParticleInfo>* & L_y_b_ptr,
                               List<ParticleInfo>* & L_y_t_ptr,
                               ListIterator<ParticleInfo> last_left_item);

    //The sorted subLists, that can be accesssed by the entries in L_x(y)_copy->
    //get_subList_ptr() are constructed.
    void build_up_sorted_subLists(List<ParticleInfo> & L_x_copy,
                                  List<ParticleInfo> & act_y_List_copy);

    // ************functions needed for subtree by subtree tree construction **********

    //The reduced quadtree is build up subtree by subtree (the lists LE, ME the
    //centers, D1, D2, M, quad_tree_leaves are not calculated here.
    void  build_up_red_quad_tree_subtree_by_subtree(const Graph & G,
            NodeArray<NodeAttributes> & A,
            QuadTreeNM & T);

    //The root node of T is constructed and contained_nodes is set to the list of
    //all nodes of G.
    void build_up_root_vertex(const Graph & G, QuadTreeNM & T);

    //The reduced subtree of T rooted at *subtree_root_ptr containing all the particles
    //of subtree_root_ptr->get_contained_nodes() is constructed; Pointers to leaves
    //of the subtree that contain more than particles_in_leaves() particles in their
    //contained_nodes() lists are added to new_subtree_root_List_ptr; The lists
    //contained_nodes() are nonempty only for the (actual) leaves of T.
    void construct_subtree(NodeArray<NodeAttributes> & A,
                           QuadTreeNM & T,
                           QuadTreeNodeNM* subtree_root_ptr,
                           List<QuadTreeNodeNM*> & new_subtree_root_List);

    //A complete subtree of T and of depth subtree_depth, rooted at *T.get_act_ptr() is
    //constructed. Furthermore leaf_ptr[i][j] points to a leaf node of the subtree
    //that represents the quadratic subregion of *T.get_act_ptr() at subtree_depth
    //and position [i][j] i,j in 0,...,maxindex;act_depth(x_index,y_index) are
    //helping variables for recursive calls.
    void construct_complete_subtree(QuadTreeNM & T,
                                    int subtree_depth,
                                    Array2D<QuadTreeNodeNM*> & leaf_ptr,
                                    int act_depth,
                                    int act_x_index,
                                    int act_y_index);

    //The particles in subtree_root_ptr->get_contained_nodes() are assigned to
    //the the contained_nodes lists of the leaves of the subtree by using the
    //information of A,leaf_ptr and maxindex. Afterwards contained_nodes of
    // *subtree_root_ptr is empty.
    void set_contained_nodes_for_leaves(NodeArray<NodeAttributes> & A,
                                        QuadTreeNodeNM* subtree_root_ptr,
                                        Array2D<QuadTreeNodeNM*> & leaf_ptr,
                                        int maxindex);

    //The subtree of T rooted at *T.get_act_ptr() is traversed bottom up, such that
    //the subtreeparticlenumber of every node in this subtree is set correctly.
    void set_particlenumber_in_subtree_entries(QuadTreeNM & T);

    //The reduced subtree rooted at *T.get_act_ptr() is calculated ; A pointer to
    //every leaf of this subtree that contains more then particles_in_leaves()
    //particles is added to new_subtree_root_List; The lists contained_nodes are
    //empty for all but the leaves.
    void construct_reduced_subtree(NodeArray<NodeAttributes> & A,
                                   QuadTreeNM & T,
                                   List<QuadTreeNodeNM*> & new_subtree_root_List);

    //All subtrees of *T.get_act_ptr() that have a child c of *T.get_act_ptr() as root
    //and c.get_particlenumber_in_subtree() == 0 are deleted.
    void delete_empty_subtrees(QuadTreeNM & T);

    //If *T.get_act_ptr() is a degenerated node (has only one child c) *T.get_act_ptr()
    //is deleted from T and the child c is linked with the father of *T.get_act_ptr()
    //if *T.get_act_ptr() is the root of T than c is set to the new root of T
    //T.get_act_ptr() points to c afterwards; Furthermore true is returned if
    // *T.get_act_ptr() has been degenerated, else false is returned.
    bool check_and_delete_degenerated_node(QuadTreeNM & T);

    //The subtree rooted at new_leaf_ptr is deleted, *new_leaf_ptr is a leaf
    //of T and new_leaf_ptr->get_contained_nodes() contains all the particles
    //contained in the leaves of the deleted subtree; Precondition: T.get_act_ptr() is
    //new_leaf_ptr.
    void delete_sparse_subtree(QuadTreeNM & T, QuadTreeNodeNM* new_leaf_ptr);

    //new_leaf_ptr->get_contained_nodes() contains all the particles contained in
    //the leaves of its subtree afterwards; Precondition: T.get_act_ptr() is
    //new_leaf_ptr
    void collect_contained_nodes(QuadTreeNM & T, QuadTreeNodeNM* new_leaf_ptr);

    //If all nodes in T.get_act_ptr()->get_contained_nodes() have the same position
    //false is returned. Else true is returned and
    //the boxlength, down_left_corner and level of *T.get_act_ptr() is updated
    //such that this values are minimal (i.e. the smallest quad that contains all
    //the particles of T.get_act_ptr()->get_contained_nodes(); If all this particles
    //are placed at a point nothing is done.
    bool find_smallest_quad(NodeArray<NodeAttributes> & A, QuadTreeNM & T);

    // *********functions needed for subtree by subtree tree construction(end) ********

    //Finds the small cell of the actual Node of T iteratively,and updates
    //Sm_downleftcorner, Sm_boxlength, and level of *act_ptr.
    void find_small_cell_iteratively(QuadTreeNodeNM* act_ptr,
                                     double x_min,
                                     double x_max,
                                     double y_min,
                                     double y_max);

    //Finds the small cell of the actual Node of T by Aluru's Formula, and updates
    //Sm_downleftcorner, Sm_boxlength, and level of *act_ptr.
    void find_small_cell_by_formula(QuadTreeNodeNM* act_ptr,
                                    double x_min,
                                    double x_max,
                                    double y_min,
                                    double y_max);

    //The reduced quad tree is deleted; Furthermore the treenode_number is calculated.
    void delete_red_quad_tree_and_count_treenodes(QuadTreeNM & T);

    //The multipole expansion terms ME are calculated for all nodes of T ( centers are
    //initialized for each cell and quad_tree_leaves stores pointers to leaves of T).
    void form_multipole_expansions(NodeArray<NodeAttributes> & A,
                                   QuadTreeNM & T,
                                   List<QuadTreeNodeNM*> & quad_tree_leaves);

    //The multipole expansion List ME for the tree rooted at T.get_act_ptr() is
    //recursively calculated.
    void form_multipole_expansion_of_subtree(NodeArray<NodeAttributes> & A,
            QuadTreeNM & T,
            List<QuadTreeNodeNM*> & quad_tree_leaves);

    //The Lists ME and LE are both initialized to zero entries for *act_ptr.
    void init_expansion_Lists(QuadTreeNodeNM* act_ptr);

    //The center of the box of *act_ptr is initialized.
    void set_center(QuadTreeNodeNM* act_ptr);

    //Calculate List ME for *act_ptr Precondition: *act_ptr is a leaf.
    void form_multipole_expansion_of_leaf_node(NodeArray<NodeAttributes> & A,
            QuadTreeNodeNM* act_ptr);

    //Add the shifted ME Lists of *act_ptr to act_ptr->get_father_ptr() ; precondition
    // *act_ptr has a father_node.
    void add_shifted_expansion_to_father_expansion(QuadTreeNodeNM* act_ptr);

    //According to NMM T is traversed recursively top-down starting from act_node_ptr
    //== T.get_root_ptr() and thereby the lists D1, D2, M and LE are calculated for all
    //treenodes.
    void calculate_local_expansions_and_WSPRLS(NodeArray<NodeAttributes> & A,
            QuadTreeNodeNM* act_node_ptr);

    //If the small cell of ptr_1 and ptr_2 are well separated true is returned (else
    //false).
    bool well_separated(QuadTreeNodeNM* ptr_1, QuadTreeNodeNM* ptr_2);

    //If ptr_1 and ptr_2 are nonequal and bordering true is returned; else false.
    bool bordering(QuadTreeNodeNM* ptr_1, QuadTreeNodeNM* ptr_2);

    //The shifted local expansion of the father of node_ptr is added to the local
    //expansion of node_ptr;precondition: node_ptr is not the root of T.
    void add_shifted_local_exp_of_parent(QuadTreeNodeNM* node_ptr);

    //The multipole expansion of *ptr_1 is transformed into a local expansion around
    //the center of *ptr_2 and added to *ptr_2 s local expansion list.
    void add_local_expansion(QuadTreeNodeNM* ptr_1, QuadTreeNodeNM* ptr_2);

    //The multipole expansion for every particle of leaf_ptr->contained_nodes
    //(1,0,...) is transformed into a local expansion around the center of *ptr_2 and
    //added to *ptr_2 s local expansion List;precondition: *leaf_ptr is a leaf.
    void add_local_expansion_of_leaf(NodeArray<NodeAttributes> & A,
                                     QuadTreeNodeNM* leaf_ptr,
                                     QuadTreeNodeNM* act_ptr);

    //For each leaf v in quad_tree_leaves the force contribution defined by
    //v.get_local_exp() is calculated and stored in F_local_exp.
    void transform_local_exp_to_forces(NodeArray <NodeAttributes> & A,
                                       List<QuadTreeNodeNM*> & quad_tree_leaves,
                                       NodeArray<DPoint> & F_local_exp);

    //For each leaf v in quad_tree_leaves the force contribution defined by all nodes
    //in v.get_M() is calculated and stored in F_multipole_exp.
    void transform_multipole_exp_to_forces(NodeArray<NodeAttributes> & A,
                                           List<QuadTreeNodeNM*> & quad_tree_leaves,
                                           NodeArray<DPoint> & F_multipole_exp);

    //For each leaf v in quad_tree_leaves the force contributions from all leaves in
    //v.get_D1() and v.get_D2() are calculated.
    void calculate_neighbourcell_forces(NodeArray<NodeAttributes> & A,
                                        List<QuadTreeNodeNM*> & quad_tree_leaves,
                                        NodeArray<DPoint> & F_direct);

    //Add repulsive force contributions for each node.
    void add_rep_forces(const Graph & G,
                        NodeArray<DPoint> & F_direct,
                        NodeArray<DPoint> & F_multipole_exp,
                        NodeArray<DPoint> & F_local_exp,
                        NodeArray<DPoint> & F_rep);

    //Returns the repulsing force_function_value of scalar d.
    double f_rep_scalar(double d);

    //Init BK -matrix for values n, k in 0 to t.
    void init_binko(int t);

    //Free space for BK.
    void free_binko();

    //Returns n over k.
    double binko(int n, int k);

    //The way to construct the reduced tree (0) = level by level (1) path by path
    //(2) subtree by subtree
    int tree_construction_way() const
    {
        return _tree_construction_way;
    }

    void tree_construction_way(int a)
    {
        _tree_construction_way = (((0 <= a) && (a <= 2)) ? a : 0);
    }

    //(0) means that the smallest quadratic cell that surrounds a node of the
    //quadtree is calculated iteratively in constant time (1) means that it is
    //calculated by the formula of Aluru et al. in constant time
    int find_sm_cell() const
    {
        return _find_small_cell;
    }

    void find_sm_cell(int a)
    {
        _find_small_cell = (((0 <= a) && (a <= 1)) ? a : 0);
    }

    //Max. number of particles that are contained in a leaf of the red. quadtree.
    void particles_in_leaves(int b)
    {
        _particles_in_leaves = ((b >= 1) ? b : 1);
    }
    int particles_in_leaves() const
    {
        return _particles_in_leaves;
    }

    //The precision p for the p-term multipole expansions.
    void precision(int p)
    {
        _precision  = ((p >= 1) ? p : 1);
    }
    int  precision() const
    {
        return _precision;
    }
};

}//namespace ogdf
#endif

