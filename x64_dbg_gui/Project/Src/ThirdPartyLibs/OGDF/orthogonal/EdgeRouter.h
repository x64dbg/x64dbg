/*
 * $Revision: 2573 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-10 18:48:33 +0200 (Di, 10. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of EdgeRouter...
 *
 * ... which places node boxes in replacement areas of an orthogonal
 * drawing step and routes edges to minimize bends.
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

#ifndef OGDF_EDGE_ROUTER_H
#define OGDF_EDGE_ROUTER_H



#include <ogdf/internal/orthogonal/RoutingChannel.h>
#include <ogdf/orthogonal/MinimumEdgeDistances.h>
#include <ogdf/orthogonal/OrthoLayout.h>
#include <ogdf/basic/Layout.h>
#include <ogdf/basic/GridLayout.h>
#include <ogdf/basic/GridLayoutMapped.h>
#include <ogdf/planarity/PlanRep.h>
#include <ogdf/orthogonal/OrthoRep.h>
#include <ogdf/internal/orthogonal/NodeInfo.h>

namespace ogdf
{

//! edge types, defined by necessary bends
enum bend_type
{
    bend_free,
    bend_1left,
    bend_1right,
    bend_2left,
    bend_2right, //results
    prob_bf,
    prob_b1l,
    prob_b1r,
    prob_b2l,
    prob_b2r
}; //and preliminary


// unprocessed, processed in degree1 preprocessing, used by degree1
enum process_type
{
    unprocessed,
    processed,
    used
};


/**
 * Places node boxes in replacement areas of orthogonal
 * drawing step and route edges to minimize bends
 */
class OGDF_EXPORT EdgeRouter
{
public:
    //constructor
    EdgeRouter() { }

    EdgeRouter(
        PlanRep & pru,
        OrthoRep & H,
        GridLayoutMapped & L,
        CombinatorialEmbedding & E,
        RoutingChannel<int> & rou,
        MinimumEdgeDistances<int> & med,
        NodeArray<int> & nodewidth,
        NodeArray<int> & nodeheight);

    virtual ~EdgeRouter() { }

    void init(
        PlanRep & pru,
        RoutingChannel<int> & rou,
        bool align = false);

    //! sets the computed distances in structure MinimumEdgeDistance m_med
    void setDistances();

    //! places nodes in cages and routes incident edges
    void call();

    //! places nodes in cages and routes incident edges
    void call(
        PlanRep & pru,
        OrthoRep & H,
        GridLayoutMapped & L,
        CombinatorialEmbedding & E,
        RoutingChannel<int> & rou,
        MinimumEdgeDistances<int> & med,
        NodeArray<int> & nodewidth,
        NodeArray<int> & nodeheight,
        bool align = false);

    //! applies precomputed placement
    void place(node v/*, int l_sep, int l_overh*/);

    //! computes placement
    void compute_place(node v, NodeInfo & inf/*, int sep = 10.0, int overh = 2*/);

    //! computes routing after compute_place
    void compute_routing(node v);

    //! compute glue points positions
    void compute_glue_points_y(node v);

    //! compute glue points positions
    void compute_gen_glue_points_y(node v);

    //! compute glue points positions
    void compute_glue_points_x(node & v);

    //! compute glue points positions
    void compute_gen_glue_points_x(node v);

    //! sets values derivable from input
    void initialize_node_info(node v, int sep);

    // returns assigned connection point and glue point coordinates for each edge
    int cp_x(adjEntry ae)
    {
        return m_acp_x[ae];    //!< connection point (cage border) coord of source
    }
    int cp_y(adjEntry ae)
    {
        return m_acp_y[ae];    //!< connection point (cage border) coord of source
    }
    int gp_x(adjEntry ae)
    {
        return m_agp_x[ae];    //!< glue point (node border)
    }
    int gp_y(adjEntry ae)
    {
        return m_agp_y[ae];    //!< glue point (node border)
    }

    bend_type abendType(adjEntry ae)
    {
        return m_abends[ae];
    }

    void addbends(BendString & bs, const char* s2);

    edge addRightBend(edge e);

    edge addLeftBend(edge e);

    //! adjEntries for edges in inLists
    adjEntry outEntry(NodeInfo & inf, OrthoDir d, int pos)
    {
        if(inf.is_in_edge(d, pos))
            return (*inf.inList(d).get(pos))->adjTarget();
        else
            return (*inf.inList(d).get(pos))->adjSource();//we only bend on outentries
    }

