/*
 * $Revision: 2583 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-12 01:02:21 +0200 (Do, 12. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of the class ExtractKuratowskis
 *
 * \author Jens Schmidt
 *
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

#ifndef OGDF_EXTRACT_KURATOWSKIS_H
#define OGDF_EXTRACT_KURATOWSKIS_H

#include <ogdf/internal/planarity/BoyerMyrvoldPlanar.h>
#include <ogdf/internal/planarity/FindKuratowskis.h>
#include <ogdf/basic/Stack.h>

namespace ogdf
{


//! Extracts all possible paths with backtracking using given edges and special constraints
class OGDF_EXPORT DynamicBacktrack
{
public:
    //! Constructor
    DynamicBacktrack(
        const Graph & g,
        const NodeArray<int> & dfi,
        const EdgeArray<int> & flags)
        :   m_flags(flags),
            m_dfi(dfi),
            m_parent(g, NULL)
    {
    }

    //! Reinitializes backtracking with new constraints. All paths will be traversed again.
    /** Startedges are either only \a startInclude or not \a startExclude, all startedges
     * have to contain the flag \a startFlag, if \a startFlag != 0. The \a start- and \a endnode
     * of extracted paths is given, too.
     */
    void init(
        const node start,
        const node end,
        const bool less,
        const int flag,
        const int startFlag,
        const edge startInclude,
        const edge startExlude);

    //! Returns next possible path from \a start- to \a endnode, if exists.
    /** The path is returned to \a list. After that a process image is made,
     * allowing to pause backtracking and extracting further paths later.
     * \a Endnode returns the last traversed node.
     */
    bool addNextPath(SListPure<edge> & list, node & endnode);

    //! Returns next possible path under constraints from \a start- to \a endnode, if exists.
    /** All paths avoid \a exclude-edges, except if on an edge with flag \a exceptOnEdge.
     * The NodeArray \a nodeflags is used to mark visited nodes. Only that part of the path,
     * which doesn't contain \a exclude-edges is finally added.
     * The path is returned to \a list. After that a process image is made,
     * allowing to pause backtracking and extracting further paths later.
     * \a Endnode returns the last traversed node.
     */
    bool addNextPathExclude(SListPure<edge> & list,
                            node & endnode,
                            const NodeArray<int> & nodeflags,
                            int exclude,
                            int exceptOnEdge);

    // avoid automatic creation of assignment operator
    //! Assignment is not defined!
    DynamicBacktrack & operator=(const DynamicBacktrack &);

    //! Marks an edge with three Flags: externalPath, pertinentPath and/or singlePath
    enum enumKuratowskiFlag
    {
        externalPath        = 0x00001, // external paths, e.g. stopX->Ancestor
        pertinentPath       = 0x00002, // pertinent paths, e.g. wNode->V
        singlePath          = 0x00004, // marker for one single path
    };

protected:
    //! Flags, that partition the edges into pertinent and external subgraphs
    const EdgeArray<int> & m_flags;
    //! The one and only DFI-NodeArray
    const NodeArray<int> & m_dfi;

    //! Start node of backtracking
    node start;
    //! Identifies endnodes
    node end;
    //! Iff true, DFI of endnodes has to be < \a DFI[end], otherwise the only valid endnode is \a end
    bool less;
    //! Every traversed edge has to be signed with this flag
    int flag;

    //! Saves the parent edge for each node in path
    NodeArray<adjEntry> m_parent;

    //! Backtracking stack. A NULL-element indicates a return from a child node
    StackPure<adjEntry> stack;
};

//! Wrapper-class for Kuratowski Subdivisions containing the minortype and edgelist
class OGDF_EXPORT KuratowskiWrapper
{
public:
    //! Constructor
    KuratowskiWrapper() { }

    //! Returns true, iff subdivision is a K3,3-minor
    inline bool isK33() const
    {
        return subdivisionType != E5;
    }
    //! Returns true, iff subdivision is a K5-minor
    inline bool isK5() const
    {
        return subdivisionType == E5;
    }

    //! Possible minortypes of a Kuratowski Subdivision
    enum enumSubdivisionType
    {
        A = 0,
        AB = 1,
        AC = 2,
        AD = 3,
        AE1 = 4,
        AE2 = 5,
        AE3 = 6,
        AE4 = 7,
        B = 8,
        C = 9,
        D = 10,
        E1 = 11,
        E2 = 12,
        E3 = 13,
        E4 = 14,
        E5 = 15
    };
    //! Minortype of the Kuratowski Subdivision
    int subdivisionType;

    //! The node which was embedded while the Kuratowski Subdivision was found
    node V;

    //! Contains the edges of the Kuratowski Subdivision
    SListPure<edge> edgeList;
};

//! Extracts multiple Kuratowski Subdivisions
/** \pre Graph has to be simple.
 */
class ExtractKuratowskis
{
public:
    //! Constructor
    ExtractKuratowskis(BoyerMyrvoldPlanar & bm);
    //! Destructor
    ~ExtractKuratowskis() { }

