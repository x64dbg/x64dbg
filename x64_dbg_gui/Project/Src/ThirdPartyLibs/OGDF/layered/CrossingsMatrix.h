/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class CrossingsMatrix.
 *
 * \author Andrea Wagner
 *         Michael Schulz
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

#ifndef OGDF_CROSSINGS_MATRIX_H
#define OGDF_CROSSINGS_MATRIX_H

#include <ogdf/basic/EdgeArray.h>
#include <ogdf/basic/Array2D.h>
#include <ogdf/layered/Hierarchy.h>

namespace ogdf
{

//---------------------------------------------------------
// CrossingsMatrix
// implements crossings matrix which is used by some
// TwoLayerCrossingMinimization heuristics (e.g. split)
//---------------------------------------------------------
class OGDF_EXPORT CrossingsMatrix
{
public:
    CrossingsMatrix() : matrix(0, 0, 0, 0)
    {
        m_bigM = 10000;
    }

    CrossingsMatrix(const Hierarchy & H);

    ~CrossingsMatrix() { }

    int operator()(int i, int j) const
    {
        return matrix(map[i], map[j]);
    }

    void swap(int i, int j)
    {
        map.swap(i, j);
    }

    //! ordinary init
    void init(Level & L);

    //! SimDraw init
    void init(Level & L, const EdgeArray<unsigned int>* edgeSubGraph);

private:
    Array<int> map;
    Array2D<int> matrix;
    //! need this for SimDraw to grant epsilon-crossings instead of zero-crossings
    int m_bigM; // is set to some big number in both constructors
};

}// end namespace ogdf

#endif
