/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class NodeInfo.
 *
 * The class NodeInfo holds the information that is necessary for
 * the rerouting of the edges after the constructive compaction step
 * the rerouting works on a PlanRep and derives the info in member
 * get_data.
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

#ifndef OGDF_NODEINFO_H
#define OGDF_NODEINFO_H

#include <ogdf/internal/orthogonal/RoutingChannel.h>
#include <ogdf/orthogonal/MinimumEdgeDistances.h>
#include <ogdf/basic/AdjEntryArray.h>
#include <ogdf/orthogonal/OrthoRep.h>
#include <ogdf/planarity/PlanRep.h>
#include <ogdf/basic/GridLayout.h>

namespace ogdf
{

class OGDF_EXPORT NodeInfo
{
public:
    //standard constr.
    NodeInfo()
    {
        init();
    }

    void init()
    {
        for(int i = 0; i < 4; i++)
        {
            for(int j = 0; j < 4; j++)
            {
                m_nbe[i][j] = 0;
                m_delta[i][j] = 0;
                m_eps[i][j] = 0;
                m_routable[i][j] = 0;
                m_flips[i][j] = 0;
            }
            num_s_edges[i] = 0;
            m_gen_pos[i] = -1;
            m_nbf[i] = 0;
            m_coord[i] = 0;
            m_ccoord[i] = 0;
        }
        lu = ll = ru = rl = tl = tr = bl = br = 0;
    }//init

    //Constructor, adj holds entry for inner face edge
    NodeInfo(
        OrthoRep & H,
        GridLayout & L,
        node v,
        adjEntry adj,
        RoutingChannel<int> & rc,
        NodeArray<int> & nw,
        NodeArray<int> & nh) : m_adj(adj)
    {
        init();
        get_data(H, L, v, rc, nw, nh);
    }

    virtual ~NodeInfo() { }

    adjEntry cage_entry()
    {
        return m_adj;
    }

    //get
    int coord(OrthoDir bs) const
    {
        return m_coord[bs];    //nodeboxside coordinates (real size)
    }
    int cage_coord(OrthoDir bs) const
    {
        return m_ccoord[bs];    //nodecageside coordinates (expanded size)
    }
    int cageCoord(OrthoDir bs) const
    {
        return m_ccoord[bs];    //nodecageside coordinates (expanded size)
    }

    //return distance between Node and  Cage coord
    int coordDistance(OrthoDir bs)
    {
        int result;
        switch(bs)
        {
        case odSouth:
        case odEast:
            result = m_ccoord[bs] - m_coord[bs];
            break;
        case odNorth:
        case odWest:
            result =  m_coord[bs] - m_ccoord[bs];
            break;
        default:
            cout << "unknown direction in coordDistance" << flush;
            OGDF_THROW(AlgorithmFailureException);
        }//switch
        OGDF_ASSERT(result >= 0);
        return result;
    }//coordDistance

    //returns side coord respecting already flipped edges
    int free_coord(OrthoDir s_main, OrthoDir s_to);

    int node_xsize() const
    {
        return box_x_size;    //original box sizes, fake
    }
    int node_ysize() const
    {
        return box_y_size;
    }

    int nodeSize(OrthoDir od) const
    {
        return ((od % 2 == 0) ? box_y_size : box_x_size);
    }
    int cageSize(OrthoDir od) const
    {
        return ((od % 2 == 0) ? cage_y_size : cage_x_size);
    }
    int rc(OrthoDir od) const
    {
        return m_rc[od];    //routing channel size
    }

    List<edge> & inList(OrthoDir bs)
    {
        return in_edges[bs];
    }
    List<bool> & inPoint(OrthoDir bs)
    {
        return point_in[bs];
    }

    //these values are computed dependant on the nodes placement
    int l_upper_unbend()
    {
        return lu;    //position of first and last unbend edge on every side
    }
    int l_lower_unbend()
    {
        return ll;
    }
    int r_upper_unbend()
    {
        return ru;
    }
    int r_lower_unbend()
    {
        return rl;
    }
    int t_left_unbend()
    {
        return tl;
    }
    int t_right_unbend()
    {
        return tr;
    }
    int b_left_unbend()
    {
        return bl;
    }
    int b_right_unbend()
    {
        return br;
    }

    //object separation distances
    //if (no) generalization enters..., side/gener. dependant paper delta values
    //distance at side mainside, left/right from existing generalization to side neighbour
    int delta(OrthoDir mainside, OrthoDir neighbour) const
    {
        return m_delta[mainside][neighbour];
    }

    //paper epsilon
    int eps(OrthoDir mainside, OrthoDir neighbour) const
    {
        return m_eps[mainside][neighbour];
    }

    //cardinality of the set of edges that will bend, bside side to the side bneighbour
    int num_bend_edges(OrthoDir s1, OrthoDir sneighbour)
    {
        return m_nbe[s1][sneighbour];
    }
    int num_E_hook(OrthoDir s1, OrthoDir sneighbour)
    {
        return m_routable[s1][sneighbour];
    }
    //number of edges flipped from s1 to s2 to save one bend
    int & flips(OrthoDir s1, OrthoDir s2)
    {
        return m_flips[s1][s2];
    }

    int num_bend_free(OrthoDir s) const
    {
        return m_nbf[s];    //number of edges routed bendfree
    }
    int & nbf(OrthoDir s)
    {
        return m_nbf[s];
    }

    int num_edges(OrthoDir od) const
    {
        return num_s_edges[od]; //return number of edges at side od
    }

    //position of gen. edges in edge lists for every side, starting with 1
    int gen_pos(OrthoDir od) const
    {
        return m_gen_pos[od];
    }
    bool has_gen(OrthoDir od)
    {
        return m_gen_pos[od] > -1;
    }

