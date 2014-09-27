/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief planar biconnected augmentation algorithm with fixed
 *        combinatorial embedding.
 *
 * \author Bernd Zey
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

#ifndef OGDF_PLANAR_AUGMENTATION_FIX_H
#define OGDF_PLANAR_AUGMENTATION_FIX_H

#include <ogdf/module/AugmentationModule.h>
#include <ogdf/basic/GraphCopy.h>
#include <ogdf/internal/augmentation/PALabel.h>


namespace ogdf
{

class DynamicBCTree;


/**
 * \brief The algorithm for biconnectivity augmentation with fixed combinatorial embedding.
 *
 */
class OGDF_EXPORT PlanarAugmentationFix : public AugmentationModule
{

public:
    //! Creates an instance of planar augmentation with fixed embedding.
    PlanarAugmentationFix() { }

    ~PlanarAugmentationFix() { }


protected:
    /**
     * \brief The implementation of the algorithm call.
     *
     * \param g is the working graph.
     * \param L is the list of all new edges.
     */
    void doCall(Graph & g, List<edge> & L);

private:
    /**
     * \brief The embedding of g.
     */
    CombinatorialEmbedding* m_pEmbedding;

    /**
     * \brief The embedding of the actual partial graph.
     */
    CombinatorialEmbedding* m_pActEmbedding;

    /**
     * \brief The working graph.
     */
    Graph* m_pGraph;

    /**
     * \brief The inserted edges by the algorithm.
     */
    List<edge>* m_pResult;

    /**
     * \brief The actual dynamic bc-tree.
     */
    DynamicBCTree* m_pBCTree;

    /**
     * \brief The actual partial graph.
     */
    GraphCopy m_graphCopy;

    /**
     * \brief Edge-array required for construction of the graph copy.
     */
    EdgeArray<edge> m_eCopy;

    /**
     * \brief The list of all labels.
     */
    List<pa_label> m_labels;

    /**
     * \brief Array that contains iterators to the list of labels
     *       if a node is a parent of a label.
     */
    NodeArray< ListIterator<pa_label> > m_isLabel;

    /**
     * \brief Array that contains the label a node belongs to.
     */
    NodeArray<pa_label> m_belongsTo;

    /**
     * \brief Array that contains the iterator of the label a node belongs to.
     */
    NodeArray< ListIterator<node> > m_belongsToIt;

    /**
     * \brief The actual root of the bc-tree.
     */
    node m_actBCRoot;

    /**
     * \brief The main function for planar augmentation.
     */
    void augment(adjEntry adjOuterFace);

    /**
     * \brief Modifies the root of the bc-tree.
     */
    void modifyBCRoot(node oldRoot, node newRoot);

    /**
     * \brief Exchanges oldRoot by newRoot and updates data structurs in the bc-tree.
     */
    void changeBCRoot(node oldRoot, node newRoot);

    /**
     * \brief Adds the pendant to a label or creates one (uses followPath()).
     */
    void reduceChain(node pendant);

    /**
     * \brief Traverses upwards in the bc-tree, starting at the pendant node.
     */
    paStopCause followPath(node v, node & last);

    /**
     * \brief Finds the next matching pendants.
     */
    bool findMatching(node & pendant1, node & pendant2, adjEntry & v1, adjEntry & v2);

    /**
     * \brief Called by findMatching, if a dominating tree was detected.
     */
    void findMatchingRev(node & pendant1, node & pendant2, adjEntry & v1, adjEntry & v2);

    /**
     * \brief Creates a new label.
     */
    pa_label newLabel(node cutvertex, node parent, node pendant, paStopCause whyStop);

    /**
     * \brief Adds pendant \a p to label \a l.
     */
    void addPendant(node p, pa_label & l);

    /**
     * \brief Inserts the label into the list of labels maintaining decreasing order.
     */
    ListIterator<pa_label> insertLabel(pa_label l);

    /**
     * \brief Connect the two pendants.
     */
    void connectPendants(node pendant1, node pendant2, adjEntry adjV1, adjEntry adjV2);

    /**
     * \brief Connects the remaining label.
     */
    void connectSingleLabel();

    /**
     * \brief Deletes the pendant.
     */
    void deletePendant(node pendant);

    /**
     * \brief Deletes the label.
     */
    void deleteLabel(pa_label & l, bool removePendants = true);

    /**
     * \brief Removes the label from the list of labels.
     */
    void removeLabel(pa_label & l);

};  // class PlanarAugmentationFix


} // namespace ogdf

#endif
