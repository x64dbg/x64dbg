/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of the sub-problem class for the Branch&Cut algorithm
 * for the Maximum C-Planar SubGraph problem
 * Contains separation algorithms as well as primal heuristics.
 *
 * \author Mathias Jansen
 *
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

#ifndef OGDF_MAX_CPLANAR_SUB_H
#define OGDF_MAX_CPLANAR_SUB_H

#include <ogdf/internal/cluster/MaxCPlanar_Master.h>
#include <ogdf/basic/ArrayBuffer.h>
#include <ogdf/planarity/BoyerMyrvold.h>

#include <abacus/sub.h>

namespace ogdf
{

class Sub : public ABA_SUB
{

public:

    Sub(ABA_MASTER* master);
    Sub(ABA_MASTER* master, ABA_SUB* father, ABA_BRANCHRULE* branchRule, List<ABA_CONSTRAINT*> & criticalConstraints);

    virtual ~Sub();

    // Creation of a child-node in the Branch&Bound tree according to the
    // branching rule \a rule
    virtual ABA_SUB* generateSon(ABA_BRANCHRULE* rule);


protected:


    // Checks if the current solution of the LP relaxation is also a feasible solution to the ILP,
    // i.e. Integer-feasible + satisfying all Kuratowski- and Cut-constraints.
    virtual bool feasible();

    virtual int makeFeasible()
    {
        return 0;    // to trick ABA_SUB::solveLp...
    }
    //  virtual int removeNonLiftableCons() { return 0; } // to trick ABA_SUB::solveLp into returning 2
    int repair();

    virtual int optimize()
    {
        Logger::slout() << "OPTIMIZE BEGIN\tNode=" << this->id() << "\n";
        int ret = ABA_SUB::optimize();
        Logger::slout() << "OPTIMIZE END\tNode=" << this->id() << " db=" << dualBound() << "\tReturn=" << (ret ? "(error)" : "(ok)") << "\n";
        return ret;
    }

    //functions for testing properties of the clustered graph

    //! Checks if the cluster induced graphs and their complement
    //! are connected in the current solution.
    bool checkCConnectivity(const GraphCopy & support);
    bool checkCConnectivityOld(const GraphCopy & support);

    //! run through the pointer list parent and return the representative
    //! i.e. the node with parent[v] == v
    inline node getRepresentative(node v, NodeArray<node> & parent)
    {
        while(v != parent[v])
            v = parent[v];
        return v;
    }

    // Todo: Think about putting this into extended_graph_alg.h to make it
    // publically available
    int clusterBags(ClusterGraph & CG, cluster c);

    // using the below two functions iunstead (and instead of solveLp) we would get a more traditional situation
    //  virtual int separate() {
    //      return separateRealO(0.001);
    //  }
    //  virtual int pricing()  {
    //      return pricingRealO(0.001);
    //  }

    /** these functions are mainly reporting to let abacus think everthing is normal. the actual work is done
     * by separateReal() and pricingReal().
     * The steering of this process is performed in solveLp()
     */

    int separateReal(double minViolate);
    int pricingReal(double minViolate);

    inline int separateRealO(double minViolate)
    {
        Logger::slout() << "\tSeparate (minViolate=" << minViolate << ")..";
        int r = separateReal(minViolate);
        Logger::slout() << "..done: " << r << "\n";
        return r;
    }
    inline int pricingRealO(double minViolate)
    {
        Logger::slout() << "\tPricing (minViolate=" << minViolate << ")..";
        int r = pricingReal(minViolate);
        master()->m_varsPrice += r;
        Logger::slout() << "..done: " << r << "\n";
        return r;
    }

    virtual int separate()
    {
        Logger::slout() << "\tReporting Separation: " << ((m_reportCreation > 0) ? m_reportCreation : 0) << "\n";
        return (m_reportCreation > 0) ? m_reportCreation : 0;
    }
    virtual int pricing()
    {
        if(inOrigSolveLp) return 1;
        Logger::slout() << "\tReporting Prizing: " << ((m_reportCreation < 0) ? -m_reportCreation : 0) << "\n";
        return (m_reportCreation < 0) ? -m_reportCreation : 0;
    }

    virtual int solveLp();

    // Implementation of virtual function improve() inherited from ABA::SUB.
    // Invokes the function heuristicImprovePrimalBound().
    // Tthis function belongs to the ABACUS framework. It is invoked after each separation-step.
    // Since the heuristic is rather time consuming and because it is not very advantageous
    // to run the heuristic even if additional constraints have been found, the heuristic
    // is run "by hand" each time no further constraints coud be found, i.e. after having solved
    // the LP-relaxation optimally.
    virtual int improve(double & primalValue);

    // Two functions inherited from ABACUS and overritten to manipulate the branching behaviour.
    virtual int selectBranchingVariableCandidates(ABA_BUFFER<int> & candidates);
    virtual int selectBranchingVariable(int & variable);

