/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Abstract MultilevelBuilder builds all Levels
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

#ifndef OGDF_MULTILEVEL_BUILDER_H
#define OGDF_MULTILEVEL_BUILDER_H

#include <ogdf/basic/Graph.h>
#include <ogdf/internal/energybased/MultilevelGraph.h>

namespace ogdf
{

class OGDF_EXPORT MultilevelBuilder
{
private:
    /**
     * \brief This method constructs one more level on top of an existing MultilevelGraph.
     * It must be implemented in any MultilevelBuilder. A level is built by
     *  adding node-merges to the MultilevelGraph and updating the graph accordingly.
     * This is achieved by calling MLG.
     *
     * @param MLG is the MultilevelGraph for which a new gevel will be built.
     *
     * @return true if the Graph was changed or false if no Level can be built.
     */
    virtual bool buildOneLevel(MultilevelGraph & MLG) = 0;

protected:
    // if set to true the length of the edge between two merged nodes will be added to
    //  all edges that are moved to the other node in this merge.
    int m_adjustEdgeLengths;
    int m_numLevels; //!< stores number of levels for statistics purposes

public:
    virtual ~MultilevelBuilder() { }
    MultilevelBuilder(): m_adjustEdgeLengths(0), m_numLevels(1) { }

    virtual void buildAllLevels(MultilevelGraph & MLG)
    {
        m_numLevels = 1;
        MLG.updateReverseIndizes();
        MLG.updateMergeWeights();
        while(buildOneLevel(MLG))
        {
            m_numLevels++;
        }
        MLG.updateReverseIndizes();
    }

    void setEdgeLengthAdjustment(int factor)
    {
        m_adjustEdgeLengths = factor;
    }
    int getNumLevels()
    {
        return m_numLevels;
    }

};

} // namespace ogdf

#endif
