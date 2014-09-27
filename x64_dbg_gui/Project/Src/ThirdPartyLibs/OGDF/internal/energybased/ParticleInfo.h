/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class ParticleInfo.
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

#ifndef OGDF_PARTICLE_INFO_H
#define OGDF_PARTICLE_INFO_H

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/List.h>

namespace ogdf
{

class OGDF_EXPORT ParticleInfo
{
    //Helping data structure for building up the reduced quad tree by NMM.

    //Outputstream for ParticleInfo.
    friend ostream & operator<< (ostream & output, const ParticleInfo & A)
    {
        output << " node_index " << A.vertex->index() << " x_y_coord " << A.x_y_coord;
        if(A.marked == true)
            output << " marked ";
        else
            output << " unmarked ";
        output << " sublist_ptr ";
        if(A.subList_ptr == NULL)
            output << "NULL";
        else
            output << A.subList_ptr;
        return output;
    }

    //inputstream for ParticleInfo
    friend istream & operator>> (istream & input,  ParticleInfo & A)
    {
        input >> A;
        return input;
    }

public:

    ParticleInfo()    //constructor
    {
        vertex = NULL;
        x_y_coord = 0;
        cross_ref_item = NULL;
        copy_item = NULL;
        subList_ptr = NULL;
        marked = false;
        tmp_item = NULL;
    }

    ~ParticleInfo() { }   //destructor

    void set_vertex(node v)
    {
        vertex = v;
    }
    void set_x_y_coord(double c)
    {
        x_y_coord = c;
    }
    void set_cross_ref_item(ListIterator<ParticleInfo> it)
    {
        cross_ref_item = it;
    }
    void set_subList_ptr(List<ParticleInfo>* ptr)
    {
        subList_ptr = ptr;
    }
    void set_copy_item(ListIterator<ParticleInfo> it)
    {
        copy_item = it;
    }
    void mark()
    {
        marked = true;
    }
    void unmark()
    {
        marked = false;
    }
    void set_tmp_cross_ref_item(ListIterator<ParticleInfo> it)
    {
        tmp_item = it;
    }

    node get_vertex() const
    {
        return vertex;
    }
    double get_x_y_coord() const
    {
        return x_y_coord;
    }
    ListIterator<ParticleInfo> get_cross_ref_item() const
    {
        return cross_ref_item;
    }
    List<ParticleInfo>* get_subList_ptr() const
    {
        return subList_ptr;
    }
    ListIterator<ParticleInfo> get_copy_item() const
    {
        return copy_item;
    }
    bool is_marked() const
    {
        return marked;
    }
    ListIterator<ParticleInfo> get_tmp_cross_ref_item() const
    {
        return tmp_item;
    }

private:
    node vertex;      //the vertex of G that is associated with this attributes
    double x_y_coord; //the x (resp. y) coordinate of the actual position of the vertex
    ListIterator<ParticleInfo> cross_ref_item;  //the Listiterator of the
    //ParticleInfo-Element that
    //containes the vertex in the List storing the other
    //coordinates (a cross reference)
    List<ParticleInfo>*  subList_ptr;   //points to the subList of L_x(L_y) where the
    //actual entry of ParticleInfo has to be stored
    ListIterator<ParticleInfo>  copy_item;  //the item of this entry in the copy List
    bool marked; //indicates if this ParticleInfo object is marked or not
    ListIterator<ParticleInfo> tmp_item;    //a temporily item that is used to construct
    //the cross references for the copy_Lists
    //and the subLists
};


//Needed for sorting algorithms in ogdf/List and ogdf/Array.
class ParticleInfoComparer
{
public:
    //Returns -1(1) if height of a <(>) height of b. If they are equal 0 is
    //returned.
    static int compare(const ParticleInfo & a, const ParticleInfo & b)
    {
        double p = a.get_x_y_coord();
        double q = b.get_x_y_coord();
        if(p < q) return  -1;
        else if(p > q) return 1;
        else return 0;
    }
    OGDF_AUGMENT_STATICCOMPARER(ParticleInfo)
};

}//namespace ogdf

#endif
