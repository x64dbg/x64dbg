/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declares class CircularLayout
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


#ifndef OGDF_CIRCULAR_LAYOUT_H
#define OGDF_CIRCULAR_LAYOUT_H


#include <ogdf/module/LayoutModule.h>


namespace ogdf
{

class OGDF_EXPORT GraphCopyAttributes;
struct ClusterStructure;

//! The circular layout algorithm.
/**
 * The implementation used in CircularLayout is based on
 * the following publication:
 *
 * Ugur Dogrus&ouml;z, Brendan Madden, Patrick Madden: <i>Circular %Layout in the
 * %Graph %Layout Toolkit</i>. Proc. %Graph Drawing 1996, LNCS 1190, pp. 92-100, 1997.
 *
 * <H3>Optional parameters</H3>
 * Circular layout provides the following optional parameters.
 *
 * <table>
 *   <tr>
 *     <th><i>Option</i><th><i>Type</i><th><i>Default</i><th><i>Description</i>
 *   </tr><tr>
 *     <td><i>minDistCircle</i><td>double<td>20.0
 *     <td>The minimal distance between nodes on a circle.
 *   </tr><tr>
 *     <td><i>minDistLevel</i><td>double<td>20.0
 *     <td>The minimal distance between father and child circle.
 *   </tr><tr>
 *     <td><i>minDistSibling</i><td>double<td>10.0
 *     <td>The minimal distance between circles on same level.
 *   </tr><tr>
 *     <td><i>minDistCC</i><td>double<td>20.0
 *     <td>The minimal distance between connected components.
 *   </tr><tr>
 *     <td><i>pageRatio</i><td>double<td>1.0
 *     <td>The page ratio used for packing connected components.
 *   </tr>
 * </table>
 */
class OGDF_EXPORT CircularLayout : public LayoutModule
{
public:
    //! Creates an instance of circular layout.
    CircularLayout();

    // destructor
    ~CircularLayout() { }


    /**
     *  @name The algorithm call
     *  @{
     */

    //! Computes a circular layout for graph attributes \a GA.
    void call(GraphAttributes & GA);


    /** @}
     *  @name Optional parameters
     *  @{
     */

    //! Returns the option <i>minDistCircle</i>.
    double minDistCircle() const
    {
        return m_minDistCircle;
    }

    //! Sets the option <i>minDistCircle</i> to \a x.
    void minDistCircle(double x)
    {
        m_minDistCircle = x;
    }

    //! Returns the option <i>minDistLevel</i>.
    double minDistLevel() const
    {
        return m_minDistLevel;
    }

    //! Sets the option <i>minDistLevel</i> to \a x.
    void minDistLevel(double x)
    {
        m_minDistLevel = x;
    }

    //! Returns the option <i>minDistSibling</i>.
    double minDistSibling() const
    {
        return m_minDistSibling;
    }

    //! Sets the option <i>minDistSibling</i> to \a x.
    void minDistSibling(double x)
    {
        m_minDistSibling = x;
    }

    //! Returns the option <i>minDistCC</i>.
    double minDistCC() const
    {
        return m_minDistCC;
    }

    //! Sets the option <i>minDistCC</i> to \a x.
    void minDistCC(double x)
    {
        m_minDistCC = x;
    }

    //! Returns the option <i>pageRatio</i>.
    double pageRatio() const
    {
        return m_pageRatio;
    }

    //! Sets the option <i>pageRatio</i> to \a x.
    void pageRatio(double x)
    {
        m_pageRatio = x;
    }

    //! @}

private:
    double m_minDistCircle;  //!< The minimal distance between nodes on a circle.
    double m_minDistLevel;   //!< The minimal distance between father and child circle.
    double m_minDistSibling; //!< The minimal distance between circles on same level.
    double m_minDistCC;      //!< The minimal distance between connected components.
    double m_pageRatio;      //!< The page ratio used for packing connected components.

    void doCall(GraphCopyAttributes & AG, ClusterStructure & C);

    void assignClustersByBiconnectedComponents(ClusterStructure & C);

    int sizeBC(node vB);

    void computePreferedAngles(
        ClusterStructure & C,
        const Array<double> & outerRadius,
        Array<double> & preferedAngle);

    void assignPrefAngle(ClusterStructure & C,
                         const Array<double> & outerRadius,
                         Array<double> & preferedAngle,
                         int c,
                         int l,
                         double r1);

};


} // end namespace ogdf


#endif
