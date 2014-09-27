/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of Spring-Embedder (Fruchterman,Reingold)
 *        algorithm.
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

#ifndef OGDF_SPRING_EMBEDDER_FR_H
#define OGDF_SPRING_EMBEDDER_FR_H


#include <ogdf/module/LayoutModule.h>
#include <ogdf/basic/Array2D.h>


namespace ogdf
{


class OGDF_EXPORT GraphCopyAttributes;
class OGDF_EXPORT GraphCopy;


//! The spring-embedder layout algorithm by Fruchterman and Reingold.
/**
 * The implementation used in SpringEmbedderFR is based on
 * the following publication:
 *
 * Thomas M. J. Fruchterman, Edward M. Reingold: <i>%Graph Drawing by Force-directed
 * Placement</i>. Software - Practice and Experience 21(11), pp. 1129-1164, 1991.
 *
 * <H3>Optional parameters</H3>
 * Fruchterman/Reingold layout provides the following optional parameters.
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>iterations</i><td>int<td>400
 *     <td>The number of iterations performed in the optimization.
 *   </tr><tr>
 *     <td><i>noise</i><td>bool<td>true
 *     <td>If set to true, (small) random perturbations are performed.
 *   </tr><tr>
 *     <td><i>minDistCC</i><td>double<td>20.0
 *     <td>The minimum distance between connected components.
 *   </tr><tr>
 *     <td><i>pageRatio</i><td>double<td>1.0
 *     <td>The page ratio.
 *   </tr><tr>
 *     <td><i>scaling</i><td> #Scaling <td> #scScaleFunction
 *     <td>The scaling method for scaling the inital layout.
 *   </tr><tr>
 *     <td><i>scaleFunctionFactor</i><td>double<td>8.0
 *     <td>The scale function factor (used if scaling = scScaleFunction).
 *   </tr><tr>
 *     <td><i>userBoundingBox</i><td>rectangle<td>(0.0,100.0,0.0,100.0)
 *     <td>The user bounding box for scaling (used if scaling = scUserBoundingBox).
 *   </tr>
 * </table>
 */
class OGDF_EXPORT SpringEmbedderFR : public LayoutModule
{
public:
    //! The scaling method used by the algorithm.
    enum Scaling
    {
        scInput,           //!< bounding box of input is used.
        scUserBoundingBox, //!< bounding box set by userBoundingBox() is used.
        scScaleFunction    //!< automatic scaling is used with parameter set by scaleFunctionFactor() (larger factor, larger b-box).
    };


    //! Creates an instance of Fruchterman/Reingold layout.
    SpringEmbedderFR();

    // destructor
    ~SpringEmbedderFR() { }


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
        if(i > 0)
            m_iterations = i;
    }

    double fineness() const
    {
        return m_fineness;
    }

    void fineness(double f)
    {
        m_fineness = f;
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

    //! Returns the current scaling method.
    Scaling scaling() const
    {
        return m_scaling;
    }

    //! Sets the method for scaling the inital layout to \a sc.
    void scaling(Scaling sc)
    {
        m_scaling = sc;
    }

    //! Returns the current scale function factor.
    double scaleFunctionFactor() const
    {
        return m_scaleFactor;
    }

    //! Sets the scale function factor to \a f.
    void scaleFunctionFactor(double f)
    {
        m_scaleFactor = f;
    }

    //! Sets the user bounding box (used if scaling method is scUserBoundingBox).
    void userBoundingBox(double xmin, double ymin, double xmax, double ymax)
    {
        m_bbXmin = xmin;
        m_bbYmin = ymin;
        m_bbXmax = xmax;
        m_bbYmax = ymax;
    }

private:
    bool initialize(GraphCopy & G, GraphCopyAttributes & AG);

    void mainStep(GraphCopy & G, GraphCopyAttributes & AG);
    void cleanup()
    {
        delete m_A;
        m_A = 0;
    }

    NodeArray<ListIterator<node> > m_lit;

    int m_cF;

    double m_width;
    double m_height;

    double m_txNull;
    double m_tyNull;
    double m_tx;
    double m_ty;

    double m_k;
    double m_k2;
    double m_kk;
    int m_ki;

    int m_xA;
    int m_yA;

    Array2D<List<node> >* m_A;


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

    int    m_iterations;  //!< The number of iterations.
    double m_fineness;    //!< The fineness of the grid.
    double m_edgeLength;

    double m_xleft;       //!< Bounding box (minimal x-coordinate).
    double m_xright;      //!< Bounding box (maximal x-coordinate).
    double m_ysmall;      //!< Bounding box (minimal y-coordinate).
    double m_ybig;        //!< Bounding box (maximal y-coordinate).

    bool m_noise;         //!< Perform random perturbations?

    Scaling m_scaling;    //!< The scaling method.
    double m_scaleFactor; //!< The factor used if scaling type is scScaleFunction.

    double m_bbXmin; //!< User bounding box (minimal x-coordinate).
    double m_bbYmin; //!< User bounding box (maximal x-coordinate).
    double m_bbXmax; //!< User bounding box (minimal y-coordinate).
    double m_bbYmax; //!< User bounding box (maximal y-coordinate).

    double m_minDistCC; //!< The minimal distance between connected components.
    double m_pageRatio; //!< The page ratio.
};


} // end namespace ogdf


#endif
