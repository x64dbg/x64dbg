/*
* $Revision: 2523 $
*
* last checkin:
*   $Author: gutwenger $
*   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
***************************************************************/

/** \file
 * \brief Declaration of an exact c-planar subgraph algorithm, i.e.,
 * a maximum c-planar subgraph is computed using a branch and cut approach.
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


#ifdef _MSC_VER
#pragma once
#endif

#ifndef OGDF_MAXIMUM_CPLANAR_SUBGRAPH_H
#define OGDF_MAXIMUM_CPLANAR_SUBGRAPH_H

#include <ogdf/basic/Module.h>
#include <ogdf/basic/Timeouter.h>

#include <ogdf/module/CPlanarSubgraphModule.h>
#include <ogdf/cluster/ClusterGraph.h>
#include <ogdf/internal/cluster/MaxCPlanar_Master.h>

#include <ogdf/external/abacus.h>

namespace ogdf
{

/**
 * MaximumCPlanarSubgraph
 * \brief Exact computation of a maximum c-planar subgraph.
 */
class OGDF_EXPORT MaximumCPlanarSubgraph : public CPlanarSubgraphModule
{

#ifndef USE_ABACUS
protected:
    ReturnType doCall(
        const ClusterGraph & G,
        List<edge> & delEdges,
        List<edge> & addedEdges)
    {
        THROW_NO_ABACUS_EXCEPTION;
        return retError;
    }

    virtual ReturnType doCall(
        const ClusterGraph & G,
        List<edge> & delEdges)
    {
        THROW_NO_ABACUS_EXCEPTION;
        return retError;
    }
};
#else // USE_ABACUS

public:
    //! Construction
    MaximumCPlanarSubgraph() : m_heuristicLevel(1),
        m_heuristicRuns(1),
        m_heuristicOEdgeBound(0.4),
        m_heuristicNPermLists(5),
        m_kuratowskiIterations(10),
        m_subdivisions(10),
        m_kSupportGraphs(10),
        m_kuratowskiHigh(0.8),
        m_kuratowskiLow(0.8),
        m_perturbation(false),
        m_branchingGap(0.4),
        m_time("00:20:00"),
        m_pricing(true),
        m_checkCPlanar(false),
        m_numAddVariables(15),
        m_strongConstraintViolation(0.3),
        m_strongVariableViolation(0.3),
        m_ol(ABA_MASTER::Silent),
        m_totalTime(-1.0),
        m_heurTime(-1.0),
        m_lpTime(-1.0),
        m_lpSolverTime(-1.0),
        m_sepTime(-1.0),
        m_totalWTime(-1.0),
        m_numCCons(-1),
        m_numKCons(-1),
        m_numLPs(-1),
        m_numBCs(-1),
        m_numSubSelected(-1),
        m_portaOutput(false) {}
    //destruction
    ~MaximumCPlanarSubgraph() {}

    //! Computes set of edges delEdges, which have to be deleted
    //! in order to get a c-planar subgraph and also returns
    //! a set of edges that augments the subgraph to be
    //! completely connected.
    //! For pure c-planarity testing, the computation can be sped
    //! up by setting setCheckCPlanar(2). Then, in case G is not c-planar,
    //! the list of deleted edges does not need to correspond
    //! to a valid solution, it just indicates the result.
    ReturnType call(const ClusterGraph & G, List<edge> & delEdges,
                    List<nodePair> & addedEdges)
    {
        return doCall(G, delEdges, addedEdges);
    }
    //setter methods for the  module parameters
    void setHeuristicLevel(int i)
    {
        m_heuristicLevel = i;
    }
    void setHeuristicRuns(int i)
    {
        m_heuristicRuns = i;
    }
    void setHeuristicBound(double d)
    {
        m_heuristicOEdgeBound = d;
    }
    void setNumberOfPermutations(int i)
    {
        m_heuristicNPermLists = i;
    }
    void setNumberOfKuraIterations(int i)
    {
        m_kuratowskiIterations = i;
    }
    void setNumberOfSubDivisions(int i)
    {
        m_subdivisions = i;
    }
    void setNumberOfSupportGraphs(int i)
    {
        m_kSupportGraphs = i;
    }
    void setUpperRounding(double d)
    {
        m_kuratowskiHigh = d;
    }
    void setLowerRounding(double d)
    {
        m_kuratowskiLow = d;
    }
    void setPerturbation(bool b)
    {
        m_perturbation = b;
    }
    void setBranchingGap(double d)
    {
        m_branchingGap = d;
    }
    void setTimeLimit(String s)
    {
        m_time = s.cstr();
    }
    void setOutputLevel(ABA_MASTER::OUTLEVEL o)
    {
        m_ol = o;
    }
    void setPortaOutput(bool b)
    {
        m_portaOutput = b;
    }
    void setPricing(bool b)
    {
        m_pricing = b;
    }
    void setCheckCPlanar(bool b)
    {
        m_checkCPlanar = b;
    }
    void setNumAddVariables(int n)
    {
        m_numAddVariables = n;
    }
    void setStrongConstraintViolation(double d)
    {
        m_strongConstraintViolation = d;
    }
    void setStrongVariableViolation(double d)
    {
        m_strongVariableViolation = d;
    }
    //! Use default abacus master cut pool or dedicated connectivity and
    //! kuratowski cut pools
    bool & useDefaultCutPool()
    {
        return m_defaultCutPool;
    }

    //getter methods for results
    double getTotalTime()
    {
        return m_totalTime;
    }
    double getHeurTime()
    {
        return m_heurTime;
    }
    double getLPTime()
    {
        return m_lpTime;
    }
    double getLPSolverTime()
    {
        return m_lpSolverTime;
    }
    double getSeparationTime()
    {
        return m_sepTime;
    }
    double getTotalWTime()
    {
        return m_totalWTime;
    }
    //! Returns number of connectivity constraints added during computation
    int    getNumCCons()
    {
        return m_numCCons;
    }
    //! Returns number of Kuratowski constraints added during computation
    int    getNumKCons()
    {
        return m_numKCons;
    }
    //! Returns number of optimized LPs (only LP-relaxations)
    int    getNumLPs()
    {
        return m_numLPs;
    }
    //! Returns number of generated LPs
    int    getNumBCs()
    {
        return m_numBCs;
    }
    //! Returns number of subproblems selected by ABACUS
    int    getNumSubSelected()
    {
        return m_numSubSelected;
    }
    //! Returns number of global variables. Todo: Real number from ABACUS
    int    getNumVars()
    {
        return m_numVars;
    }

    //! Writes feasible solutions as a file in PORTA format
    void writeFeasible(const String & filename, Master & master,
                       ABA_MASTER::STATUS & status);
