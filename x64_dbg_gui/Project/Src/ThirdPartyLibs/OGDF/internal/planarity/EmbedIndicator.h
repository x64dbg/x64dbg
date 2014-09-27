/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of the class EmbedIndicator.
 *
 * Implements the direction Indicator. Used by class EmbedPQTree.
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



#ifndef OGDF_EMBED_INDICATOR_H
#define OGDF_EMBED_INDICATOR_H



#include <ogdf/internal/planarity/PQNode.h>
#include <ogdf/internal/planarity/PQNodeKey.h>
#include <ogdf/internal/planarity/PQInternalKey.h>
#include <ogdf/internal/planarity/IndInfo.h>

namespace ogdf
{


class EmbedIndicator : public PQNode<edge, IndInfo*, bool>
{
public:

    EmbedIndicator(int count, PQNodeKey<edge, IndInfo*, bool>* infoPtr)
        : PQNode<edge, IndInfo*, bool>(count, infoPtr) { }

    virtual ~EmbedIndicator()
    {
        delete getNodeInfo()->userStructInfo();
        delete getNodeInfo();
    }

    PQNodeType  type() const
    {
        return leaf;
    }

    void type(PQNodeType) { }

    PQNodeStatus  status() const
    {
        return PQNodeRoot::INDICATOR;
    }

    void status(PQNodeStatus) { }

    PQNodeMark  mark() const
    {
        return UNMARKED;
    }

    void mark(PQNodeMark) { }

    PQLeafKey<edge, IndInfo*, bool>* getKey() const
    {
        return 0;
    }

    bool setKey(PQLeafKey<edge, IndInfo*, bool>* pointerToKey)
    {
        return (pointerToKey == 0);
    }

    PQInternalKey<edge, IndInfo*, bool>* getInternal() const
    {
        return 0;
    }

    bool setInternal(PQInternalKey<edge, IndInfo*, bool>* pointerToInternal)
    {
        return (pointerToInternal == 0);
    }
};


}

#endif

