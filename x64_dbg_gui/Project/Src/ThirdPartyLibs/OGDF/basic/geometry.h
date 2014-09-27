/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of classes DPoint, DPolyline, DLine, DRect, DScaler.
 *
 * \author Joachim Kupke
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

#ifndef OGDF_GEOMETRY_H
#define OGDF_GEOMETRY_H

#include <ogdf/basic/List.h>
#include <ogdf/basic/Hashing.h>
#include <float.h>
#include <math.h>

#define OGDF_GEOM_EPS  1e-06


namespace ogdf
{

//! Determines the orientation in hierarchical layouts.
enum Orientation
{
    topToBottom, //!< Edges are oriented from top to bottom.
    bottomToTop, //!< Edges are oriented from bottom to top.
    leftToRight, //!< Edges are oriented from left to right.
    rightToLeft  //!< Edges are oriented from right to left.
};


// Important: be careful, if compared values are (+/-)DBL_MAX !!!
inline
bool DIsEqual(const double & a, const double & b, const double eps = OGDF_GEOM_EPS)
{
    return (a < (b + eps) && a > (b - eps));
}

inline
bool DIsGreaterEqual(const double & a, const double & b, const double eps = OGDF_GEOM_EPS)
{
    return (a > (b - eps));
}

inline
bool DIsGreater(const double & a, const double & b, const double eps = OGDF_GEOM_EPS)
{
    return (a > (b + eps));
}

inline
bool DIsLessEqual(const double & a, const double & b, const double eps = OGDF_GEOM_EPS)
{
    return (a < (b + eps));
}

inline
bool DIsLess(const double & a, const double & b, const double eps = OGDF_GEOM_EPS)
{
    return (a < (b - eps));
}

inline
double DRound(const double & d, int prec = 0)
{
    if(prec == 0)
        return floor(d + 0.5);
    double factor = pow(10.0, ((double) prec));
    return DRound(d * factor, 0) / factor;
}

/**
 * \brief Parameterized base class for points.
 *
 * This class serves as base class for two-dimensional points with specific
 * coordinate types like integer points (IPoint) and real points (DPoint).
 * The template parameter NUMBER is the type for the coordinates of the point
 * and has to support assignment and equality/inequality operators.
 */
template <class NUMBER>
class GenericPoint
{
public:
    //! The type for coordinates of the point.
    typedef NUMBER numberType;

    NUMBER m_x; //!< The x-coordinate.
    NUMBER m_y; //!< The y-coordinate.

    //! Creates a generic point.
    /**
     * \warning Does not assign something like zero to the coordinates,
     *          since we do not require that 0 can be casted to a NUMBER.
     */
    GenericPoint() { }

    //! Creates a generic point (\a x,\a y).
    GenericPoint(NUMBER x, NUMBER y) : m_x(x), m_y(y) { }

    //! Copy constructor.
    GenericPoint(const GenericPoint & ip) : m_x(ip.m_x), m_y(ip.m_y) { }

    //! Assignment operator.
    GenericPoint operator=(const GenericPoint & ip)
    {
        m_x = ip.m_x;
        m_y = ip.m_y;
        return *this;
    }

    //! Equality operator.
    bool operator==(const GenericPoint & ip) const
    {
        return m_x == ip.m_x && m_y == ip.m_y;
    }

    //! Inequality operator.
    bool operator!=(const GenericPoint & ip) const
    {
        return m_x != ip.m_x || m_y != ip.m_y;
    }

};//class GenericPoint


/**
 * \brief Integer points.
 *
 * This class represent a two-dimensional point with integer coordinates.
 */
class OGDF_EXPORT IPoint : public GenericPoint<int>
{
public:
    //! Creates an integer point (0,0).
    IPoint() : GenericPoint<int>(0, 0) { }

    //! Creates an integer point (\a x,\a y).
    IPoint(int x, int y) : GenericPoint<int>(x, y) { }

    //! Copy constructor.
    IPoint(const IPoint & ip) : GenericPoint<int>(ip) { }