    //! Extracts all Kuratowski Subdivisions and adds them to \a output (without bundles)
    void extract(
        const SListPure<KuratowskiStructure> & allKuratowskis,
        SList<KuratowskiWrapper> & output);

    //! Extracts all Kuratowski Subdivisions and adds them to \a output (with bundles)
    void extractBundles(
        const SListPure<KuratowskiStructure> & allKuratowskis,
        SList<KuratowskiWrapper> & output);

    //! Enumeration over Kuratowski Type none, K33, K5
    enum enumKuratowskiType
    {
        none    = 0, //!< no kuratowski subdivision exists
        K33     = 1, //!< a K3,3 subdivision exists
        K5      = 2  //!< a K5 subdivision exists
    };

    //! Checks, if \a list forms a valid Kuratowski Subdivision and returns the type
    /**
     * @return Returns the following value:
     *           - none = no Kuratowski
     *           - K33 = the K3,3
     *           - K5 = the K5
     */
    static int whichKuratowski(
        const Graph & m_g,
        const NodeArray<int> & dfi,
        const SListPure<edge> & list);

    //! Checks, if edges in Array \a edgenumber form a valid Kuratowski Subdivision and returns the type
    /**
     * \pre The numer of edges has to be 1 for used edges, otherwise 0.
     * @return Returns the following value:
     *           - none = no Kuratowski
     *           - K33 = the K3,3
     *           - K5 = the K5
     */
    static int whichKuratowskiArray(
        const Graph & g,
        //const NodeArray<int>& m_dfi,
        EdgeArray<int> & edgenumber);

    //! Returns true, iff the Kuratowski is not already contained in output
    static bool isANewKuratowski(
        const Graph & g,
        const SListPure<edge> & kuratowski,
        const SList<KuratowskiWrapper> & output);
    //! Returns true, iff the Kuratowski is not already contained in output
    /** \pre Kuratowski Edges are all edges != 0 in the Array.
     */
    static bool isANewKuratowski(
        //const Graph& g,
        const EdgeArray<int> & test,
        const SList<KuratowskiWrapper> & output);

    // avoid automatic creation of assignment operator
    //! Assignment operator is undefined!
    ExtractKuratowskis & operator=(const ExtractKuratowskis &);

protected:
    //! Link to class BoyerMyrvoldPlanar
    BoyerMyrvoldPlanar & BMP;

    //! Input graph
    const Graph & m_g;

    //! Some parameters, see BoyerMyrvold for further instructions
    int m_embeddingGrade;
    //! Some parameters, see BoyerMyrvold for further instructions
    const bool m_avoidE2Minors;

    //! Value used as marker for visited nodes etc.
    /** Used during Backtracking and the extraction of some specific minortypes
     */
    int m_nodeMarker;
    //! Array maintaining visited bits on each node
    NodeArray<int> m_wasHere;

    //! The one and only DFI-NodeArray
    const NodeArray<int> & m_dfi;

    //! Returns appropriate node from given DFI
    const Array<node> & m_nodeFromDFI;

    //! The adjEntry which goes from DFS-parent to current vertex
    const NodeArray<adjEntry> & m_adjParent;

    //! Adds external face edges to \a list
    inline void addExternalFacePath(
        SListPure<edge> & list,
        const SListPure<adjEntry> & externPath)
    {
        SListConstIterator<adjEntry> itExtern;
        for(itExtern = externPath.begin(); itExtern.valid(); ++itExtern)
        {
            list.pushBack((*itExtern)->theEdge());
        }
    }

    //! Returns \a adjEntry of the edge between node \a high and a special node
    /** The special node is that node with the lowest DFI not less than the DFI of \a low.
     */
    inline adjEntry adjToLowestNodeBelow(node high, int low);

    //! Adds DFS-path from node \a bottom to node \a top to \a list
    /** \pre Each virtual node has to be merged.
     */
    inline void addDFSPath(SListPure<edge> & list, node bottom, node top);
    //! Adds DFS-path from node \a top to node \a bottom to \a list
    /** \pre Each virtual node has to be merged.
     */
    inline void addDFSPathReverse(SListPure<edge> & list, node bottom, node top);

    //! Separates \a list1 from edges already contained in \a list2
    inline void truncateEdgelist(SListPure<edge> & list1, const SListPure<edge> & list2);

