/** \file
 * \brief Implementation of a parser for SteinLib instances.
 *
 * \author Matthias Woste
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

#ifndef OGDF_STEIN_LIB_PARSER_H_
#define OGDF_STEIN_LIB_PARSER_H_

#include <ogdf/internal/steinertree/EdgeWeightedGraph.h>
#include <string.h>
#include <sys/stat.h>

namespace ogdf
{

/*!
 * \brief Reads a SteinLib file and converts it into a weighted graph and a set of terminal nodes.
 *
 * Attention: The coordinate section is not read!
 */
template<typename T>
class SteinLibParser
{
public:
    /*!
     * \brief Reads a SteinLib file and converts it into a weighted graph and a set of terminal nodes
     * @param fileName Name of the SteinLib file
     * @param wG Graph structure that will represent the graph stored in the SteinLib file
     * @param terminals List of terminals specified in the SteinLib file
     * @param isTerminal Incidence vector for the terminal nodes in the graph
     * @return True, if the parsing was successful, false otherwise
     */
    bool readSteinLibInstance(const char* fileName, EdgeWeightedGraph<T> & wG, List<node> & terminals,
                              NodeArray<bool> & isTerminal) const
    {

        std::ifstream is(fileName);
        if(!is && !is.good())
        {
            return false;
        }

        char buffer[1024];
        int section = 0;
        int nextSection = 1;
        char value[1024];
        char key[1024];
        int scannedElements;
        int n;
        int v1, v2, v3;
        Array<node> indexToNode;
        //node root; // root terminal (directed case)

        std::string name, date, creator, remark;

        // 1. line = identifier
        is.getline(buffer, 1024);
        if(strcasecmp(buffer, "33D32945 STP File, STP Format Version 1.0")
                && strcasecmp(buffer, "33d32945 STP File, STP Format Version  1.00"))
        {
            return false;
        }

        while(!is.eof())
        {
            is.getline(buffer, 1024);

            if(buffer[0] == '#' || strlen(buffer) == 0 || buffer[0] == '\n')
            {
                continue;
            }

            switch(section)
            {
            case 0:
                if(!strcasecmp(buffer, "SECTION Comment")
                        && nextSection == 1)
                {
                    section = 1;
                }
                else if(!strcasecmp(buffer, "SECTION Graph")
                        && nextSection == 2)
                {
                    section = 2;
                }
                else if(!strcasecmp(buffer, "SECTION Terminals")
                        && nextSection == 3)
                {
                    section = 3;
                }
                else if(!strcasecmp(buffer, "SECTION Coordinates")
                        && nextSection == 4)
                {
                    section = 4;
                }
                else if(!strcasecmp(buffer, "EOF") && nextSection >= 4)
                {
                    return true;
                }
                break;
            case 1: // comment section
                sscanf(buffer, "%s %s", key, value);
                if(strcmp(key, "Name") == 0)
                {
                    name = value;
                }
                else if(!strcasecmp(key, "Date"))
                {
                    date = value;
                }
                else if(!strcasecmp(key, "Creator"))
                {
                    creator = value;
                }
                else if(!strcasecmp(key, "Remark"))
                {
                    remark = value;
                }
                else if(!strcasecmp(key, "END"))
                {
                    nextSection = 2;
                    section = 0;
                }
                else
                {
                    return false;
                }
                break;
            case 2: // graph section
                scannedElements = sscanf(buffer, "%s %d %d %d", key, &v1, &v2, &v3);
                switch(scannedElements)
                {
                case 1: // END
                    if(!strcasecmp(key, "END"))
                    {
                        nextSection = 3;
                        section = 0;
                    }
                    else
                    {
                        return false;
                    }
                    break;
                case 2: //Number of nodes, edges or arcs
                    if(!strcasecmp(key, "Nodes"))
                    {
                        n = v1;
                        indexToNode = Array<node>(1, n, 0);
                        for(int i = 1; i <= n; i++)
                        {
                            indexToNode[i] = wG.newNode();
                            isTerminal[indexToNode[i]] = false;
                        }
                    }
#if 0
                    else if(!strcasecmp(key, "Edges"))
                    {
                        m = v1;
                        wG.directed(false);
                    }
                    else if(!strcasecmp(key, "Arcs"))
                    {
                        m = v1;
                        wG.directed(true);
                    }
#endif
                    break;
                case 4: // specific edge or arc
                    switch(buffer[0])
                    {
                    case 'E':
                    case 'A':
                        if(v1 > n || v2 > n)
                        {
                            return false;
                        }
                        wG.newEdge(indexToNode[v1], indexToNode[v2], v3);
                        break;
                    default:
                        return false;
                    }
                    ;
                    break;
                default:
                    return false;
                }
                break;
            case 3: // terminals section
                sscanf(buffer, "%s %d", key, &v1);
#if 0
                if(!strcasecmp(key, "Terminals"))
                {
                    t = v1; // set number of terminals
                }
                else if(!strcasecmp(key, "Root"))
                {
                    if(v1 > n)
                    {
                        return false;
                    }
                    root = indexToNode[v1];
                }
                else
#endif
                    if(!strcasecmp(key, "T"))
                    {
                        if(v1 > n)
                        {
                            return false;
                        }
                        terminals.pushBack(indexToNode[v1]);
                        isTerminal[indexToNode[v1]] = true;
                    }
                    else if(!strcasecmp(key, "END"))
                    {
                        nextSection = 4;
                        section = 0;
                    }
                // no else: ignore unused keys
                break;
            case 4: // coordinates section (omitted)
                sscanf(buffer, "%s", key);
                if(!strcasecmp(key, "END"))
                {
                    nextSection = 5;
                    section = 0;
                }
                break;
            default:
                return false;
            }
        }
        return false;
    }
};

}

#endif /* OGDF_STEIN_LIB_PARSER_H_ */