    bool is_in_edge(OrthoDir od, int pos)
    {
        ListConstIterator<bool> b_it = point_in[od].get(pos);
        return *b_it;
    }

    //set
    void reclassify(OrthoDir) { }//set m_nbf, nb, m_routable based on bend_type values on s,
    //erst mal nur zum vergleichen benutzen ist soll
    void set_coord(OrthoDir bs, int co)
    {
        m_coord[bs] = co;
    }
    void set_cage_coord(OrthoDir bs, int co)
    {
        m_ccoord[bs] = co;
    }
    void setCageCoord(OrthoDir bs, int co)
    {
        m_ccoord[bs] = co;
    }

    //delta values, due to placement problems, cut to box_size / 2
    void set_delta(OrthoDir bside, OrthoDir bneighbour, int dval)
    {
        switch(bside)
        {
        case odNorth:
        case odSouth:
            if(dval > box_y_size)
            {
                dval = int(floor(((double)box_y_size / 2))) - m_eps[bside][bneighbour];
            }
            break;
        case odEast:
        case odWest:
            if(dval > box_x_size)
            {
                dval = int(floor(((double)box_x_size / 2))) - m_eps[bside][bneighbour];
            }
            break;
            OGDF_NODEFAULT
        }//switch
        m_delta[bside][bneighbour] = dval;
    }

    void set_eps(OrthoDir mainside, OrthoDir neighbour, int dval)
    {
        m_eps[mainside][neighbour] = dval;
    }

    //number of bending edges on one side at corner to second side
    //void set_num_bend_edges(box_side bs1, box_side bs2, int num) {nbe[bs1][bs2] = num;}
    //set position of generalization on each side
    void set_gen_pos(OrthoDir od, int pos)
    {
        m_gen_pos[od] = pos; //odir: N 0, E 1
    }
    void set_num_edges(OrthoDir od, int num)
    {
        num_s_edges[od] = num; //odir: N 0, E 1, check correct od parameter?
    }


    //computes the size of the cage face and the node box
    void compute_cage_size()
    {
        cage_x_size = m_ccoord[odSouth] - m_ccoord[odNorth];
        cage_y_size = m_ccoord[odEast] - m_ccoord[odWest];
    }
    //      int compute_rc(box_side b) {cout<<"rc not yet implemented\n";exit(1);}

    //set the unbend edges after (in) placement step
    void set_l_upper(int d)
    {
        lu = d;
    }
    void set_l_lower(int d)
    {
        ll = d;
    }
    void set_r_upper(int d)
    {
        ru = d;
    }
    void set_r_lower(int d)
    {
        rl = d;
    }
    void set_t_left(int d)
    {
        tl = d;
    }
    void set_t_right(int d)
    {
        tr = d;
    }
    void set_b_left(int d)
    {
        bl = d;
    }
    void set_b_right(int d)
    {
        br = d;
    }

    //paper set E_s1_s2
    void inc_E_hook(OrthoDir s_from, OrthoDir s_to, int num = 1)
    {
        m_routable[s_from][s_to] += num;
        m_nbe[s_from][s_to] += num;
    }
    void inc_E(OrthoDir s_from, OrthoDir s_to, int num = 1)
    {
        m_nbe[s_from][s_to] += num;
    }

    //read the information for node v from attributed graph/planrep
    //(needs positions ...)
    void get_data(
        OrthoRep & O,
        GridLayout & L,
        node v,
        RoutingChannel<int> & rc,
        NodeArray<int> & nw,
        NodeArray<int> & nh); //check input parameter
    void get_OR_data(node v, OrthoRep & O);
    //
    int num_routable(OrthoDir s_from, OrthoDir s_to) const
    {
        return m_routable[s_from][s_to];    //card. of paper E^_s1,s2
    }
    int & numr(OrthoDir s_from, OrthoDir s_to)
    {
        return m_routable[s_from][s_to];
    }
    int vDegree()
    {
        return m_vdegree;
    }
    adjEntry & firstAdj()
    {
        return m_firstAdj;
    }

    friend ostream & operator<<(ostream & O, const NodeInfo & inf);

private:
    int m_rc[4];
    int m_coord[4]; //coordinates of box segments, x for ls_left/right, y for s_top/bottom
    int m_ccoord[4]; //coordinates of expanded cage segments, -"-
    int cage_x_size, cage_y_size, //cage size
        box_x_size, box_y_size; //box size
    int lu, ll, ru, rl, tl, tr, bl, br; //first/last unbend edge on all sides
    //most of the following are only [4][2] but use 44 for users conv
    int m_delta[4][4]; //sepa. distance (paper delta)
    int m_eps[4][4]; //corner separation distance (paper epsilon)
    int m_gen_pos[4]; //pos num of generaliz. edge in adj lists
    int num_s_edges[4]; //number of edges at sides 0..3=N..W
    int m_routable[4][4]; //number of reroutable edges, paper E^_s1,s2, got to be initialized after box placement
    int m_flips[4][4]; //real number of flipped edges
    int m_nbe[4][4]; //paper E_s1,s2
    int m_nbf[4]; //number of bendfree edges per side
    adjEntry m_firstAdj; //adjEntry of first encountered outgoing edge, note: this is a copy

    List<edge> in_edges[4]; //inedges on each side will be replaced by dynamic ops
    //preliminary bugfix of in/out dilemma
    List<bool> point_in[4]; //save in/out info
    adjEntry m_adj; //entry of inner cage face
    //degree of expanded vertex
    int m_vdegree;
};


ostream & operator<<(ostream & O, const NodeInfo & inf);

} //end namespace

#endif
