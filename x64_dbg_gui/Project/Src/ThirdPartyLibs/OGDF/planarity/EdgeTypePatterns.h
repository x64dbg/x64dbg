/*
 * $Revision: 2566 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 23:10:08 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Edge types and patterns for planar representations
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

//edge type patterns:
//FOUR TYPE LEVELS:
//primary holds information about generalization/association,...
//secondary about merger edges,...
//user edge types can be set locally

#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_EDGE_TYPE_PATTERNS_H
#define OGDF_EDGE_TYPE_PATTERNS_H

namespace ogdf
{

typedef long edgeType;

enum UMLEdgeTypePatterns
{
    etpPrimary   = 0x0000000f,
    etpSecondary = 0x000000f0,
    etpTertiary  = 0x00000f00,
    etpFourth    = 0x0000f000,
    etpUser      = 0xff000000,
    etpAll       = 0xffffffff
}; //!!!attention sign, 7fffffff

enum UMLEdgeTypeConstants
{
    //primary types (should be disjoint bits)
    etcPrimAssociation = 0x1, etcPrimGeneralization = 0x2, etcPrimDependency = 0x4,
    //secondary types: reason of insertion (should be disjoint types, but not bits,
    //but may not completely cover others that are allowed to be set together)
    //preliminary: setsecondarytype deletes old type
    //edge in Expansion, dissection edge, face splitter, cluster boundary
    etcSecExpansion = 0x1, etcSecDissect = 0x2, etcSecFaceSplitter = 0x3,
    etcSecCluster = 0x4, etcSecClique, //the boundaries
    //tertiary types: special types
    //merger edge, vertical in hierarchy, alignment, association class connnection
    etcMerger = 0x1, etcVertical = 0x2, etcAlign = 0x3, etcAssClass = 0x8,
    //fourth types: relation of nodes
    //direct neighbours in hierarchy = brother, neighbour = halfbrother
    //same level = cousin, to merger = ToMerger, from Merger = FromMerger
    etcBrother = 0x1, etcHalfBrother = 0x2, etcCousin = 0x3,
    //fifth level types
    etcFifthToMerger = 0x1, etcFifthFromMerger = 0x2
            //user type hint: what you have done with the edge, e.g. brother edge
            //that is embedded crossing free and should be drawn bend free
};
enum UMLEdgeTypeOffsets
{
    etoPrimary = 0, etoSecondary = 4, etoTertiary = 8, etoFourth = 12, etoFifth = 16,
    etoUser = 24
};

} //end namespace ogdf

#endif
