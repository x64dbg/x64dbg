/** \file
 * \brief Declares class ProcrustesSubLayout
 *
 * \author Martin Gronemann
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


#ifndef OGDF_PROCRUSTES_SUB_LAYOUT_H
#define OGDF_PROCRUSTES_SUB_LAYOUT_H


#include <ogdf/module/LayoutModule.h>


namespace ogdf
{

class ProcrustesPointSet
{
public:
    //! Constructor for allocating mem for numPoints points
    ProcrustesPointSet(int numPoints);

    //! Destructor which frees mem
    ~ProcrustesPointSet();

    //! translates and scales the set such that the avg center is 0, 0 and the avg size is 1.0
    void normalize(bool flip = false);

    //! rotates the point set so it fits somehow on other
    void rotateTo(const ProcrustesPointSet & other);

    //! calculates a value how good the two point sets match
    double compare(const ProcrustesPointSet & other) const;

    //! sets ith coordinate
    void set(int i, double x, double y)
    {
        m_x[i] = x;
        m_y[i] = y;
    }

    //! returns ith x-coordinate
    double getX(int i) const
    {
        return m_x[i];
    }

    //! returns ith y-coordinate
    double getY(int i) const
    {
        return m_y[i];
    }

    //! returns the origins x
    double originX() const
    {
        return m_originX;
    }

    //! returns the origins y
    double originY() const
    {
        return m_originY;
    }

    //! returns the scale factor
    double scale() const
    {
        return m_scale;
    }

    //! returns the rotation angle
    double angle() const
    {
        return m_angle;
    }

    //! returns true if the point set is flipped by y coord
    bool isFlipped() const
    {
        return m_flipped;
    }

private:
    //! The number of points
    int m_numPoints;

    //! x coordinates
    double* m_x;

    //! y coordinates
    double* m_y;

    //! the original avg center's x when normalized
    double m_originX;

    //! the original avg center's y when normalized
    double m_originY;

    //! the scale factor
    double m_scale;

    //! if rotated, the angle
    double m_angle;

    //! if flipped then this is true
    bool m_flipped;
};

//! A simple procrustes analysis implementation
/*!
 */
class OGDF_EXPORT ProcrustesSubLayout : public LayoutModule
{
public:
    //! Creates an instance of circular layout.
    ProcrustesSubLayout(LayoutModule* pSubLayout);

    // destructor
    ~ProcrustesSubLayout() { }

    //! Computes a circular layout for graph attributes \a GA.
    void call(GraphAttributes & GA);

    //! Should the new layout scale be used or the initial scale? Default: inital
    void setScaleToInitialLayout(bool flag)
    {
        m_scaleToInitialLayout = flag;
    }

    //! Should the new layout scale be used or the initial scale?
    bool scaleToInitialLayout() const
    {
        return m_scaleToInitialLayout;
    }

private:
    //! Does a reverse transform of graph attributes by using the origin, scale and angle in pointset
    void reverseTransform(GraphAttributes & graphAttributes, const ProcrustesPointSet & pointSet);

    //! Moves all coords in graphAttributes by dx, dy
    void translate(GraphAttributes & graphAttributes, double dx, double dy);

    //! Rotates all coords in graphAttributes by angle
    void rotate(GraphAttributes & graphAttributes, double angle);

    //! Scales all coords in graphAttributes by scale
    void scale(GraphAttributes & graphAttributes, double scale);

    //! Flips all y coordinates
    void flipY(GraphAttributes & graphAttributes);

    //! copysthe coords in graph attributes to the point set
    void copyFromGraphAttributes(const GraphAttributes & graphAttributes, ProcrustesPointSet & pointSet);

    //! The layout module to call for a new layout
    LayoutModule* m_pSubLayout;

    //! option for enabling/disabling scaling to initial layout scale
    bool m_scaleToInitialLayout;
};

} // end of namespace ogdf

#endif