    //! Returns the euclidean distance between \a p and this point.
    double distance(const IPoint & p) const;
};//class IPoint


//! Output operator for integer points.
OGDF_EXPORT ostream & operator<<(ostream & os, const IPoint & ip);


template<> class DefHashFunc<IPoint>
{
public:
    int hash(const IPoint & ip) const
    {
        return 7 * ip.m_x + 23 * ip.m_y;
    }
};


/**
 * \brief Polylines with integer coordinates.
 *
 * This class represents integer polylines by a list of integer points.
 * Such polylines are, e.g., used in layouts for representing bend
 * point lists. Note that in this case, only the bend points are in the
 * list and neither the start nor the end point.
 */
class OGDF_EXPORT IPolyline : public List<IPoint>
{
public:
    //! Creates an empty integer polyline.
    IPolyline() { }

    //! Copy constructor.
    IPolyline(const IPolyline & ipl) : List<IPoint>(ipl) { }

    //! Assignment operator.
    IPolyline & operator=(const IPolyline & ipl)
    {
        List<IPoint>::operator =(ipl);
        return *this;
    }

    //! Returns the euclidean length of the polyline.
    double length() const;
};



/**
 * \brief Real points.
 *
 * This class represent a two-dimensional point with real coordinates.
 */
class OGDF_EXPORT DPoint : public GenericPoint<double>
{
public:
    //! Creates a real point (0,0).
    DPoint() : GenericPoint<double>(0, 0) { }

    //! Creates a real point (\a x,\a y).
    DPoint(double x, double y) : GenericPoint<double>(x, y) { }

    //! Copy constructor.
    DPoint(const DPoint & dp) : GenericPoint<double>(dp) { }

    //! Relaxed equality operator.
    bool operator==(const DPoint & dp) const
    {
        return DIsEqual(m_x, dp.m_x) && DIsEqual(m_y, dp.m_y);
    }

    //! Returns the norm of the point.
    double norm() const
    {
        return sqrt(m_x * m_x + m_y * m_y);
    }

    //! Addition of real points.
    DPoint operator+(const DPoint & p) const;

    //! Subtraction of real points.
    DPoint operator-(const DPoint & p) const;

    //! Returns the euclidean distance between \a p and this point.
    double distance(const DPoint & p) const;
};

//! Output operator for real points.
OGDF_EXPORT ostream & operator<<(ostream & os, const DPoint & dp);


/**
 * \brief Vectors with real coordinates.
 */
class OGDF_EXPORT DVector : public DPoint
{
public:

    //! Creates a vector (0,0).
    DVector() : DPoint() { }

    //! Creates a vector (\a x,\a y).
    DVector(double x, double y) : DPoint(x, y) { }

    //! Copy constructor.
    DVector(const DVector & dv) : DPoint(dv) { }

    //! Assignment operator.
    DVector operator=(const DPoint & ip)
    {
        if(this != &ip)
        {
            m_x = ip.m_x;
            m_y = ip.m_y;
        }
        return *this;
    }

    //! Multiplies all coordinates with \a val.
    DVector operator*(const double val) const;

    //! Divides all coordinates by \a val.
    DVector operator/(const double val) const;

    //! Returns the length of the vector.
    double length() const;

    //! Returns the determinante of the vector.
    double operator^(const DVector & dv) const;

    //! Returns the scalar product of this vecor and \a dv.
    double operator*(const DVector & dv) const;

    /**
    * \brief Returns a vector that is orthogonal to this vector.
    *
    * Returns the vector \f$(y/x,1)\f$ if \f$x\neq 0\f$, or \f$(1,0)\f$
    * otherwise, where \f$(x,y)\f$ is this vector.
    */
    DVector operator++() const;

