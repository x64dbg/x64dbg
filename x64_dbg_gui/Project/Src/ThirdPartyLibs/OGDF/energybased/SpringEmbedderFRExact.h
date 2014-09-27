/*
 * $Revision: 2524 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-03 09:54:22 +0200 (Tue, 03 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of Spring-Embedder (Fruchterman,Reingold)
 *        algorithm with exact force computations.
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

#ifndef OGDF_SPRING_EMBEDDER_FR_EXACT_H
#define OGDF_SPRING_EMBEDDER_FR_EXACT_H


#include <ogdf/module/ForceLayoutModule.h>
#include <ogdf/basic/SList.h>


namespace ogdf
{


class OGDF_EXPORT GraphCopyAttributes;
class OGDF_EXPORT GraphCopy;


class OGDF_EXPORT SpringEmbedderFRExact : public ForceLayoutModule
{
public:
    enum CoolingFunction { cfFactor, cfLogarithmic };

    //! Creates an instance of Fruchterman/Reingold (exact) layout.
    SpringEmbedderFRExact();

    // destructor
    ~SpringEmbedderFRExact() { }


    //! Calls the layout algorithm for graph attributes \a GA.
    void call(GraphAttributes & GA);


    //! Returns the current setting of iterations.
    int iterations() const
    {
        return m_iterations;
    }

    //! Sets the number of iterations to \a i.
    void iterations(int i)
    {
        OGDF_ASSERT(i > 0)
        m_iterations = i;
    }

    //! Returns the current setting of nodes.
    bool noise() const
    {
        return m_noise;
    }

    //! Sets the parameter noise to \a on.
    void noise(bool on)
    {
        m_noise = on;
    }

    //! Switches use of node weights given in GraphAttributtes
    void nodeWeights(bool on)
    {
        m_useNodeWeight = on;
    }
    //! Returns the current setting for the cooling function.
    CoolingFunction coolingFunction() const
    {
        return m_coolingFunction;
    }

    //! Sets the parameter coolingFunction to \a f.
    void coolingFunction(CoolingFunction f)
    {
        m_coolingFunction = f;
    }

    //! Returns the ideal edge length.
    double idealEdgeLength() const
    {
        return m_idealEdgeLength;
    }

    //! Sets the ideal edge length to \a len.
    void idealEdgeLength(double len)
    {
        m_idealEdgeLength = len;
    }

    //! Returns the minimum distance between connected components.
    double minDistCC() const
    {
        return m_minDistCC;
    }

    //! Sets the minimum distance between connected components to \a x.
    void minDistCC(double x)
    {
        m_minDistCC = x;
    }

    //! Returns the page ratio.
    double pageRatio()
    {
        return m_pageRatio;
    }

    //! Sets the page ration to \a x.
    void pageRatio(double x)
    {
        m_pageRatio = x;
    }

    void checkConvergence(bool b)
    {
        m_checkConvergence = b;
    }
    bool checkConvergence()
    {
        return m_checkConvergence;
    }
    void convTolerance(double tol)
    {
        m_convTolerance = tol;
    }

private:
    class ArrayGraph
    {
        int m_numNodes;
        int m_numEdges;
        int m_numCC;

        GraphAttributes* m_ga;
        node* m_orig;
        Array<SList<node> > m_nodesInCC;
        NodeArray<int>      m_mapNode;

    public:
        ArrayGraph(GraphAttributes & ga);
        ~ArrayGraph();

        void initCC(int i);

        int numberOfCCs() const
        {
            return m_numCC;
        }
        int numberOfNodes() const
        {
            return m_numNodes;
        }
        int numberOfEdges() const
        {
            return m_numEdges;
        }

        node original(int v) const
        {
            return m_orig[v];
        }
        const SList<node> & nodesInCC(int i) const
        {
            return m_nodesInCC[i];
        }

        int* m_src;
        int* m_tgt;
        double* m_x;
        double* m_y;
        double* m_nodeWeight;
        //this should be part of a multilevel layout interface class later on
        bool m_useNodeWeight; //should given nodeweights be used or all set to 1.0?
    };

    double log2(double x)
    {
        return log(x) / log(2.0);
    }
    double mylog2(int x)
    {
        double l = 0.0;
        while(x > 0)
        {
            l++;
            x >>= 1;
        }
        return l / 2;
    }

    void initialize(ArrayGraph & component);
    void mainStep(ArrayGraph & component);
    void mainStep_sse3(ArrayGraph & component);

    // Fruchterman, Reingold
    //double f_att(double d) { return d*d / m_idealEdgeLength; }
    //double f_rep(double d) { return m_idealEdgeLength*m_idealEdgeLength / d; }

    // eades
    //double f_att(double d) { return 5.0 * d * log2(d/m_idealEdgeLength); }
    //double f_rep(double d) { return 20.0 / d; }

    // cooling function
    void cool(double & tx, double & ty, int & cF);

    int    m_iterations; //!< The number of iterations.
    bool   m_noise;      //!< Perform random perturbations?
    CoolingFunction m_coolingFunction; //!< The selected cooling function


    //double m_tx;
    //double m_ty;

    double m_coolFactor_x;
    double m_coolFactor_y;

    double m_idealEdgeLength; //!< The ideal edge length.
    double m_minDistCC;       //!< The minimal distance between connected components.
    double m_pageRatio;       //!< The page ratio.

    //int m_cF;
    double m_txNull;
    double m_tyNull;
    //see above at ArrayGraph
    bool m_useNodeWeight;
    bool m_checkConvergence; //<! If set to true, computation is stopped if movement falls below threshold
    double m_convTolerance; //<! Fraction of ideal edge length below which convergence is achieved
};


} // end namespace ogdf


#endif
