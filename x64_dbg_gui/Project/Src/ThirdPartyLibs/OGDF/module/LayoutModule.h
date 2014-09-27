/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of interface for layout algorithms (class
 *        LayoutModule)
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

#ifndef OGDF_LAYOUT_MODULE_H
#define OGDF_LAYOUT_MODULE_H



#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/basic/Constraints.h>
#include <ogdf/internal/energybased/MultilevelGraph.h>

namespace ogdf
{


/**
 * \brief Interface of general layout algorithms.
 *
 */
class OGDF_EXPORT LayoutModule
{
public:
    //! Initializes a layout module.
    LayoutModule() { }

    virtual ~LayoutModule() { }

    /**
     * \brief Computes a layout of graph \a GA.
     *
     * This method is the actual algorithm call and must be implemented by
     * derived classes.
     * @param GA is the input graph and will also be assigned the layout information.
     */
    virtual void call(GraphAttributes & GA) = 0;

    /**
     * \brief Computes a layout of graph \a GA wrt the constraints in \a GC
     * (if applicable).
     */
    virtual void call(GraphAttributes & GA, GraphConstraints & GC)
    {
        call(GA);
    }

    /**
     * \brief Computes a layout of graph \a GA.
     *
     * @param GA is the input graph and will also be assigned the layout information.
     */
    void operator()(GraphAttributes & GA)
    {
        call(GA);
    }

    OGDF_MALLOC_NEW_DELETE
};


} // end namespace ogdf


#endif
