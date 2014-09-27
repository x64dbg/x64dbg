/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class QuadTreeNodeNM.
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

#ifndef OGDF_QUAD_TREE_NODE_NM_H
#define OGDF_QUAD_TREE_NODE_NM_H


#include <ogdf/basic/Graph.h>
#include <ogdf/basic/List.h>
#include <ogdf/basic/geometry.h>
#include <ogdf/internal/energybased/ParticleInfo.h>
#include <complex>


using std::complex;

namespace ogdf
{

class QuadTreeNodeNM
{
    //Helping data structure that stores the information needed to represent
    //a node of the reduced quad tree in the New Multipole Method (NMM).

    //Outputstream for QuadTreeNodeNM.
    friend ostream & operator<< (ostream &, const QuadTreeNodeNM &);

    //Inputstream for QuadTreeNodeNM.
    friend istream & operator>> (istream &, QuadTreeNodeNM &);

public:

    QuadTreeNodeNM();     //constructor
    ~QuadTreeNodeNM();    //destructor

    void set_Sm_level(int l)
    {
        Sm_level = l;
    }
    void set_Sm_downleftcorner(DPoint dlc)
    {
        Sm_downleftcorner = dlc;
    }
    void set_Sm_boxlength(double l)
    {
        Sm_boxlength = l;
    }
    void set_x_List_ptr(List<ParticleInfo>* x_ptr)
    {
        L_x_ptr = x_ptr;
    }
    void set_y_List_ptr(List<ParticleInfo>* y_ptr)
    {
        L_y_ptr = y_ptr;
    }
    void set_particlenumber_in_subtree(int p)
    {
        subtreeparticlenumber = p;
    }
    void set_Sm_center(complex<double> c)
    {
        Sm_center = c;
    }
    void set_contained_nodes(List<node> & L)
    {
        contained_nodes = L;
    }
    void pushBack_contained_nodes(node v)
    {
        contained_nodes.pushBack(v);
    }
    node pop_contained_nodes()
    {
        return contained_nodes.popFrontRet();
    }
    bool contained_nodes_empty()
    {
        return contained_nodes.empty();
    }

    void set_I(List<QuadTreeNodeNM*> & l)
    {
        I = l;
    }
    void set_D1(List<QuadTreeNodeNM*> & l)
    {
        D1 = l;
    }
    void set_D2(List<QuadTreeNodeNM*> & l)
    {
        D2 = l;
    }
    void set_M(List<QuadTreeNodeNM*> & l)
    {
        M = l;
    }

    //LE[i] is set to local[i] for i = 0 to precision and space for LE is reserved.
    void set_locale_exp(Array<complex<double> > & local, int precision)
    {
        int i;
        LE = new complex<double> [precision + 1];
        for(i = 0 ; i <= precision; i++)
            LE[i] = local[i];
    }

    //ME[i] is set to multi[i] for i = 0 to precision and space for LE is reserved.
    void set_multipole_exp(Array<complex<double> > & multi, int precision)
    {
        int i;
        ME = new complex<double> [precision + 1];
        for(i = 0 ; i <= precision; i++)
            ME[i] = multi[i];
    }

    //ME[i] is set to multi[i] for i = 0 to precision and no space for LE is reserved.
    void replace_multipole_exp(Array<complex<double> > & multi, int precision)
    {
        int i;
        for(i = 0 ; i <= precision; i++)
            ME[i] = multi[i];
    }

    void set_father_ptr(QuadTreeNodeNM* f)
    {
        father_ptr = f;
    }
    void set_child_lt_ptr(QuadTreeNodeNM* c)
    {
        child_lt_ptr = c;
    }
    void set_child_rt_ptr(QuadTreeNodeNM* c)
    {
        child_rt_ptr = c;
    }
    void set_child_lb_ptr(QuadTreeNodeNM* c)
    {
        child_lb_ptr = c;
    }
    void set_child_rb_ptr(QuadTreeNodeNM* c)
    {
        child_rb_ptr = c;
    }