    /**
    * \brief Returns a vector that is orthogonal to this vector.
    *
    * Returns the vector \f$(-y/x,-1)\f$ if \f$x\neq 0\f$, or \f$(-1,0)\f$
    * otherwise, where \f$(x,y)\f$ is this vector.
    */
    DVector operator--() const;
};



/**
 * \brief Polylines with real coordinates.
 *
 * This class represents real polylines by a list of real points.
 * Such polylines are, e.g., used in layouts for representing bend
 * point lists.
 */
class OGDF_EXPORT DPolyline : public List<DPoint>
{
    static const double s_prec; //!< The conversion-precision.
public:
    //! Creates an empty integer polyline.
    DPolyline() { }

    //! Copy constructor.
    DPolyline(const DPolyline & dpl) : List<DPoint>(dpl) { }

    //! Assignment operator.
    DPolyline & operator=(const DPolyline & dpl)
    {
        List<DPoint>::operator =(dpl);
        return *this;
    }

    //! Returns the euclidean length of the polyline.
    double length() const;

    /**
     * \brief Returns a point on the polyline which is \a fraction * \a len
     *        away from the start point.
     *
     * @param fraction defines the fraction of \a lento be considered.
     * @param len is the given length, or the length of the polyline if \a len < 0.
     */
    DPoint position(const double fraction, double len = -1.0) const;

    //! Writes the polyline as graph in gml-format to file \a filename.
    void writeGML(const char* filename) const;

    //! Writes the polyline as graph in gml-format to output stream \a stream.
    void writeGML(ostream & stream) const;

    //! Deletes all successive points with equal coordinates.
    void unify();

    //! Deletes all redundant points on the polyline that lie on a straight line given by their adajcent points.
    void normalize();

    //! Deletes all redundant points on the polyline that lie on a straight line given by their adajcent points.
    void normalize(DPoint src, //start point of the edge
                   DPoint tgt); //end point of the edge

    //! Converts all coordinates rounded to \a s_prec decimal digits.
    void convertToInt();

    //void reConvertToDouble();
};


/**
 * \brief Lines with real coordinates.
 */
class OGDF_EXPORT DLine
{

protected:
    DPoint m_start; //!< The start point of the line.
    DPoint m_end;   //!< The end point of the line.

public:

    //! Creates an empty line.
    DLine() : m_start(), m_end() {}

    //! Creates a line with start point \a p1 and end point \a p2.
    DLine(const DPoint & p1, const DPoint & p2) : m_start(p1), m_end(p2) {}

    //! Copy constructor.
    DLine(const DLine & dl) : m_start(dl.m_start), m_end(dl.m_end) {}

    //! Creates a line with start point (\a x1,\a y1) and end point (\a x2,\a y2).
    DLine(double x1, double y1, double x2, double y2)
    {
        m_start.m_x = x1;
        m_start.m_y = y1;
        m_end.m_x = x2;
        m_end.m_y = y2;
    }

    //! Equality operator.
    bool operator==(const DLine & dl) const
    {
        return m_start == dl.m_start && m_end == dl.m_end;
    }

    //! Inequality operator.
    bool operator!=(const DLine & dl) const
    {
        return !(*this == dl);
    }

    //! Assignment operator.
    DLine & operator= (const DLine & dl)
    {
        if(this != &dl)    // don't assign myself
        {
            m_start = dl.m_start;
            m_end   = dl.m_end;
        }
        return *this;
    }

    //! Returns the start point of the line.
    const DPoint & start() const
    {
        return m_start;
    }

    //! Returns the end point of the line.
    const DPoint & end() const
    {
        return m_end;
    }

    //! Returns the x-coordinate of the difference (end point - start point).
    double dx() const
    {
        return m_end.m_x - m_start.m_x;
    }

    //! Returns the y-coordinate of the difference (end point - start point).
    double dy() const
    {
        return m_end.m_y - m_start.m_y;
    }

    //! Returns the slope of the line.
    double slope() const
    {
        return (dx() == 0) ? DBL_MAX : dy() / dx();
    }

    //! Returns the value y' such that (0,y') lies on the unlimited straight-line define dby this line.
    double yAbs() const
    {
        return (dx() == 0) ? DBL_MAX : m_start.m_y - (slope() * m_start.m_x);
    }

    //! Returns true iff this line runs vertically.
    bool isVertical()   const
    {
        return (DIsEqual(dx(), 0.0));
    }

