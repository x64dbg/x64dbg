/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Implementation of algorithms as templates working with
 *        different list types
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

#ifndef OGDF_LIST_TEMPLATES_H
#define OGDF_LIST_TEMPLATES_H


#include <ogdf/basic/Array.h>


namespace ogdf
{

// sorts list L using quicksort
template<class LIST>
void quicksortTemplate(LIST & L)
{
    const int n = L.size();
    Array<typename LIST::value_type> A(n);

    int i = 0;
    typename LIST::iterator it;
    for(it = L.begin(); it.valid(); ++it)
        A[i++] = *it;

    A.quicksort();

    for(i = 0, it = L.begin(); i < n; i++)
        *it++ = A[i];
}


// sorts list L using quicksort and compare element comp
template<class LIST, class COMPARER>
void quicksortTemplate(LIST & L, COMPARER & comp)
{
    const int n = L.size();
    Array<typename LIST::value_type> A(n);

    int i = 0;
    typename LIST::iterator it;
    for(it = L.begin(); it.valid(); ++it)
        A[i++] = *it;

    A.quicksort(comp);

    for(i = 0, it = L.begin(); i < n; i++)
        *it++ = A[i];
}


} // end namespace ogdf

#endif
