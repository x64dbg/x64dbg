/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration and implementation of the class PQNodeRoot.
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


#ifndef OGDF_PQ_NODE_ROOT_H
#define OGDF_PQ_NODE_ROOT_H



namespace ogdf
{


/**
 * The class PQNodeRoot is used as a base class of the class
 * PQNode. Using the class PQNodeRoot, a user may
 * refer to a node without the class structure.
 */

class PQNodeRoot
{

public:
    enum PQNodeType { PNode = 1, QNode = 2, leaf = 3 };

    enum SibDirection { NODIR, LEFT, RIGHT };

    // Status Definitions
    enum PQNodeStatus
    {
        EMPTY         = 1,
        PARTIAL       = 2,
        FULL          = 3,
        PERTINENT     = 4,
        TO_BE_DELETED = 5,

        // Extra node status defines
        INDICATOR     = 6,
        ELIMINATED    = 6,  //!< Nodes removed durign the template reduction are marked as
        //!< as ELIMINATED. Their memory is not freed. They are kept
        //!< for parent pointer update.
        WHA_DELETE    = 7,  //!< Nodes that need to be removed in order to obtain a
        //!< maximal pertinent sequence are marked WHA_DELETE.
        PERTROOT      = 8   //!< The pertinent Root is marked PERTROOTduring the clean up
                        //!< after a reduction. Technical.
    };

    // Mark Definitions for Bubble Phase
    enum PQNodeMark { UNMARKED = 0, QUEUED = 1, BLOCKED = 2, UNBLOCKED = 3 };


    PQNodeRoot() { }
    virtual ~PQNodeRoot() { }

    OGDF_NEW_DELETE
};

}

#endif