    //! Returns true iff this line runs horizontally.
    bool isHorizontal() const
    {
        return (DIsEqual(dy(), 0.0));
    }

    /**
     * \brief Returns true iff \a line and this line intersect.
     *
     * @param line is the second line.
     * @param inter is assigned  the intersection point if true is returned.
     * @param endpoints determines if common endpoints are treated as intersection.
     */
    bool intersection(const DLine & line, DPoint & inter, bool endpoints = true) const;

    //! Returns true iff \a p lie on this line.
    bool contains(const DPoint & p) const;

    //! Returns the length (euclidean distance between start and edn point) of this line.
    double length() const
    {
        return m_start.distance(m_end);
    }

    /**
     * \brief Computes the intersection between this line and the horizontal line through y = \a horAxis.
     *
     * @param horAxis defines the horizontal line.
     * @param crossing is assigned the x-coordinate of the intersection point.
     *
     * \return the number of intersection points (0 = none, 1 = one, 2 = this
     *         line lies on the horizontal line through y = \a horAxis).
     */
    int horIntersection(const double horAxis, double & crossing) const;

    // gives the intersection with the vertical axis 'verAxis', returns the number of intersections
    // 0 = no, 1 = one, 2 = infinity or both end-points, e.g. parallel on this axis
    /**
     * \brief Computes the intersection between this line and the vertical line through x = \a verAxis.
     *
     * @param verAxis defines the vertical line.
     * @param crossing is assigned the y-coordinate of the intersection point.
     *
     * \return the number of intersection points (0 = none, 1 = one, 2 = this
     *         line lies on the vertical line through x = \a verAxis).
     */
    int verIntersection(const double verAxis, double & crossing) const;
};

//! Output operator for lines.
ostream & operator<<(ostream & os, const DLine & dl);


/**
 * \brief Rectangles with real coordinates.
 */
class OGDF_EXPORT DRect
{

private:
    DPoint m_p1; //!< The lower left point of the rectangle.
    DPoint m_p2; //!< The upper right point of the rectangle.

public:
    //! Creates a rectangle with lower left and upper right point (0,0).
    DRect() : m_p1(), m_p2() {}

    //! Creates a rectangle with lower left point \a p1 and upper right point \a p2.
    DRect(const DPoint & p1, const DPoint & p2) : m_p1(p1), m_p2(p2)
    {
        normalize();
    }

    //! Creates a rectangle with lower left point (\a x1,\a y1) and upper right point (\a x1,\a y2).
    DRect(double x1, double y1, double x2, double y2)
    {
        m_p1.m_x = x1;
        m_p1.m_y = y1;
        m_p2.m_x = x2;
        m_p2.m_y = y2;
        normalize();
    }

    //! Creates a rectangle defined by the end points of line \a dl.
    DRect(const DLine & dl) : m_p1(dl.start()), m_p2(dl.end())
    {
        normalize();
    }

    //! Copy constructor.
    DRect(const DRect & dr) : m_p1(dr.m_p1), m_p2(dr.m_p2)
    {
        normalize();
    }

    //! Equality operator.
    bool operator==(const DRect & dr) const
    {
        return m_p1 == dr.m_p1 && m_p2 == dr.m_p2;
    }

    //! Inequality operator.
    bool operator!=(const DRect & dr) const
    {
        return !(*this == dr);
    }

    //! Assignment operator.
    DRect & operator= (const DRect & dr)
    {
        if(this != &dr)    // don't assign myself
        {
            m_p1 = dr.m_p1;
            m_p2 = dr.m_p2;
        }
        return *this;
    }

    //! Returns the width of the rectangle.
    double width() const
    {
        return m_p2.m_x - m_p1.m_x;
    }

    //! Returns the height of the rectangle.
    double height() const
    {
        return m_p2.m_y - m_p1.m_y;
    }

