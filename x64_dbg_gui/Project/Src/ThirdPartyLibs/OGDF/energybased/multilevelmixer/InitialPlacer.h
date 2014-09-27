/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Abstract InitialPlacer places the nodes of the level into the next.
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

#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_INITIAL_PLACER_H
#define OGDF_INITIAL_PLACER_H

#include <ogdf/basic/Graph.h>
#include <ogdf/internal/energybased/MultilevelGraph.h>

namespace ogdf
{

class OGDF_EXPORT InitialPlacer
{
protected:
    bool m_randomOffset;

public:
    InitialPlacer(): m_randomOffset(true) { }
    virtual ~InitialPlacer() { }

    virtual void placeOneLevel(MultilevelGraph & MLG) = 0;

    void setRandomOffset(bool on)
    {
        m_randomOffset = on;
    }

};

} // namespace ogdf

#endif
