/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class PlanarSubgraphPQTree.
 *
 * Datastructure used by the planarization module FastPlanarSubgraph.
 * Derived class of MaxSequencePQTree. Implements an Interface
 * of MaxSequencePQTree for the planarization module FastPlanarSubgraph.
 *
 * \author Sebastian Leipert
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


#ifndef OGDF_PLANAR_SUBGRAPH_PQTREE_H
#define OGDF_PLANAR_SUBGRAPH_PQTREE_H



#include <ogdf/basic/Graph.h>
#include <ogdf/basic/SList.h>
#include <ogdf/internal/planarity/PQTree.h>
#include <ogdf/internal/planarity/PlanarLeafKey.h>
#include <ogdf/internal/planarity/MaxSequencePQTree.h>

namespace ogdf
{

// Overriding the function doDestruction (see basic.h)
// Allows deallocation of lists of PlanarLeafKey<whaInfo*>
// and PQLeafKey<edge,IndInfo*,bool> in constant time using OGDF
// memory management.


typedef PQLeafKey<edge, whaInfo*, bool>* PtrPQLeafKeyEWB;

template<>
inline bool doDestruction<PtrPQLeafKeyEWB>(const PtrPQLeafKeyEWB*)
{
    return false;
}

typedef PlanarLeafKey<whaInfo*>* PtrPlanarLeafKeyW;

template<>
inline bool doDestruction<PtrPlanarLeafKeyW>(const PtrPlanarLeafKeyW*)
{
    return false;
}



class PlanarSubgraphPQTree: public MaxSequencePQTree<edge, bool>
{

public:

    PlanarSubgraphPQTree() : MaxSequencePQTree<edge, bool>() { }

    virtual ~PlanarSubgraphPQTree() { }

    //! Initializes a new PQ-tree with a set of leaves.
    virtual int Initialize(SListPure<PlanarLeafKey<whaInfo*>*> & leafKeys);

    //! Replaces the pertinent subtree by a set of new leaves.
    void ReplaceRoot(SListPure<PlanarLeafKey<whaInfo*>*> & leafKeys);

    //! Reduces a set of leaves.
    virtual bool Reduction(
        SListPure<PlanarLeafKey<whaInfo*>*> & leafKeys,
        SList<PQLeafKey<edge, whaInfo*, bool>*> & eliminatedKeys);

private:

    //! Replaces a pertinet subtree by a set of new leaves if the root is full.
    void ReplaceFullRoot(SListPure<PlanarLeafKey<whaInfo*>*> & leafKeys);

    //! Replaces a pertinet subtree by a set of new leaves if the root is partial.
    void ReplacePartialRoot(SListPure<PlanarLeafKey<whaInfo*>*> & leafKeys);

    //! Removes the leaves that have been marked for elimination from the PQ-tree.
    void removeEliminatedLeaves(SList<PQLeafKey<edge, whaInfo*, bool>*> & eliminatedKeys);

};

}

#endif
