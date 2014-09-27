/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Preprocessor Layout simplifies Graphs for use in other Algorithms
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

#ifndef OGDF_PREPROCESSOR_LAYOUT_H
#define OGDF_PREPROCESSOR_LAYOUT_H


#include <ogdf/basic/ModuleOption.h>
#include <ogdf/module/MultilevelLayoutModule.h>


namespace ogdf
{


/** \brief The PreprocessorLayout removes multi-edges and self-loops.
 *
 * To draw a graph using the ModularMultilevelMixer or other layouts the
 * graph must be simple, i.e., contain neither multi-edges nor self-loops.
 * Edges that conflict with these rules are deleted in the PreprocessorLayout.
 * A secondary layout is then called that can work on the graph in required form.
 * After the layout has been computed, the edges are inserted back into the
 * graph, as they may have been relevant for the user.
 */
class OGDF_EXPORT PreprocessorLayout : public MultilevelLayoutModule
{
private:
    /** \brief Deleted Edges are stored in EdgeData
     *
     * EdgeData stores the deleted edges to allow restauration of the original
     * graph after the layout has been computed.
     */
    struct EdgeData
    {
        EdgeData(int edgeIndex, int sourceIndex, int targetIndex, double weight)
            : edgeIndex(edgeIndex), sourceIndex(sourceIndex), targetIndex(targetIndex), weight(weight)
        { }

        int edgeIndex;
        int sourceIndex;
        int targetIndex;
        double weight;
    };

    ModuleOption<LayoutModule> m_secondaryLayout;
    std::vector<EdgeData> m_deletedEdges;
    bool m_randomize;

    void call(Graph & G, MultilevelGraph & MLG);

public:

    //! Constructor
    PreprocessorLayout();

    //! Destructor
    ~PreprocessorLayout() { }


    //! Calculates a drawing for the Graph \a MLG.
    void call(MultilevelGraph & MLG);

    //! Calculates a drawing for the Graph \a GA.
    void call(GraphAttributes & GA);

    //! Sets the secondary layout.
    void setLayoutModule(LayoutModule* layout)
    {
        m_secondaryLayout.set(layout);
    }

    //! Defines whether the positions of the node are randomized before the secondary layout call.
    void setRandomizePositions(bool on)
    {
        m_randomize = on;
    }
};


} // namespace ogdf

#endif
