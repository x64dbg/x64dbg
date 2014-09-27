/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Handles Abacus Dependencies.
 *
 * Include this file whenever you want to use Abacus. It does the
 * rest for you. Just be sure to structure your code using the
 * USE_ABACUS Flag. (See AbacusOptimalCrossingMinimizer for an
 * example).
 *
 * \author Markus Chimani
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


#ifndef OGDF_ABACUS_H
#define OGDF_ABACUS_H

#ifdef USE_ABACUS

#include <abacus/variable.h>
#include <abacus/constraint.h>
#include <abacus/master.h>
#include <abacus/buffer.h>
#include <abacus/sub.h>
#include <abacus/row.h>
#include <abacus/nonduplpool.h>
#include <abacus/active.h>
#include <abacus/branchrule.h>
#include <abacus/conbranchrule.h>

#else // USE_ABACUS

#include <ogdf/basic/basic.h>
#define THROW_NO_ABACUS_EXCEPTION OGDF_THROW_PARAM(LibraryNotSupportedException, lnscAbacus)


#endif // USE_ABACUS

#endif // OGDF_ABACUS_H