    /**
     * \brief Normalizes the rectangle.
     *
     * Makes sure that the lower left point lies below and left of the upper
     * right point.
     */
    void normalize()
    {
        if(width() < 0)  swap(m_p2.m_x, m_p1.m_x);
        if(height() < 0) swap(m_p2.m_y, m_p1.m_y);
    }

    //! Returns the lower left point of the rectangle.
    const DPoint & p1() const
    {
        return m_p1;
    }

    //! Returns the upper right point of the rectangle.
    const DPoint & p2() const
    {
        return m_p2;
    }

    //! Returns the top side of the rectangle.
    const DLine topLine() const
    {
        return DLine(DPoint(m_p1.m_x, m_p2.m_y), DPoint(m_p2.m_x, m_p2.m_y));
    }

    //! Returns the right side of the rectangle.
    const DLine rightLine() const
    {
        return DLine(DPoint(m_p2.m_x, m_p2.m_y), DPoint(m_p2.m_x, m_p1.m_y));
    }

    //! Returns the left side of the rectangle.
    const DLine leftLine() const
    {
        return DLine(DPoint(m_p1.m_x, m_p1.m_y), DPoint(m_p1.m_x, m_p2.m_y));
    }

    //! Returns the bottom side of the rectangle.
    const DLine bottomLine() const
    {
        return DLine(DPoint(m_p2.m_x, m_p1.m_y), DPoint(m_p1.m_x, m_p1.m_y));
    }

    //! Swaps the y-coordinates of the two points.
    void yInvert()
    {
        swap(m_p1.m_y, m_p2.m_y);
    }

    //! Swaps the x-coordinates of the two points.
    void xInvert()
    {
        swap(m_p1.m_x, m_p2.m_x);
    }

    //! Returns true iff \a p lies within this rectangle.
    bool contains(const DPoint & p) const
    {
        if(DIsLess(p.m_x, m_p1.m_x) ||
                DIsGreater(p.m_x, m_p2.m_x) ||
                DIsLess(p.m_y, m_p1.m_y) ||
                DIsGreater(p.m_y, m_p2.m_y))
            return false;
        return true;
    }
};

//! Output operator for rectangles.
OGDF_EXPORT ostream & operator<<(ostream & os, const DRect & dr);


/**
* \brief Scaling between coordinate systems.
*/
class OGDF_EXPORT DScaler
{

private:

    const DRect* m_from; //!< Rectangluar area in source coordinate system.
    const DRect* m_to; //!< Rectangluar area in target coordinate system.

    double m_factorX; //!< The scaling factor for the x-coordinates.
    double m_factorY; //!< The scaling factor for the y-coordinates.
    double m_offsetX; //!< The offset for the x-coordinates.
    double m_offsetY; //!< The offset for the y-coordinates.

public:
    //! Creates a scaler for scaling from area \a from to area \a to.
    DScaler(const DRect & from, const DRect & to) :
        m_from(&from),
        m_to(&to),
        m_factorX(to.width() / from.width()),
        m_factorY(to.height() / from.height()),
        m_offsetX(to.p1().m_x - from.p1().m_x* m_factorX),
        m_offsetY(to.p1().m_y - from.p1().m_y* m_factorY) { }

    ~DScaler() {}

    //! Returns the rectangle in the source coordinate system.
    const DRect & from() const
    {
        return *m_from;
    }

    //! Returns the rectangle in the target coordinate system.
    const DRect & to()   const
    {
        return *m_to;
    }

    //! Transforms x-coordinates from source to target coordinate system.
    double scaleToX(double x)
    {
        return x * m_factorX + m_offsetX;
    }

    //! Transforms y-coordinates from source to target coordinate system.
    double scaleToY(double y)
    {
        return y * m_factorY + m_offsetY;
    }

    //! Scales a horizontal length from source to target coordinate system.
    double scaleWidth(double width)
    {
        return width  * m_to->width() / m_from->width();
    }

    //! Scales a vertical length from source to target coordinate system.
    double scaleHeight(double height)
    {
        return height * m_to->height() / m_from->height();
    }
};


//! Output operator for scalers.
OGDF_EXPORT ostream & operator<<(ostream & os, const DScaler & ds);


/**
 * \brief Line segments with real coordinates.
 */
class OGDF_EXPORT DSegment : public DLine
{

protected:

public:

