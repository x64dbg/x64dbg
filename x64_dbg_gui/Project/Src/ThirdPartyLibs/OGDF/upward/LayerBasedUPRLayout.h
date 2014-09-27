/*
 * $Revision: 2524 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-03 09:54:22 +0200 (Tue, 03 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of upward planarization layout algorithm.
 *
 * \author Hoi-Ming Wong
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

#ifndef OGDF_LAYER_BASED_UPR_LAYOUT_H
#define OGDF_LAYER_BASED_UPR_LAYOUT_H



#include <ogdf/basic/ModuleOption.h>
#include <ogdf/upward/UpwardPlanRep.h>
#include <ogdf/module/RankingModule.h>
#include <ogdf/module/UPRLayoutModule.h>
#include <ogdf/module/HierarchyLayoutModule.h>
#include <ogdf/layered/OptimalHierarchyLayout.h>
#include <ogdf/layered/FastHierarchyLayout.h>
#include <ogdf/layered/OptimalRanking.h>


namespace ogdf
{

class OrderComparer
{
public:
    OrderComparer(const UpwardPlanRep & _UPR, Hierarchy & _H);

    // if vH1 and vH2 are placed on the same layer and node vH1 has to drawn on the lefthand side of vH2 (according to UPR) then return true;
    bool less(node vH1, node vH2) const ;

private:
    const UpwardPlanRep & UPR;
    Hierarchy & H;
    NodeArray<int> dfsNum;
    //EdgeArray<int> outEdgeOrder;
    mutable NodeArray<bool> crossed;

    //traverse with dfs using edge order from left to right and compute the dfs number.
    void dfs_LR(edge e,
                NodeArray<bool> & visited,
                NodeArray<int> & dfsNum,
                int & num);

    //return true if vUPR1 is on the lefthand side of vUPR2 according to UPR.
    bool left(node vUPR1,
              List<edge> chain1, //if vUPR1 is associated with a long edge dummy vH1, then chain1 contain vH1
              node vUPR2 ,
              List<edge> chain2 // if vUPR2 is associated with a long edge dummy vH2, then chain2 contain vH2
             ) const;

    //return true if vUPR1 is on the lefthand side of vUPR2 according to UPR.
    // pred.: source or target of both edge muss identical
    bool left(edge e1UPR, edge e2UPR) const;

    //return true if vUPR1 is on the lefthand side of vUPR2 according to UPR.
    // use only by method less for the case when both node vH1 and vH2 are long-edge dummies.
    // level: the current level of the long-edge dummies
    bool left(List<edge> & chain1, List<edge> & chain2, int level) const;

    //return true if there is a node above vUPR with rank level or lower
    bool checkUp(node vUPR, int level) const;
};


class OGDF_EXPORT LayerBasedUPRLayout : public UPRLayoutModule
{
public:

    // constructor: sets options to default values
    LayerBasedUPRLayout()
    {
        // set default value
        FastHierarchyLayout* fhl = new FastHierarchyLayout();
        fhl->nodeDistance(40.0);
        fhl->layerDistance(40.0);
        fhl->fixedLayerDistance(true);
        m_layout.set(fhl);
        OptimalRanking* opRank = new OptimalRanking();
        opRank->separateMultiEdges(false);
        m_ranking.set(opRank);
        m_numLevels = 0;
        m_maxLevelSize = 0;
    }

    // destructor
    ~LayerBasedUPRLayout() { }

    // returns the number of crossings in the layout after the algorithm
    // has been applied
    int numberOfCrossings() const
    {
        return m_crossings;
    }

    // module option for the computation of the final layout
    void setLayout(HierarchyLayoutModule* pLayout)
    {
        m_layout.set(pLayout);
    }


    void setRanking(RankingModule* pRanking)
    {
        m_ranking.set(pRanking);
    }

    //! Use only the 3. phase of Sugiyama' framework for layout.
    void UPRLayoutSimple(const UpwardPlanRep & UPR, GraphAttributes & AG);

    //! Return the number of layers/levels. Not implemented if use methode callSimple(..).
    int numberOfLayers()
    {
        return m_numLevels;
    }

    //! Return the max. number of elements on a layer. Not implemented if use methode callSimple(..).
    int maxLayerSize()
    {
        return m_maxLevelSize;
    }

protected :

    /*
     * @param UPR is the upward planarized representation of the input graph.
     * @param AG has to be assigned the hierarchy layout.
     */
    virtual void doCall(const UpwardPlanRep & UPR, GraphAttributes & AG);

    int m_crossings;

    ModuleOption<RankingModule> m_ranking;

    ModuleOption<HierarchyLayoutModule> m_layout;


    struct RankComparer
    {
        Hierarchy* H;
        bool less(node v1, node v2) const
        {
            return (H->rank(v1) < H->rank(v2));
        }
    };


private:

    // compute a ranking of the nodes of UPR.
    // Precond. a ranking module muss be set
    void computeRanking(const UpwardPlanRep & UPR, NodeArray<int> & rank);


    //! rearanging the position of the sources in order to reduce some crossings.
    void postProcessing_sourceReorder(Hierarchy & H, List<node> & sources);


    //! reduce the long edge dummies (LED)
    void postProcessing_reduceLED(Hierarchy & H, List<node> & sources)
    {
        forall_listiterators(node, it, sources)
        postProcessing_reduceLED(H, *it);
    }

    void postProcessing_reduceLED(Hierarchy & H, node vH);

    void post_processing_reduce(Hierarchy & H, int & i, node s, int minIdx, int maxIdx, NodeArray<bool> & markedNodes);

    //! mark all the nodes dominated by sH. (Help method for postProcessing_reduceLED() )
    void postProcessing_markUp(Hierarchy & H, node sH, NodeArray<bool> & markedNodes);


    //! delete level i of H.
    void post_processing_deleteLvl(Hierarchy & H, int i);

    //! delete the interval [beginIdx,endIdx] on the level j.
    void post_processing_deleteInterval(Hierarchy & H, int beginIdx, int endIdx, int & j);

    //! insert the interval  [beginIdx,endIdx] of level i-1 to level i at position pos.
    void post_processing_CopyInterval(Hierarchy & H, int i, int beginIdx, int endIdx, int pos);

    int m_numLevels;
    int m_maxLevelSize;



    //------------------------ UPRLayoutSimple methods --------------------------------------------
    void callSimple(GraphAttributes & AG, adjEntry adj //left most edge of the source
                   );

    // needed for UPRLayoutSimple
    void dfsSortLevels(
        adjEntry adj1,
        const NodeArray<int> & rank,
        Array<SListPure<node> > & nodes);

    // needed for UPRLayoutSimple
    void longestPathRanking(const Graph & G, NodeArray<int> & rank);


};

}

#endif
