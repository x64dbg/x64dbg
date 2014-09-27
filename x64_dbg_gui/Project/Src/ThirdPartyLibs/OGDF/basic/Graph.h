/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Includes declaration of graph class.
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

#ifndef OGDF_GRAPH_H
#define OGDF_GRAPH_H


#include <ogdf/basic/List.h>
#include <ogdf/basic/NodeArray.h>
#include <ogdf/basic/EdgeArray.h>



namespace ogdf
{

//! Output operator for nodes; prints node index (or "nil").
OGDF_EXPORT ostream & operator<<(ostream & os, ogdf::node v);

//! Output operator for edges; prints source and target indices (or "nil").
OGDF_EXPORT ostream & operator<<(ostream & os, ogdf::edge e);

//! Output operator for adjacency entries; prints node and twin indices (or "nil").
OGDF_EXPORT ostream & operator<<(ostream & os, ogdf::adjEntry adj);


} // end namespace ogdf

#endif