    //! Creates an empty line segment.
    DSegment() : DLine() {}

    //! Creates a line segment from \a p1 to \a p2.
    DSegment(const DPoint & p1, const DPoint & p2) : DLine(p1, p2) {}

    //! Creates a line segment defined by the start and end point of line \a dl.
    DSegment(const DLine & dl) : DLine(dl) {}

    //! Creates a line segment from (\a x1,\a y1) to (\a x2,\a y2).
    DSegment(double x1, double y1, double x2, double y2) : DLine(x1, y1, x2, y2) {}

    //! Copy constructor.
    DSegment(const DSegment & ds) : DLine(ds) {}


    /**
     * \brief Determines if \a segment is left or right of this segment.
     *
     * \return a positve number if \a segment is left of this segment, and a
     *         a negative number if \a segment is right of this segment.
     */
    double det(const DSegment & segment) const
    {
        return (dx() * segment.dy() - dy() * segment.dx());
    }
};


/**
 * \brief Polygons with real coordinates.
 */
class OGDF_EXPORT DPolygon : public DPolyline
{

protected:

    bool m_counterclock; //!< If true points are given in conter-clockwise order.

public:
    /**
     * \brief Creates an empty polygon.
     *
     * @param cc determines in which order the points will be given; true means
     *        counter-clockwise, false means clockwise.
     */
    DPolygon(bool cc = true) : m_counterclock(cc) { }

    //! Creates a polgon from a rectangle.
    DPolygon(const DRect & rect, bool cc = true) : m_counterclock(cc)
    {
        operator=(rect);
    }

    //! Copy constructor.
    DPolygon(const DPolygon & dop) : DPolyline(dop), m_counterclock(dop.m_counterclock) { }

    //! Returns true iff points are given in counter-clockwise order.
    bool counterclock()
    {
        return m_counterclock;
    }

    //! Assignment operator.
    DPolygon & operator=(const DPolygon & dop)
    {
        List<DPoint>::operator =(dop);
        m_counterclock = dop.m_counterclock;
        return *this;
    }

    //! Assignment operator (for assigning from a rectangle).
    DPolygon & operator=(const DRect & rect);

    //! Returns the line segment that starts at position \a it.
    DSegment segment(ListConstIterator<DPoint> it) const;


    //! Inserts point \a p, that must lie on a polygon segment.
    ListIterator<DPoint> insertPoint(const DPoint & p)
    {
        return insertPoint(p, begin(), begin());
    }

    /**
     * \brief Inserts point \a p, but just searching from point \a p1 to \a p2.
     *
     * That is, from the segment starting at \a p1 to the segment ending at \a p2.
     */
    ListIterator<DPoint> insertPoint(const DPoint & p,
                                     ListIterator<DPoint> p1,
                                     ListIterator<DPoint> p2);

    //! Inserts point p on every segment (a,b) with \a p in the open range ]a, b[.
    void insertCrossPoint(const DPoint & p);

    //! Returns the list of intersection points of this polygon with \a p.
    int getCrossPoints(const DPolygon & p, List<DPoint> & crossPoints) const;

    //! Deletes all consecutive points that are equal.
    void unify();

    //! Deletes all points, which are not facets.
    void normalize();

    //! Writes the polygon as graph in gml-format to file \a filename.
    void writeGML(const char* filename) const;

    //! Writes the polygon as graph in gml-format to output stream \a stream.
    void writeGML(ostream & stream)      const;

    /**
     * \brief Checks wether a Point /a p is inside the Poylgon or not.
     * \note Polygons with crossings have inner areas that count as outside!
     * \par p the Point to check.
     * return true if Point is inside.
     */
    bool containsPoint(DPoint & p) const;
};

//! Output operator for polygons.
OGDF_EXPORT ostream & operator<<(ostream & os, const DPolygon & dop);



} // end namespace ogdf

#endif
