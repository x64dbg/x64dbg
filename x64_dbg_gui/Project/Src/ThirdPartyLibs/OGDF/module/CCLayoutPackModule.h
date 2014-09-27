/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of interface for algorithms that arrange/pack
 * layouts of connected components.
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

#ifndef OGDF_CC_LAYOUT_PACK_MODULE_H
#define OGDF_CC_LAYOUT_PACK_MODULE_H



#include <ogdf/basic/GraphAttributes.h>



namespace ogdf
{


/**
 * \brief Base class of algorithms that arrange/pack layouts of connected
 *        components.
 *
 * \see PlanarizationLayout<BR>PlanarizationGridLayout
 */
class OGDF_EXPORT CCLayoutPackModule
{
public:
    //! Initializes a layout packing module.
    CCLayoutPackModule() { }

    virtual ~CCLayoutPackModule() { }

    /**
     * \brief Arranges the rectangles given by \a box.
     *
     * The algorithm call takes an input an array \a box of rectangles with
     * real coordinates and computes in \a offset the offset to (0,0) of each
     * rectangle in the layout.
     *
     * This method is the actual algorithm call and must be overridden by derived
     * classes.
     * @param box is the array of input rectangles.
     * @param offset is assigned the offset of each rectangle to the origin (0,0).
     *        The offset of a rectangle is its lower left point in the layout.
     * @param pageRatio is the desired page ratio (width / height) of the
     *        resulting layout.
     */
    virtual void call(Array<DPoint> & box,
                      Array<DPoint> & offset,
                      double pageRatio = 1.0) = 0;

    /**
     * \brief Arranges the rectangles given by \a box.
     *
     * The algorithm call takes an input an array \a box of rectangles with
     * real coordinates and computes in \a offset the offset to (0,0) of each
     * rectangle in the layout.
     * @param box is the array of input rectangles.
     * @param offset is assigned the offset of each rectangle to the origin (0,0).
     *        The offset of a rectangle is its lower left point in the layout.
     * @param pageRatio is the desired page ratio (width / height) of the
     *        resulting layout.
     */
    void operator()(Array<DPoint> & box,
                    Array<DPoint> & offset,
                    double pageRatio = 1.0)
    {
        call(box, offset, pageRatio);
    }

    /**
     * \brief Arranges the rectangles given by \a box.
     *
     * The algorithm call takes an input an array \a box of rectangles with
     * integer coordinates and computes in \a offset the offset to (0,0) of each
     * rectangle in the layout.
     *
     * This method is the actual algorithm call and must be overridden by derived
     * classes.
     * @param box is the array of input rectangles.
     * @param offset is assigned the offset of each rectangle to the origin (0,0).
     *        The offset of a rectangle is its lower left point in the layout.
     * @param pageRatio is the desired page ratio (width / height) of the
     *        resulting layout.
     */
    virtual void call(Array<IPoint> & box,
                      Array<IPoint> & offset,
                      double pageRatio = 1.0) = 0;

    /**
     * \brief Arranges the rectangles given by \a box.
     *
     * The algorithm call takes an input an array \a box of rectangles with
     * integer coordinates and computes in \a offset the offset to (0,0) of each
     * rectangle in the layout.
     * @param box is the array of input rectangles.
     * @param offset is assigned the offset of each rectangle to the origin (0,0).
     *        The offset of a rectangle is its lower left point in the layout.
     * @param pageRatio is the desired page ratio (width / height) of the
     *        resulting layout.
     */
    void operator()(Array<IPoint> & box,
                    Array<IPoint> & offset,
                    double pageRatio = 1.0)
    {
        call(box, offset, pageRatio);
    }

    /**
     * \brief Checks if the rectangles in \a box do not overlap for given offsets.
     *
     * This function serves for checking if the computed offsets are correct in
     * the sense that the rectangles do not overlap in the resulting layout.
     * @param box is the array of rectangles.
     * @param offset is the array of corresponding offsets.
     */
    static bool checkOffsets(const Array<DPoint> & box,
                             const Array<DPoint> & offset);

    /**
     * \brief Checks if the rectangles in \a box do not overlap for given offsets.
     *
     * This function serves for checking if the computed offsets are correct in
     * the sense that the rectangles do not overlap in the resulting layout.
     * @param box is the array of rectangles.
     * @param offset is the array of corresponding offsets.
     */
    static bool checkOffsets(const Array<IPoint> & box,
                             const Array<IPoint> & offset);


    OGDF_MALLOC_NEW_DELETE

private:
    /**
     * \brief Checks if the rectangles in \a box do not overlap for given offsets.
     *
     * This is a parameterized function for generic point types \a POINT.
     * @param box is the array of rectangles.
     * @param offset is the array of corresponding offsets.
     */
    template<class POINT>
    static bool checkOffsetsTP(
        const Array<POINT> & box,
        const Array<POINT> & offset);
};


} // end namespace ogdf


#endif
