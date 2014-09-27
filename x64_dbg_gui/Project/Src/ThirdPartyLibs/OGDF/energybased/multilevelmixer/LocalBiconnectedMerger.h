/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Merges nodes with neighbour to get a Multilevel Graph
 *
 * \author Gereon Bartel
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

#include <ogdf/energybased/multilevelmixer/MultilevelBuilder.h>
#include <ogdf/basic/HashArray.h>

#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_LOCAL_BICONNECTED_MERGER_H
#define OGDF_LOCAL_BICONNECTED_MERGER_H

namespace ogdf
{

class OGDF_EXPORT LocalBiconnectedMerger : public MultilevelBuilder
{
private:
    double m_levelSizeFactor;
    NodeArray<node> m_substituteNodes;
    NodeArray<bool> m_isCut;
    HashArray<int, int> m_realNodeMarks;

    void initCuts(Graph & G);
    int realNodeMark(int index);

    //! Creates the next level in the hierarchy by merging
    //! vertices based on matching, edge cover, and local biconnectivity check.
    bool buildOneLevel(MultilevelGraph & MLG);
    bool doMerge(MultilevelGraph & MLG, node parent, node mergePartner, int level);
    bool doMergeIfPossible(Graph & G, MultilevelGraph & MLG, node parent, node mergePartner, int level);
    bool canMerge(Graph & G, node parent, node mergePartner);
    bool canMerge(Graph & G, node parent, node mergePartner, int testStrength);

public:
    //! Constructs a LocalBiconnectedMerger multilevel builder.
    LocalBiconnectedMerger();
    //! Specifies the ratio between two consecutive level sizes up to which
    //! merging is done.
    void setFactor(double factor);
};

} // namespace ogdf

#endif
