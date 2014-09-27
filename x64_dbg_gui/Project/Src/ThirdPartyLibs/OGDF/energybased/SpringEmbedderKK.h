/*
 * $Revision: 2524 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-03 09:54:22 +0200 (Tue, 03 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of Spring-Embedder algorithm (Kamada,Kawai).
 *
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

#ifndef OGDF_SPRING_EMBEDDER_KK_H
#define OGDF_SPRING_EMBEDDER_KK_H


#include <ogdf/module/LayoutModule.h>
#include <ogdf/basic/Array2D.h>
#include <ogdf/basic/tuples.h>


namespace ogdf
{


class OGDF_EXPORT GraphCopyAttributes;
class OGDF_EXPORT GraphCopy;


//! The spring-embedder layout algorithm by Kamada and Kawai.
/**
 * The implementation used in SpringEmbedderKK is based on
 * the following publication:
 *
 * Tomihisa Kamada, Satoru Kawai: <i>%An Algorithm for Drawing
 * General Undirected Graphs</i>. Information Processing Letters 31, pp. 7-15, 1989.
 *
 * Precondition: The input graph has to be connected.
 * <H3>Optional parameters</H3>
 * There are some parameters that can be tuned to optimize the
 * algorithm's behavior regarding runtime and layout quality.
 * First of all note that the algorithm uses all pairs shortest path
 * to compute the graph theoretic distance. This can be done either
 * with BFS (ignoring node sizes) in quadratic time or by using
 * e.g. Floyd's algorithm in cubic time with given edge lengths
 * that may reflect the node sizes.  Also m_computeMaxIt decides
 * if the computation is stopped after a fixed maximum number of
 * iterations. The desirable edge length can either be set or computed
 * from the graph and the given layout.
 * Kamada-Kawai layout provides the following optional parameters.
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>tolerance</i><td>int<td>0.0001
 *     <td>Tolerance for the energy level (below which the main loop stops).
 *   </tr>
 * </table>
 */
class OGDF_EXPORT SpringEmbedderKK : public LayoutModule
{
public:
    typedef Tuple2<double, double> dpair;

    //! The scaling method used for the desirable length.
    //! TODO: Non-functional so far, scScaleFunction is used
    enum Scaling
    {
        scInput,           //!< bounding box of input is used.
        scUserBoundingBox, //!< bounding box set by userBoundingBox() is used.
        scScaleFunction,    //!< automatic scaling is used, computed using only the node sizes.
        scScaleAdaptive    //!< automatic scaling is used, adapting the value per iteration.
    };

    //! Constructor: Constructs instance of Kamada Kawai Layout
    SpringEmbedderKK() : m_tolerance(0.001), m_ltolerance(0.0001), m_computeMaxIt(true),
        m_K(5.0), m_desLength(0.0), m_distFactor(2.0), m_useLayout(true),
        m_gItBaseVal(50), m_gItFactor(16)
    {
        m_maxLocalIt = m_maxGlobalIt = maxVal;
    }

    //! Destructor
    ~SpringEmbedderKK() {}

    //! Calls the layout algorithm for graph attributes \a GA.
    //! Currently, GA.doubleWeight is NOT used to allow simple
    //! distinction of BFS/APSS. Precondition: Graph is connected.
    void call(GraphAttributes & GA);
    //! Calls the layout algorithm for graph attributes \a GA
    //! using values in eLength for distance computation.
    //! Precondition: Graph is connected.
    void call(GraphAttributes & GA, const EdgeArray<double> & eLength);

    //! Sets the value for the stop tolerance, below which the
    //! system is regarded stable (balanced) and the optimization stopped
    void setStopTolerance(double s)
    {
        m_tolerance = s;
    }

    //! If set to true, the given layout is used for the initial positions
    void setUseLayout(bool b)
    {
        m_useLayout = b;
    }
    bool useLayout()
    {
        return m_useLayout;
    }

    //! If set != 0, value zerolength is used to determine the
    //! desirable edge length by L = zerolength / max distance_ij.
    //! Otherwise, zerolength is determined using the node number and sizes.
    void setZeroLength(double d)
    {
        m_zeroLength = d;
    }
    double zeroLength()
    {
        return m_zeroLength;
    }

    //! Sets desirable edge length directly
    void setDesLength(double d)
    {
        m_desLength = d;
    }


