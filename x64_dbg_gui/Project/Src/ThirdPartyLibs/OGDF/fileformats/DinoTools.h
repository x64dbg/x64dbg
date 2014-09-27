/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of the class DinoTools
 *
 * \author Dino Ahr
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

#ifndef OGDF_DINO_TOOLS_H
#define OGDF_DINO_TOOLS_H

#include <ogdf/basic/String.h>
#include <ogdf/basic/Array.h>


namespace ogdf
{

//---------------------------------------------------------
// D i n o T o o l s
//
// provides some useful tools
//---------------------------------------------------------
class OGDF_EXPORT DinoTools
{

public:

    // Extracts the single values of string str with format
    // "x, y, width, height," and puts them into doubleArray
    static void stringToDoubleArray(const String & str, Array<double> & doubleArray);

    // Reports errors to cout
    // Value -1 for inputFileLine indicates that this information is
    // not available
    static void reportError(const char* functionName,
                            int sourceLine,
                            const char* errorMessage,
                            int inputFileLine = -1,
                            bool abort = true);

}; // class DinoTools



} // end namespace ogdf

#endif