    //! Adds the given constraints to the given pool
    inline int addPoolCons(ABA_BUFFER<ABA_CONSTRAINT*> & cons, ABA_STANDARDPOOL<ABA_CONSTRAINT, ABA_VARIABLE>* pool)
    {
        return (master()->useDefaultCutPool()) ? addCons(cons) : addCons(cons, pool);
    }//addPoolCons
    inline int separateCutPool(ABA_STANDARDPOOL<ABA_CONSTRAINT, ABA_VARIABLE>* pool, double minViolation)
    {
        return (master()->useDefaultCutPool()) ? 0 : constraintPoolSeparation(0, pool, minViolation);
    }//separateCutPool

private:

    Master* master()
    {
        return (Master*)master_;
    }

    // A flag indicating if constraints have been found in the current separation step.
    // Is used to check, if the primal heuristic should be run or not.
    bool m_constraintsFound;
    bool detectedInfeasibility;
    bool inOrigSolveLp;
    double realDualBound;

    // used for the steering in solveLp
    int m_reportCreation;
    bool m_sepFirst;
    List< ABA_CONSTRAINT* > criticalSinceBranching;
    ArrayBuffer<ABA_CONSTRAINT* > bufferedForCreation;
    int createVariablesForBufferedConstraints();

    void myAddVars(ABA_BUFFER<ABA_VARIABLE*> & b)
    {
        int num = b.number();
        ABA_BUFFER<bool> keep(master(), num);
        for(int i = num; i-- > 0;)
            keep.push(true);
        int r = addVars(b, 0, &keep);
        OGDF_ASSERT(r == num)
    }


    // Computes the support graph for Kuratowski- constraints according to the current LP-solution.
    // Parameter \a low defines a lower threshold, i.e all edges having value at most \a low
    // are not added to the support graph.
    // Parameter \a high defines an upper threshold, i.e all edges having value at least \a high,
    // are added to the support graph.
    // Edges having LP-value k that lies between \a low and \a high are added to the support graph
    // randomly with a probability of k.
    void kuratowskiSupportGraph(
        GraphCopy & support,
        double low,
        double high);

    // Computes the support graph for Connectivity- constraints according to the current LP-solution.
    void connectivitySupportGraph(
        GraphCopy & support,
        EdgeArray<double> & weight);

    // Computes the integer solution induced Graph.
    void intSolutionInducedGraph(GraphCopy & support);

    // Computes and returns the value of the lefthand side of the Kuratowski constraint
    // induced by \a it and given support graph \a gc.
    double subdivisionLefthandSide(
        SListConstIterator<KuratowskiWrapper> it,
        GraphCopy* gc);

    // Updates the best known solution according to integer solution of this subproblem.
    void updateSolution();


    // The following four functions are used by the primal heuristic.

    int getArrayIndex(double lpValue);

    void childClusterSpanningTree(
        GraphCopy & GC,
        List<edgeValue> & clusterEdges,
        List<nodePair> & MSTEdges);

    void clusterSpanningTree(
        ClusterGraph & C,
        cluster c,
        ClusterArray<List<nodePair> > & treeEdges,
        ClusterArray<List<edgeValue> > & clusterEdges);

    // Tries to improve the best primal solution by means of the current fractional LP-solution.
    double heuristicImprovePrimalBound(
        List<nodePair> & originalEdges,
        List<nodePair> & connectionEdges,
        List<edge> & deletedEdges);

    //! Adds the given constraints to the connectivity cut pool
    inline int addCutCons(ABA_BUFFER<ABA_CONSTRAINT*> cons)
    {
        return addPoolCons(cons, ((Master*)master_)->getCutConnPool());
    }
    //! Adds the given constraints to the planarity cut pool
    inline int addKuraCons(ABA_BUFFER<ABA_CONSTRAINT*> cons)
    {
        return addPoolCons(cons, ((Master*)master_)->getCutKuraPool());
    }

    //tries to regenerate connectivity cuts
    inline int separateConnPool(double minViolation)
    {
        return separateCutPool(((Master*)master_)->getCutConnPool(), minViolation);
    }
    //tries to regenerate kuratowski cuts
    inline int separateKuraPool(double minViolation)
    {
        return separateCutPool(((Master*)master_)->getCutKuraPool(), minViolation);
    }
    /*
    void minimumSpanningTree(
        GraphCopy &GC,
        List<nodePair> &clusterEdges,
        List<nodePair> &MSTEdges);

    void recursiveMinimumSpanningTree(
        ClusterGraph &C,
        cluster c,
        ClusterArray<List<nodePair> > &treeEdges,
        List<nodePair> &edgesByIncLPValue,
        List<node> &clusterNodes);

    double heuristicImprovePrimalBoundDet(
        List<nodePair> &origEdges,
        List<nodePair> &conEdges,
        List<nodePair> &delEdges);
        */

};

}//end namespace

#endif
