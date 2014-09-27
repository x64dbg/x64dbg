/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of interface for hierarchy layout algorithms
 *       (3. phase of Sugiyama) for cluster graphs.
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

#ifndef OGDF_HIER_CLUSTER_LAYOUT_MODULE_H
#define OGDF_HIER_CLUSTER_LAYOUT_MODULE_H



#include <ogdf/cluster/ClusterGraphCopyAttributes.h>


namespace ogdf
{


/**
 * \brief Interface of hierarchy layout algorithms for cluster graphs.
 *
 * \see SugiyamaLayout
 */
class OGDF_EXPORT HierarchyClusterLayoutModule
{
public:
    //! Initializes a hierarchy cluster layout module.
    HierarchyClusterLayoutModule() { }

    virtual ~HierarchyClusterLayoutModule() { }

    /**
     * \brief Computes a hierarchy layout of a clustered hierarchy \a H in \a ACG.
     * @param H is the input clustered hierarchy.
     * @param ACG is assigned the cluster hierarchy layout.
     */
    void callCluster(const ExtendedNestingGraph & H, ClusterGraphAttributes & ACG)
    {
        ClusterGraphCopyAttributes ACGC(H, ACG);
        doCall(H, ACGC);
        ACGC.transform();
    }

protected:
    /**
     * \brief Implements the actual algorithm call.
     *
     * Must be implemented by derived classes.
     *
     * @param H is the input clustered hierarchy.
     * @param ACGC has to be assigned the cluster hierarchy layout.
     */
    virtual void doCall(
        const ExtendedNestingGraph & H,
        ClusterGraphCopyAttributes & ACGC) = 0;

    OGDF_MALLOC_NEW_DELETE
};


} // end namespace ogdf


#endif