#ifdef OGDF_DEBUG
    bool getSolByHeuristic()
    {
        return m_solByHeuristic;
    }
#endif


protected:
    //! Computes a maximum c-planar subgraph, returns the
    //! set of edges that have to be deleted in delEdges
    //! if delEdges is empty on return, the clustered
    //! graph G is c-planar
    virtual ReturnType doCall(const ClusterGraph & G,
                              List<edge> & delEdges)
    {
        List<nodePair> addEdges;
        return doCall(G, delEdges, addEdges);
    }

    //as above, also returns the set of edges that were
    //added to construct a completely connected planar
    //graph that contains the computed c-planar subgraph
    virtual ReturnType doCall(const ClusterGraph & G,
                              List<edge> & delEdges,
                              List<nodePair> & addedEdges);

    double getDoubleTime(const ABA_TIMER & act)
    {
        long tempo = act.centiSeconds() + 100 * act.seconds() + 6000 * act.minutes() + 360000 * act.hours();
        return ((double) tempo) / 100.0;
    }//getdoubletime

    //! Stores clusters in subtree at c in bottom up order in theList
    void getBottomUpClusterList(const cluster c, List< cluster > & theList);

private:
    //Abacus Master class settings in order of appearance
    int m_heuristicLevel, m_heuristicRuns;
    double m_heuristicOEdgeBound;
    int m_heuristicNPermLists, m_kuratowskiIterations;
    int m_subdivisions, m_kSupportGraphs;
    double m_kuratowskiHigh, m_kuratowskiLow;
    bool m_perturbation;
    double m_branchingGap;
    const char* m_time;
    bool m_pricing;//<! Decides if pricing is used
    bool m_checkCPlanar;//<! If set to true, only c-planarity is checked
    int m_numAddVariables;
    double m_strongConstraintViolation;
    double m_strongVariableViolation;

    ABA_MASTER::OUTLEVEL m_ol;

    const char* getPortaFileName()
    {
        return "porta.poi";
    }
    const char* getIeqFileName()
    {
        return "porta.ieq";
    }
    const int maxConLength()
    {
        return 1024;
    }
    void outputCons(ofstream & os,
                    ABA_STANDARDPOOL<ABA_CONSTRAINT, ABA_VARIABLE>* connCon,
                    ABA_STANDARDPOOL<ABA_VARIABLE, ABA_CONSTRAINT>* stdVar);

    //results
    double m_totalTime;  //<! Total computation time, returned from abacus
    double m_heurTime;   //<! Time spent in heuristic, returned from abacus
    double m_lpTime;     //<! Cpu time spent in members of the LP-interface, returned from abacus
    double m_lpSolverTime; //<! Cpu time required by the LP solver, returned from abacus
    double m_sepTime;  //<! Cpu time spent in the separation of cutting planes, returned from abacus
    double m_totalWTime; //<! Total wall clock time, returned from abacus
    int    m_numCCons;   //<! Number of added connectivity constraints
    int    m_numKCons;   //<! Number of added kuratowski constraints
    int    m_numLPs;     //<! The number of optimized linear programs (LP-relax.).
    int    m_numBCs;     //<! The number of generated subproblems.
    int    m_numSubSelected; //<! The number of subproblems selected by ABACUS.
    int    m_numVars;    //<! The number of global variables.
    bool   m_portaOutput; //<! If set to true, output for PORTA in ieq format is generated.
    bool   m_defaultCutPool; //<! Passed through to master
#ifdef OGDF_DEBUG
    bool m_solByHeuristic;
#endif

};

#endif // USE_ABACUS

} //end namespace ogdf


#endif // OGDF_MAXIMUM_CPLANAR_SUBGRAPH_H
