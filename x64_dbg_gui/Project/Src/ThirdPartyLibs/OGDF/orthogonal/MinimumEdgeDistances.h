/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class MinimumEdgeDistances which maintains
 *        minimum distances between attached edges at a vertex
 *       (delta's and epsilon's)
 *
 * \author Carsten Gutwenger
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


#ifndef OGDF_MINIMUM_EDGE_DISTANCE_H
#define OGDF_MINIMUM_EDGE_DISTANCE_H


#include <ogdf/orthogonal/OrthoRep.h>


namespace ogdf
{


//---------------------------------------------------------
// MinimumEdgeDistances
// maintains input sizes for improvement compaction (delta's
// and epsilon's)
//---------------------------------------------------------
template <class ATYPE>
class MinimumEdgeDistances
{
public:
    // constructor
    MinimumEdgeDistances(const Graph & G, ATYPE sep) : m_delta(G), m_epsilon(G)
    {
        m_sep = sep;
    }

    // returns delta_s(v)^i (with i = 0 => l, i = 1 => r)
    const ATYPE & delta(node v, OrthoDir s, int i) const
    {
        OGDF_ASSERT(0 <= int(s) && int(s) <= 3 && 0 <= i && i <= 1);
        return m_delta[v].info[s][i];
    }

    ATYPE & delta(node v, OrthoDir s, int i)
    {
        OGDF_ASSERT(0 <= int(s) && int(s) <= 3 && 0 <= i && i <= 1);
        return m_delta[v].info[s][i];
    }

    // returns epsilon_s(v)^i (with i = 0 => l, i = 1 => r)
    const ATYPE & epsilon(node v, OrthoDir s, int i) const
    {
        OGDF_ASSERT(0 <= int(s) && int(s) <= 3 && 0 <= i && i <= 1);
        return m_epsilon[v].info[s][i];
    }

    ATYPE & epsilon(node v, OrthoDir s, int i)
    {
        OGDF_ASSERT(0 <= int(s) && int(s) <= 3 && 0 <= i && i <= 1);
        return m_epsilon[v].info[s][i];
    }

    ATYPE separation() const
    {
        return m_sep;
    }

    void separation(ATYPE sep)
    {
        m_sep = sep;
    }


private:
    struct InfoType
    {
        ATYPE info[4][2];
    };

    NodeArray<InfoType> m_delta;
    NodeArray<InfoType> m_epsilon;
    ATYPE m_sep;
};


} // end namespace ogdf


#endif