    //! Extracts minortype A and adds it to list \a output
    void extractMinorA(
        SList<KuratowskiWrapper> & output,
        const KuratowskiStructure & k,
        //const WInfo& info,
        const SListPure<edge> & pathX,
        const node endnodeX,
        const SListPure<edge> & pathY,
        const node endnodeY,
        const SListPure<edge> & pathW);
    //! Extracts minortype B and adds it to list \a output (no bundles)
    void extractMinorB(
        SList<KuratowskiWrapper> & output,
        //NodeArray<int>& nodeflags,
        //const int nodemarker,
        const KuratowskiStructure & k,
        const WInfo & info,
        const SListPure<edge> & pathX,
        const node endnodeX,
        const SListPure<edge> & pathY,
        const node endnodeY,
        const SListPure<edge> & pathW);
    //! Extracts minortype B and adds it to list \a output (with bundles)
    void extractMinorBBundles(
        SList<KuratowskiWrapper> & output,
        NodeArray<int> & nodeflags,
        const int nodemarker,
        const KuratowskiStructure & k,
        EdgeArray<int> & flags,
        const WInfo & info,
        const SListPure<edge> & pathX,
        const node endnodeX,
        const SListPure<edge> & pathY,
        const node endnodeY,
        const SListPure<edge> & pathW);
    //! Extracts minortype C and adds it to list \a output
    void extractMinorC(
        SList<KuratowskiWrapper> & output,
        const KuratowskiStructure & k,
        const WInfo & info,
        const SListPure<edge> & pathX,
        const node endnodeX,
        const SListPure<edge> & pathY,
        const node endnodeY,
        const SListPure<edge> & pathW);
    //! Extracts minortype D and adds it to list \a output
    void extractMinorD(
        SList<KuratowskiWrapper> & output,
        const KuratowskiStructure & k,
        const WInfo & info,
        const SListPure<edge> & pathX,
        const node endnodeX,
        const SListPure<edge> & pathY,
        const node endnodeY,
        const SListPure<edge> & pathW);
    //! Extracts minortype E and adds it to list \a output (no bundles)
    void extractMinorE(
        SList<KuratowskiWrapper> & output,
        bool firstXPath,
        bool firstPath,
        bool firstWPath,
        bool firstWOnHighestXY,
        const KuratowskiStructure & k,
        const WInfo & info,
        const SListPure<edge> & pathX,
        const node endnodeX,
        const SListPure<edge> & pathY,
        const node endnodeY,
        const SListPure<edge> & pathW);
    //! Extracts minortype E and adds it to list \a output (bundles)
    void extractMinorEBundles(
        SList<KuratowskiWrapper> & output,
        bool firstXPath,
        bool firstPath,
        bool firstWPath,
        bool firstWOnHighestXY,
        NodeArray<int> & nodeflags,
        const int nodemarker,
        const KuratowskiStructure & k,
        EdgeArray<int> & flags,
        const WInfo & info,
        const SListPure<edge> & pathX,
        const node endnodeX,
        const SListPure<edge> & pathY,
        const node endnodeY,
        const SListPure<edge> & pathW);
    //! Extracts minorsubtype E1 and adds it to list \a output
    void extractMinorE1(
        SList<KuratowskiWrapper> & output,
        int before,
        //const node z,
        const node px,
        const node py,
        const KuratowskiStructure & k,
        const WInfo & info,
        const SListPure<edge> & pathX,
        const node endnodeX,
        const SListPure<edge> & pathY,
        const node endnodeY,
        const SListPure<edge> & pathW,
        const SListPure<edge> & pathZ,
        const node endnodeZ);
    //! Extracts minorsubtype E2 and adds it to list \a output
    void extractMinorE2(
        SList<KuratowskiWrapper> & output,
        /*int before,
        const node z,
        const node px,
        const node py,*/
        const KuratowskiStructure & k,
        const WInfo & info,
        const SListPure<edge> & pathX,
        const node endnodeX,
        const SListPure<edge> & pathY,
        const node endnodeY,
        //const SListPure<edge>& pathW,
        const SListPure<edge> & pathZ/*,
                const node endnodeZ*/);
    //! Extracts minorsubtype E3 and adds it to list \a output
    void extractMinorE3(
        SList<KuratowskiWrapper> & output,
        int before,
        const node z,
        const node px,
        const node py,
        const KuratowskiStructure & k,
        const WInfo & info,
        const SListPure<edge> & pathX,
        const node endnodeX,
        const SListPure<edge> & pathY,
        const node endnodeY,
        const SListPure<edge> & pathW,
        const SListPure<edge> & pathZ,
        const node endnodeZ);
    //! Extracts minorsubtype E4 and adds it to list \a output
    void extractMinorE4(
        SList<KuratowskiWrapper> & output,
        int before,
        const node z,
        const node px,
        const node py,
        const KuratowskiStructure & k,
        const WInfo & info,
        const SListPure<edge> & pathX,
        const node endnodeX,
        const SListPure<edge> & pathY,
        const node endnodeY,
        const SListPure<edge> & pathW,
        const SListPure<edge> & pathZ,
        const node endnodeZ);
    //! Extracts minorsubtype E5 and adds it to list \a output
    void extractMinorE5(
        SList<KuratowskiWrapper> & output,
        /*int before,
        const node z,
        const node px,
        const node py,*/
        const KuratowskiStructure & k,
        const WInfo & info,
        const SListPure<edge> & pathX,
        const node endnodeX,
        const SListPure<edge> & pathY,
        const node endnodeY,
        const SListPure<edge> & pathW,
        const SListPure<edge> & pathZ,
        const node endnodeZ);
};

}

#endif