    bool is_root()
    {
        if(father_ptr == NULL) return true;
        else return false;
    }
    bool is_leaf()
    {
        if((child_lt_ptr == NULL) && (child_rt_ptr == NULL) && (child_lb_ptr
                == NULL) && (child_rb_ptr == NULL))
            return true;
        else return false;
    }
    bool child_lt_exists()
    {
        if(child_lt_ptr != NULL) return true;
        else return false;
    }
    bool child_rt_exists()
    {
        if(child_rt_ptr != NULL) return true;
        else return false;
    }
    bool child_lb_exists()
    {
        if(child_lb_ptr != NULL) return true;
        else return false;
    }
    bool child_rb_exists()
    {
        if(child_rb_ptr != NULL) return true;
        else return false;
    }

    int get_Sm_level() const
    {
        return Sm_level;
    }
    DPoint get_Sm_downleftcorner() const
    {
        return Sm_downleftcorner;
    }
    double get_Sm_boxlength() const
    {
        return Sm_boxlength;
    }
    List<ParticleInfo>*  get_x_List_ptr()
    {
        return L_x_ptr;
    }
    List<ParticleInfo>*  get_y_List_ptr()
    {
        return L_y_ptr;
    }
    int get_particlenumber_in_subtree()const
    {
        return subtreeparticlenumber;
    }
    complex<double> get_Sm_center() const
    {
        return Sm_center;
    }
    complex<double>* get_local_exp() const
    {
        return LE;
    }
    complex<double>* get_multipole_exp() const
    {
        return ME;
    }
    void get_contained_nodes(List<node> & L) const
    {
        L =  contained_nodes;
    }
    void get_I(List <QuadTreeNodeNM*> & l)
    {
        l = I;
    }
    void get_D1(List <QuadTreeNodeNM*> & l)
    {
        l = D1;
    }
    void get_D2(List <QuadTreeNodeNM*> & l)
    {
        l = D2;
    }
    void get_M(List <QuadTreeNodeNM*> & l)
    {
        l = M;
    }

    QuadTreeNodeNM* get_father_ptr()   const
    {
        return father_ptr;
    }
    QuadTreeNodeNM* get_child_lt_ptr() const
    {
        return child_lt_ptr;
    }
    QuadTreeNodeNM* get_child_rt_ptr() const
    {
        return child_rt_ptr;
    }
    QuadTreeNodeNM* get_child_lb_ptr() const
    {
        return child_lb_ptr;
    }
    QuadTreeNodeNM* get_child_rb_ptr() const
    {
        return child_rb_ptr;
    }

private:

    int  Sm_level;                     //level of the small cell
    DPoint Sm_downleftcorner;          //coords of the down left corner of the small cell
    double Sm_boxlength;               //length of small cell
    List<ParticleInfo>* L_x_ptr;       //points to the lists that contain each Particle
    //of G with its x(y)coordinate in increasing order
    List<ParticleInfo>* L_y_ptr;       //and a cross reference to the list_item in the
    //list  with the other coordinate
    int subtreeparticlenumber;         //the number of particles in the subtree rooted
    //at this node
    complex<double>  Sm_center;        //center of the small cell
    complex<double>* ME;               //Multipole Expansion terms
    complex<double>* LE;               //Locale Expansion terms
    List <node>  contained_nodes;      //list of nodes of G that are contained in this
    //QuadTreeNode  (emty if it is not a leave of
    //the ModQuadTree
    List <QuadTreeNodeNM*> I;          //the list of min. ill sep. nodes in DIM2
    List <QuadTreeNodeNM*> D1, D2;     //list of neighbouring(=D1) and not adjacent(=D2)
    //leaves for direct force calculation in DIM2
    List<QuadTreeNodeNM*>  M;          //list of nodes with multipole force contribution
    //like in DIM2
    QuadTreeNodeNM*  father_ptr;   //points to the father node
    QuadTreeNodeNM*  child_lt_ptr; //points to left top child
    QuadTreeNodeNM*  child_rt_ptr; //points to right bottom child
    QuadTreeNodeNM*  child_lb_ptr; //points to left bottom child
    QuadTreeNodeNM*  child_rb_ptr; //points to right bottom child
};

}//namespace ogdf

#endif