    //! adjEntries for edges in inLists
    adjEntry inEntry(NodeInfo & inf, OrthoDir d, int pos)
    {
        if(inf.is_in_edge(d, pos))
            return (*inf.inList(d).get(pos))->adjSource();
        else
            return (*inf.inList(d).get(pos))->adjTarget();
    }

    //! sets position for node v in layout to value x,y, invoked to have central control over change
    void set_position(node v, int x = 0, int y = 0);

    //! same as set but updates m_fixed, coordinates cant be changed afterwards
    void fix_position(node v, int x = 0, int y = 0);

    //! for all multiple edges, set the delta value on both sides to minimum if not m_minDelta
    /**
    * postprocessing function, hmm maybe preprocessing
    */
    void multiDelta();

    //! set alignment option: place nodes in cage at outgoing generalization
    /**
    * postprocessing function, hmm maybe preprocessing
    */
    void align(bool b)
    {
        m_align = b;
    }

    //test fuer skalierende Kompaktierung
    //void setOrSep(int sep)
    //{m_hasOrSep = true; m_orSep = sep;}


private:
    PlanRep* m_prup;
    GridLayoutMapped* m_layoutp;
    OrthoRep* m_orp;
    CombinatorialEmbedding* m_comb;
    RoutingChannel<int>* m_rc;
    MinimumEdgeDistances<int>* m_med;
    NodeArray<int>* m_nodewidth;
    NodeArray<int>* m_nodeheight;

    NodeArray<NodeInfo> infos; //!< holds the cage and placement information

    int    m_sep;   //!< minimum separation
    int    m_overh; //!< minimum overhang
    double Cconst;  //!< relative sep to overhang / delta to eps

    void unsplit(edge e1, edge e2);

    //! set coordinates of cage corners after placement
    void set_corners(node v);

    //! computes the alpha value described in the paper
    int alpha_move(OrthoDir s_to, OrthoDir s_from, node v);

    //! set minimum delta values for flip decision and adjust distances correspondingly
    bool m_minDelta;

    //! helper for oppositeExpander
    node oppositeNode(adjEntry ae)
    {
        return ae->twinNode();
    }

    //! check if the target node of the outgoing adjEntry still is a expander
    bool oppositeExpander(adjEntry ae)
    {
        Graph::NodeType nt;
        nt = m_prup->typeOf(oppositeNode(ae));
        return ((nt == Graph::highDegreeExpander) || (nt == Graph::lowDegreeExpander));
    }
    //if yes, set its m_oppositeBendType value according to the newly introduced bend

    //! computes the beta value described in the paper
    /**
     * number of additional bend free edges on side s_from
     * if move_num edges are moved from side s_from to s_to
     */
    int beta_move(OrthoDir s_from, OrthoDir s_to, int move_num, node v);

    //! compute the maximum number of moveable edges
    /**
     * dependant on space on available edges, return number of saved bends
     */
    int compute_move(OrthoDir s_from, OrthoDir s_to, int & kflip, node v);

    NodeArray<int> m_newx, m_newy; //!< new placement position for original node
    //!< node array saves info about changed position, no further change is allowed
    NodeArray<bool> m_fixed;
    EdgeArray<int>  lowe, uppe, lefte, righte; //!< max box borders for bendfree edges
    AdjEntryArray<int>  alowe, auppe, alefte, arighte;
    AdjEntryArray<int>  m_agp_x, m_agp_y; //!< because edges can connect two replacement cages
    AdjEntryArray<node> m_cage_point; //!< newly introduced bends destroy edge to point connection
    AdjEntryArray<int>  m_acp_x, m_acp_y;//!< edge connection point coordinates before treatment

    //! bends
    /**
     * 0 = bendfree, 1 = single bend from left to node,
     * 2 = single from right, 3 = int from left,
     * 4 = int from right,...
     */
    AdjEntryArray<bend_type> m_abends;

    //! keep the information about the type of bend inserted at one end of an (originally unbend) edge, so that we can check possible bendsaving
    NodeArray<bend_type> m_oppositeBendType;

    //! keep information about already processed Nodes
    NodeArray<process_type> m_processStatus;

    //alignment test
    NodeArray<bool> m_mergerSon; //!< is part of merger son cage
    NodeArray<OrthoDir> m_mergeDir; //!< direction of adjacent (to) merger edges
    bool m_align;
};

} //end namespace

#endif
