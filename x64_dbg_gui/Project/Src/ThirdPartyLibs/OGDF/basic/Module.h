/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declares base class for all module types.
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


#ifndef OGDF_MODULE_H
#define OGDF_MODULE_H


#include <ogdf/basic/basic.h>

namespace ogdf
{


/**
 * \brief Base class for modules.
 *
 * A module represents an algorithm that implements a certain interface.
 * There are various specific module types present in the OGDF, which all
 * inherit Module as a base class. These module types define the interface
 * implemented by the module.
 *
 * \sa ModuleOption
 */
class OGDF_EXPORT Module
{
public:
    //! The return type of a module.
    enum ReturnType
    {
        retFeasible, //!< The solution is feasible.
        retOptimal, //!< The solution is optimal
        retNoFeasibleSolution, //!< There exists no feasible solution.
        retTimeoutFeasible, //!< The solution is feasible, but there was a timeout.
        retTimeoutInfeasible, //!< The solution is not feasible due to a timeout.
        retError //! Computation was aborted due to an error.
    };

    //! Initializes a module.
    Module() { }

    virtual ~Module() { }

    //! Returns true iff \a retVal indicates that the module returned a feasible solution.
    static bool isSolution(ReturnType ret)
    {
        return ret == retFeasible || ret == retOptimal || ret == retTimeoutFeasible;
    }
};


} // end namespace ogdf


#endif
