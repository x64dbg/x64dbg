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

#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_INDEPENDENT_SET_MERGER_H
#define OGDF_INDEPENDENT_SET_MERGER_H

namespace ogdf
{

class OGDF_EXPORT IndependentSetMerger : public MultilevelBuilder
{
private:
    float m_base;

    std::vector<node> prebuildLevel(const Graph & G, const std::vector<node> & oldLevelNodes, int level);
    bool buildOneLevel(MultilevelGraph & MLG)
    {
        return false;
    }
    bool buildOneLevel(MultilevelGraph & MLG, std::vector<node> & levelNodes);

public:
    void buildAllLevels(MultilevelGraph & MLG);
    void setSearchDepthBase(float base);

    IndependentSetMerger();
};

} // namespace ogdf

#endif
