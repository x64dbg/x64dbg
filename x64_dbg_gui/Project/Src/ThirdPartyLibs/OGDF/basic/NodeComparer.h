/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief The NodeComparer compares nodes on base of the value
 *        stored in a node array given as constructor parameter
 *
 * \author Karsten Klein
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


#ifndef OGDF_NODECOMPARER_H
#define OGDF_NODECOMPARER_H

#include <ogdf/basic/basic.h>

namespace ogdf
{

template<class sorterType>
class NodeComparer
{
public:
    NodeComparer(NodeArray<sorterType> & valArray, bool ascending = true)
        : m_valArray(&valArray)
    {
        if(ascending)
        {
            m_smaller = -1;
            //              m_greater = 1;
        }//if
        else
        {
            m_smaller = 1;
            //              m_greater = -1;
        }

    }//constructor

    int compare(const node & v1, const node & v2) const
    {
        if((*m_valArray)[v1] < (*m_valArray)[v2]) return m_smaller;
        else if((*m_valArray)[v1] > (*m_valArray)[v2]) return -m_smaller; //m_greater;
        else return 0;
    }

    void init(NodeArray<sorterType> & valArray)
    {
        m_valArray = &valArray;
    }

    OGDF_AUGMENT_COMPARER(node)

private:
    NodeArray<sorterType>* m_valArray;
    int m_smaller;
    //      int m_greater;

};//NodeComparer


}//namespace

#endif

