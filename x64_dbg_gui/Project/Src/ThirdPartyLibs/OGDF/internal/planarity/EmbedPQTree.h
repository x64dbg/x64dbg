/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of the class EmbedPQTree.
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



#ifndef OGDF_EMBED_PQTREE_H
#define OGDF_EMBED_PQTREE_H

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/SList.h>
#include <ogdf/internal/planarity/PQTree.h>
#include <ogdf/internal/planarity/PlanarLeafKey.h>
#include <ogdf/internal/planarity/EmbedIndicator.h>

namespace ogdf
{

typedef PQBasicKey<edge, IndInfo*, bool>* PtrPQBasicKeyEIB;

template<>
inline bool doDestruction<PtrPQBasicKeyEIB>(const PtrPQBasicKeyEIB*)
{
    return false;
}


typedef PlanarLeafKey<IndInfo*>* PtrPlanarLeafKeyI;

template<>
inline bool doDestruction<PtrPlanarLeafKeyI>(const PtrPlanarLeafKeyI*)
{
    return false;
}


class EmbedPQTree: public PQTree<edge, IndInfo*, bool>
{
public:

    EmbedPQTree() : PQTree<edge, IndInfo*, bool>() { }

    virtual ~EmbedPQTree() { }

    virtual void emptyAllPertinentNodes();

    virtual void clientDefinedEmptyNode(PQNode<edge, IndInfo*, bool>* nodePtr);

    virtual int Initialize(SListPure<PlanarLeafKey<IndInfo*>*> & leafKeys);

    void ReplaceRoot(
        SListPure<PlanarLeafKey<IndInfo*>*> & leafKeys,
        SListPure<edge> & frontier,
        SListPure<node> & opposed,
        SListPure<node> & nonOpposed,
        node v);

    virtual bool Reduction(SListPure<PlanarLeafKey<IndInfo*>*> & leafKeys);

    PQNode<edge, IndInfo*, bool>* scanSibLeft(PQNode<edge, IndInfo*, bool>* nodePtr) const
    {
        return clientSibLeft(nodePtr);
    }

    PQNode<edge, IndInfo*, bool>* scanSibRight(PQNode<edge, IndInfo*, bool>* nodePtr) const
    {
        return clientSibRight(nodePtr);
    }

    PQNode<edge, IndInfo*, bool>* scanLeftEndmost(PQNode<edge, IndInfo*, bool>* nodePtr) const
    {
        return clientLeftEndmost(nodePtr);
    }

    PQNode<edge, IndInfo*, bool>* scanRightEndmost(PQNode<edge, IndInfo*, bool>* nodePtr) const
    {
        return clientRightEndmost(nodePtr);
    }

    PQNode<edge, IndInfo*, bool>* scanNextSib(
        PQNode<edge, IndInfo*, bool>* nodePtr,
        PQNode<edge, IndInfo*, bool>* other)
    {
        return clientNextSib(nodePtr, other);
    }

    virtual void getFront(
        PQNode<edge, IndInfo*, bool>* nodePtr,
        SListPure<PQBasicKey<edge, IndInfo*, bool>*> & leafKeys);

protected:

    virtual PQNode<edge, IndInfo*, bool>*
    clientSibLeft(PQNode<edge, IndInfo*, bool>* nodePtr) const;

    virtual PQNode<edge, IndInfo*, bool>*
    clientSibRight(PQNode<edge, IndInfo*, bool>* nodePtr) const;

    virtual PQNode<edge, IndInfo*, bool>*
    clientLeftEndmost(PQNode<edge, IndInfo*, bool>* nodePtr) const;

    virtual PQNode<edge, IndInfo*, bool>*
    clientRightEndmost(PQNode<edge, IndInfo*, bool>* nodePtr) const;

    virtual PQNode<edge, IndInfo*, bool>*
    clientNextSib(PQNode<edge, IndInfo*, bool>* nodePtr,
                  PQNode<edge, IndInfo*, bool>* other) const;
    virtual const char*
    clientPrintStatus(PQNode<edge, IndInfo*, bool>* nodePtr);

    virtual void front(
        PQNode<edge, IndInfo*, bool>* nodePtr,
        SListPure<PQBasicKey<edge, IndInfo*, bool>*> & leafKeys);

private:

    void ReplaceFullRoot(
        SListPure<PlanarLeafKey<IndInfo*>*> & leafKeys,
        SListPure<PQBasicKey<edge, IndInfo*, bool>*> & frontier,
        node v,
        bool addIndicator = false,
        PQNode<edge, IndInfo*, bool>* opposite = 0);

    void ReplacePartialRoot(
        SListPure<PlanarLeafKey<IndInfo*>*> & leafKeys,
        SListPure<PQBasicKey<edge, IndInfo*, bool>*> & frontier,
        node v);
};

}

#endif
