/*
 * $Revision: 2566 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 23:10:08 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of the FastPlanarSubgraph.
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


#ifndef OGDF_FAST_PLANAR_SUBGRAPH_H
#define OGDF_FAST_PLANAR_SUBGRAPH_H



#include <ogdf/module/PlanarSubgraphModule.h>


namespace ogdf
{

/**
 * \brief Computation of a planar subgraph using PQ-trees.
 *
 * Literature: Jayakumar, Thulasiraman, Swamy 1989
 *
 * <h3>Optional Parameters</h3>
 *
 * <table>
 *   <tr>
 *     <th>Option</th><th>Type</th><th>Default</th><th>Description</th>
 *   </tr><tr>
 *     <td><i>runs</i></td><td>int</td><td>0</td>
 *     <td>the number of randomized runs performed by the algorithm; the best
 *         solution is picked among all the runs. If runs is 0, one
 *         deterministic run is performed.</td>
 *   </tr>
 * </table>
 *
 * Observe that this algorithm by theory does not compute a maximal
 * planar subgraph. It is however the fastest known good heuristic.
 */
class OGDF_EXPORT FastPlanarSubgraph : public PlanarSubgraphModule
{

public:
    //! Creates an instance of the fast planar subgraph algorithm.
    FastPlanarSubgraph() : PlanarSubgraphModule()
    {
        m_nRuns = 0;
    };

    // destructor
    ~FastPlanarSubgraph() { }


    // options

    //! Sets the number of randomized runs to \a nRuns.
    void runs(int nRuns)
    {
        m_nRuns = nRuns;
    }

    //! Returns the current number of randomized runs.
    int runs() const
    {
        return m_nRuns;
    }


protected:
    //! Returns true, if G is planar, false otherwise.
    /**
     * \todo Add timeout support (limit number of runs when timeout is reached).
     */
    ReturnType doCall(const Graph & G,
                      const List<edge> & preferedEdges,
                      List<edge> & delEdges,
                      const EdgeArray<int>*  pCost,
                      bool preferedImplyPlanar);


private:
    int m_nRuns;  //!< The number of runs for randomization.


    //! Computes the list of edges to be deleted in \a G.
    /** Also performs randomization of the planarization algorithm.
     */
    void computeDelEdges(const Graph & G,
                         const EdgeArray<int>* pCost,
                         const EdgeArray<edge>* backTableEdges,
                         List<edge> & delEdges);

    //! Performs a planarization on a biconnected component pf \a G.
    /** The numbering contains an st-numbering of the component.
     */
    void planarize(
        const Graph & G,
        NodeArray<int> & numbering,
        List<edge> & delEdges);
};

}
#endif