    //! It is possible to limit the number of iterations to a fixed value
    //! Returns the current setting of iterations.
    //! These values are only used if m_computeMaxIt is set to true.
    int maxLocalIterations() const
    {
        return m_maxLocalIt;
    }
    void setGlobalIterationFactor(int i)
    {
        if(i > 0) m_gItFactor = i;
    }
    int maxGlobalIterations() const
    {
        return m_maxGlobalIt;
    }
    //! Sets the number of global iterations to \a i.
    void setMaxGlobalIterations(int i)
    {
        if(i > 0)
            m_maxGlobalIt = i;
    }
    //! Sets the number of local iterations to \a i.
    void setMaxLocalIterations(int i)
    {
        if(i > 0)
            m_maxLocalIt = i;
    }
    //! If set to true, number of iterations is computed depending on G
    void computeMaxIterations(bool b)
    {
        m_computeMaxIt = b;
    }
    //We could add some noise to the computation
    // Returns the current setting of nodes.
    //bool noise() const {
    //  return m_noise;
    //}
    // Sets the parameter noise to \a on.
    //void noise(bool on) {
    //  m_noise = on;
    //}


protected:
    //! Does the actual call
    void doCall(GraphAttributes & GA, const EdgeArray<double> & eLength, bool simpleBFS);
    //! Checks if main loop is finished because local optimum reached
    bool finished(double maxdelta)
    {
        if(m_prevEnergy == startVal)  //first step
        {
            m_prevEnergy = maxdelta;
            return false;
        }

        double diff = m_prevEnergy - maxdelta; //energy difference
        if(diff < 0.0) diff = -diff;
        //#ifdef OGDF_DEBUG
        //        cout << "Finished(): maxdelta: "<< maxdelta<<" diff/prev: "<<diff / m_prevEnergy<<"\n";
        //#endif
        //check if we want to stop
        bool done = (maxdelta < m_tolerance);// || (diff / m_prevEnergy) < m_tolerance);

        m_prevEnergy = maxdelta;   //save previous energy level
        m_prevLEnergy =  startVal; //reset energy level for local node decision

        return done;
    }//finished
    //! Checks if inner loop (current node) is finished
    bool finishedNode(double deltav)
    {
        if(m_prevLEnergy == startVal)
        {
            m_prevLEnergy = deltav;
            return deltav == 0.0;//<m_ltolerance; //locally stable
        }
        //#ifdef OGDF_DEBUG
        //        cout << "Local delta: "<<deltav<<"\n";
        //#endif
        double diff = m_prevLEnergy - deltav;
        //check if we want to stop
        bool done = (deltav == 0.0 || (diff / m_prevLEnergy) < m_ltolerance);

        m_prevLEnergy = deltav; //save previous energy level

        return done;
    }//finishedNode
    //! Changes given edge lengths (interpreted as weight factors)
    //! according to additional parameters like node size etc.
    void adaptLengths(const Graph & G,
                      const GraphAttributes & GA,
                      const EdgeArray<double> & eLengths,
                      EdgeArray<double> & adaptedLengths);
    //! Adapts positions to avoid degeneracy (all nodes on a single point)
    void shufflePositions(GraphAttributes & GA);
    //! Computes contribution of node u to the first partial
    //! derivatives (dE/dx_m, dE/dy_m) (for node m) (eq. 7 and 8 in paper)
    dpair computeParDer(node m,
                        node u,
                        GraphAttributes & GA,
                        NodeArray< NodeArray<double> > & ss,
                        NodeArray< NodeArray<double> > & dist);
    //! Compute partial derivative for v
    dpair computeParDers(node v,
                         GraphAttributes & GA,
                         NodeArray< NodeArray<double> > & ss,
                         NodeArray< NodeArray<double> > & dist);
    //! Does the necessary initialization work for the call functions
    void initialize(GraphAttributes & GA,
                    NodeArray<dpair> & partialDer,
                    const EdgeArray<double> & eLength,
                    NodeArray< NodeArray<double> > & oLength,
                    NodeArray< NodeArray<double> > & sstrength,
                    double & maxDist,
                    bool simpleBFS);
    //! Main computation loop, nodes are moved here
    void mainStep(GraphAttributes & GA,
                  NodeArray<dpair> & partialDer,
                  NodeArray< NodeArray<double> > & oLength,
                  NodeArray< NodeArray<double> > & sstrength,
                  const double maxDist);
    //! Does the scaling if no edge lengths are given but node sizes
    //! are respected
    void scale(GraphAttributes & GA);

private:
    //! The stop criterion when the forces of all strings are
    //! considered to be balanced
    double m_tolerance;  //!<value for stop criterion
    double m_ltolerance;  //!<value for local stop criterion
    int m_maxGlobalIt;   //!< Maximum number of global iterations
    int m_maxLocalIt;   //!< Maximum number of local iterations
    bool m_computeMaxIt; //!< If true, number of iterations is computed
    //! depending on number of nodes
    double m_K; //! Big K constant for strength computation
    double m_prevEnergy; //!<max energy value
    double m_prevLEnergy;//!<local energy
    double m_zeroLength; //!< Length of a side of the display area, used for
    //! edge length computation if > 0
    double m_desLength; //!< Desirable edge length, used instead if > 0
    double m_distFactor; //introduces some distance for scaling in case BFS is used

    bool m_useLayout; //!< use positions or allow to shuffle nodes to
    //!< avoid degeneration
    int m_gItBaseVal; //!< minimum number of global iterations
    int m_gItFactor;  //!< factor for global iterations: m_gItBaseVal+m_gItFactor*|V|

    static const double startVal;
    static const double minVal;
    static const double desMinLength; //!< Defines minimum desired edge length.
    //! Smaller values are treated as zero
    static const int maxVal = INT_MAX; //! defines infinite upper bound for iteration number

    double allpairsspBFS(const Graph & G, NodeArray< NodeArray<double> > & distance);
    double allpairssp(const Graph & G, const EdgeArray<double> & eLengths,
                      NodeArray< NodeArray<double> > & distance,   const double threshold = DBL_MAX);
};//SpringEmbedderKK

//Things that potentially could be added
//  //! Returns the page ratio.
//    double pageRatio() { return m_pageRatio; }
//
//  //! Sets the page ration to \a x.
//    void pageRatio(double x) { m_pageRatio = x; }
//
//  //! Returns the current scaling method.
//  Scaling scaling() const {
//      return m_scaling;
//  }
//
//  //! Sets the method for scaling the inital layout to \a sc.
//  void scaling(Scaling sc) {
//      m_scaling = sc;
//  }
//
//  //! Returns the current scale function factor.
//  double scaleFunctionFactor() const {
//      return m_scaleFactor;
//  }
//
//  //! Sets the scale function factor to \a f.
//  void scaleFunctionFactor(double f) {
//      m_scaleFactor = f;
//  }
//
//  //! Sets the user bounding box (used if scaling method is scUserBoundingBox).
//  void userBoundingBox(double xmin, double ymin, double xmax, double ymax) {
//      m_bbXmin = xmin;
//      m_bbYmin = ymin;
//      m_bbXmax = xmax;
//      m_bbYmax = ymax;
//  }


} // end namespace ogdf


#endif
