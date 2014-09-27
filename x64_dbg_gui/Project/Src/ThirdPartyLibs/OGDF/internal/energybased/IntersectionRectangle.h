/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class IntersectionRectangle which realizes axis
 *        parallel rectangles.
 *
 * The class can compute the rectangle that
 * is created by the intersection of two rectangles and it can
 * compute the area of a rectangle.
 *
 * \author Rene Weiskircher
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

#ifndef OGDF_INTERSECTION_RECTANGLE_H
#define OGDF_INTERSECTION_RECTANGLE_H


#include <ogdf/basic/geometry.h>
#include <math.h>


namespace ogdf
{


class OGDF_EXPORT IntersectionRectangle
{

private:

    DPoint m_p1; // lower left Point
    DPoint m_p2; // upper right Point
    double m_area;
    DPoint m_center;

public:

    // constructs zero area rectangle
    IntersectionRectangle() : m_p1(), m_p2(), m_area(0.0), m_center() { }

    //constructs rectangle with diagonal from p1 to p2
    IntersectionRectangle(const DPoint & p1, const DPoint & p2) : m_p1(p1), m_p2(p2)
    {
        init();
    }

    //copy constructor
    IntersectionRectangle(const IntersectionRectangle & dr) :
        m_p1(dr.m_p1), m_p2(dr.m_p2), m_area(dr.m_area), m_center(dr.m_center) { }

    //constructs rectangle with diagonal from (x1,y1) to (x2,y2)
    IntersectionRectangle(double x1, double y1, double x2, double y2)
    {
        m_p1.m_x = x1;
        m_p1.m_y = y1;
        m_p2.m_x = x2;
        m_p2.m_y = y2;
        init();
    }

    //constructs rectangle with diagonal dl
    IntersectionRectangle(const DLine & dl) : m_p1(dl.start()), m_p2(dl.end())
    {
        init();
    }

    // constructs a rectangle from the center point, width and height
    IntersectionRectangle(const DPoint &, double , double);

    // returns true if two rectangles have the same coordinates
    bool operator==(const IntersectionRectangle & dr) const
    {
        return m_p1 == dr.m_p1 && m_p2 == dr.m_p2;
    }

    // returns true if two rectangles have different coordinates
    bool operator!=(const IntersectionRectangle & dr) const
    {
        return !(*this == dr);
    }

    // assignment
    IntersectionRectangle & operator= (const IntersectionRectangle & dr)
    {
        if(this != &dr)    // don't assign myself
        {
            m_p1 = dr.m_p1;
            m_p2 = dr.m_p2;
            m_center = dr.m_center;
            m_area = dr.m_area;
        }
        return *this;
    }

    // returns the width of the rectangle
    double width() const
    {
        return m_p2.m_x - m_p1.m_x;
    }

    //returns the height of the rectangle
    double height() const
    {
        return m_p2.m_y - m_p1.m_y;
    }

    //returns the center of the rectangle
    DPoint center() const
    {
        return m_center;
    }

    //returns the area of the rectangle
    double area() const
    {
        return m_area;
    }


    // returns rect-defining vertices
    const DPoint & p1() const
    {
        return m_p1;
    }
    const DPoint & p2() const
    {
        return m_p2;
    }

    // tests if p is inside the rectangle modulo the comparison epsilon
    bool inside(const DPoint & p) const
    {
        if((p.m_x + OGDF_GEOM_EPS) < m_p1.m_x ||
                (p.m_x - OGDF_GEOM_EPS) > m_p2.m_x ||
                (p.m_y + OGDF_GEOM_EPS) < m_p1.m_y ||
                (p.m_y - OGDF_GEOM_EPS) > m_p2.m_y)
            return false;
        return true;
    }

    // tests if *this and the argument rectangle intersect
    bool intersects(const IntersectionRectangle &) const;

    // returns the rectangle resulting from intersection of this and argument.
    // Returns a rectangle with zero width and height and center (0,0) if intersection
    // is empty.
    IntersectionRectangle intersection(const IntersectionRectangle &) const;

    // computes distance between two rectangles
    double distance(const IntersectionRectangle &) const;

    //moves the rectangle such that its center is at the given point
    void move(const DPoint &);

private:
    // makes sure, that m_p1 <= m_p2, default after construction, sets area and center
    void init();

    // swaps the two y-coordinates
    void yInvert()
    {
        swap(m_p1.m_y, m_p2.m_y);
    }

    // swaps the two x-coordinates
    void xInvert()
    {
        swap(m_p1.m_x, m_p2.m_x);
    }

    // functions for computing bounding lines
    DLine bottom() const
    {
        return DLine(m_p1.m_x, m_p1.m_y, m_p2.m_x, m_p1.m_y);
    }
    DLine top() const
    {
        return DLine(m_p1.m_x, m_p2.m_y, m_p2.m_x, m_p2.m_y);
    }
    DLine left() const
    {
        return DLine(m_p1.m_x, m_p1.m_y, m_p1.m_x, m_p2.m_y);
    }
    DLine right() const
    {
        return DLine(m_p2.m_x, m_p1.m_y, m_p2.m_x, m_p2.m_y);
    }

    // computes distance between parallel line segments
    double parallelDist(const DLine &, const DLine &) const;

    // computes distance between two points
    double pointDist(const DPoint & p1, const DPoint & p2) const
    {
        return sqrt((p1.m_y - p2.m_y) * (p1.m_y - p2.m_y) + (p1.m_x - p2.m_x) * (p1.m_x - p2.m_x));
    }

    friend ostream & operator<<(ostream &, const IntersectionRectangle &);
};

/*
//the point comparer is needed for sorting points and storing them in
//sorted sequences
class PointComparer {
public:
    static int compare(const DPoint &p1, const DPoint &p2) {
        if(p1.m_x > p2.m_x) return 1;
        if(p1.m_x < p2.m_x) return -1;
        if(p1.m_y > p2.m_y) return 1;
        if(p1.m_y < p2.m_y) return -1;
        return 0;
    }
    OGDF_AUGMENT_STATICCOMPARER(DPoint)
};
*/

}
#endif
