/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Include of header files for SSE-intrinsics
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

#ifndef OGDF_INTRINSICS_H
#define OGDF_INTRINSICS_H

#include <ogdf/basic/basic.h>


#ifdef OGDF_SYSTEM_WINDOWS
#include <intrin.h>

#if (defined(_M_IX86) || defined(_M_IA64)) && !defined(_M_CEE_PURE)
#define OGDF_SSE2_EXTENSIONS
#define OGDF_SSE3_EXTENSIONS
#endif

#elif defined(OGDF_SYSTEM_UNIX) && (defined(__x86_64__) || defined(__i386__))
#include <pmmintrin.h>

#if (defined(__x86_64__) || defined(__i386__))  && !(defined(__GNUC__) && !defined(__SSE2__))
#define OGDF_SSE2_EXTENSIONS
#endif

#if (defined(__x86_64__) || defined(__i386__))  && !(defined(__GNUC__) && !defined(__SSE3__))
#define OGDF_SSE3_EXTENSIONS
#endif


#endif


#endif
