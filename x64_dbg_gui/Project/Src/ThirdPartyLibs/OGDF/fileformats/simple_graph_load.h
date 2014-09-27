/*
 * $Revision: 2583 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-12 01:02:21 +0200 (Do, 12. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of simple graph loaders.
 *
 * \author Markus Chimani, Carsten Gutwenger, Karsten Klein
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

#ifndef OGDF_SIMPLE_GRAPH_LOAD_H
#define OGDF_SIMPLE_GRAPH_LOAD_H

#include <ogdf/basic/Graph.h>
#include <ogdf/basic/GridLayout.h>


namespace ogdf
{

/** @name Simple graph formats (without layout)
 *  These functions load graphs stored in some common, simple text-based file formats. They just read
 *  the graph structure and not any layout specific data.
 */
///@{

//! Loads a graph \a G stored in the Rome-Graph-format from an input stream \a is.
/**
 * The Rome format contains (in this order) n "node-lines", 1 "separator-line", m "edge-lines".
 * These lines are as follows (whereby all IDs are integer numbers):
 *  - <b>node-line:</b> <i>NodeId</i> <tt>0</TT>
 *  - <b>separator-line:</b> starts with a <tt>#</tt>-sign
 *  - <b>edge-line:</b> <i>EdgeId</i> <tt>0</tt> <i>SourceNodeId</i> <i>TargetNodeId</i>
 *
 * @param G is assigned the loaded graph.
 * @param is is the input stream from which the graph is read.
 * \return true if the graph was loaded successfully, false otherwise.
 *
 * \warning
 * This is a very simple implementation only usable for very properly formatted files!
 */
OGDF_EXPORT bool loadRomeGraph(Graph & G, istream & is);

//! Loads a graph \a G stored in the Rome-Graph-format from a file \a fileName.
/**
 * @param G is assigned the loaded graph.
 * @param fileName is the name of the file from which the graph is read.
 * \return true if the graph was loaded successfully, false otherwise.
 *
 * \see loadRomeGraph(Graph &, istream&)
 */
OGDF_EXPORT bool loadRomeGraph(Graph & G, const char* fileName);

/** \brief Loads a graph \a G stored in the chaco file format from an input stream \a is.
 *
 * Graphs stored in the chaco file format are typically used in graph partitioning and
 * use the file extension .graph.
 *
 * The first line contains two integers separated by spacing: \#nodes \#edges
 * A third entry indicates node and edge weights (not supported here yet).
 * Each of the following \#nodes lines from 1 to \#nodes contains the space
 * separated index list of the adjacent nodes for the node associated with
 * that line (where node indices are from 1 to n).
 * Macro SIMPLE_LOAD_BUFFER_SIZE defines the length of the line read buffer
 * and should be adjusted according to the maximum read size.
 *
 * @param G is assigned the loaded graph.
 * @param is is the input stream from which the graph is read.
 * \return true if the graph was loaded successfully, false otherwise.
 */
OGDF_EXPORT bool loadChacoGraph(Graph & G, istream & is);

//! Loads a graph \a G stored in the chaco file format from a file \a fileName.
/**
 * @param G is assigned the loaded graph.
 * @param fileName is the name of the file from which the graph is read.
 * \return true if the graph was loaded successfully, false otherwise.
 *
 * \see loadChacoGraph(Graph &, istream&)
 */
OGDF_EXPORT bool loadChacoGraph(Graph & G, const char* fileName);

//! Loads a graph \a G stored in a simple format from an input stream \a is.
/**
 * Simple format has a leading line stating the name of the graph
 * and a following line stating the size of the graph.
 *
 * <pre>
 * *BEGIN unknown_name.numN.numE
 * *GRAPH numN numE UNDIRECTED UNWEIGHTED
 * </pre>
 *
 * @param G is assigned the loaded graph.
 * @param is is the input stream from which the graph is read.
 * \return true if the graph was loaded successfully, false otherwise.
 * */
OGDF_EXPORT bool loadSimpleGraph(Graph & G, istream & is);

//! Loads a graph \a G stored in a simple format from a file \a fileName.
/**
 * This format is used e.g. for the graphs from Petra Mutzel's Ph.D. Thesis.
 *
 * @param G is assigned the loaded graph.
 * @param fileName is the name of the file from which the graph is read.
 * \return true if the graph was loaded successfully, false otherwise.
 *
 * \see loadSimpleGraph(Graph &G, istream &is)
 */
OGDF_EXPORT bool loadSimpleGraph(Graph & G, const char* fileName);

//! Loads a graph \a G stored in the Y-graph-format from file stream (one line) \a lineStream.
/**
 * This format is e.g. produced by NAUTY (http://www.cs.sunysb.edu/~algorith/implement/nauty/implement.shtml)
 *
 * Details  on the format, as given in NAUTYs graph generator (see above link):
 * "[A] graph occupies one line with a terminating newline.
 * Except for the newline, each byte has the format  01xxxxxx, where
 * each "x" represents one bit of data.
 *
 * First byte:  xxxxxx is the number of vertices n
 *
 * Other ceiling(n(n-1)/12) bytes:  These contain the upper triangle of
 * the adjacency matrix in column major order.  That is, the entries
 * appear in the order (0,1),(0,2),(1,2),(0,3),(1,3),(2,3),(0,4),... .
 * The bits are used in left to right order within each byte.
 * Any unused bits on the end are set to zero.
 */
OGDF_EXPORT bool loadYGraph(Graph & G, FILE* lineStream);

///@}

/** @name Simple graph formats (with layout)
 *  These functions load and save graphs in some common, simple text-based file formats,
 *  which also store layout specific data in some limited way.
 */
///@{

//! Loads a graph \a G with layout \a gl stored in GD-Challenge-format from stream \a is.
/**
 * @param G is assigned the loaded graph.
 * @param gl is assigned the grid layout.
 * @param is is the input stream from which the graph is read.
 * \return true if the graph was loaded successfully, false otherwise.
 */
OGDF_EXPORT bool loadChallengeGraph(Graph & G, GridLayout & gl, istream & is);

//! Loads a graph \a G with layout \a gl stored in GD-Challenge-format from file \a fileName.
/**
 * @param G is assigned the loaded graph.
 * @param gl is assigned the grid layout.
 * @param fileName is the name of the file from which the graph is read.
 * \return true if the graph was loaded successfully, false otherwise.
 */
OGDF_EXPORT bool loadChallengeGraph(Graph & G, GridLayout & gl, const char* fileName);

//! Writes graph \a G with layout \a gl in GD-Challenge-format to stream \a os.
/**
 * @param G is the graph to be stored.
 * @param gl is the grid layout to be stored.
 * @param os is the output stream to which the graph is written.
 * \return true if the graph was stored successfully, false otherwise.
 */
OGDF_EXPORT bool saveChallengeGraph(const Graph & G, const GridLayout & gl, ostream & os);

//! Writes graph \a G with layout \a gl in GD-Challenge-format to file \a fileName.
/**
 * @param G is the graph to be stored.
 * @param gl is the grid layout to be stored.
 * @param fileName is the name of the file to which the graph is written.
 * \return true if the graph was stored successfully, false otherwise.
 */
OGDF_EXPORT bool saveChallengeGraph(const Graph & G, const GridLayout & gl, const char* fileName);

///@}

/** @name Simple graph formats (with subgraph)
 *  These functions load and store graphs in a simple text-based file format that also specifies
 *  a subgraph (given as a list of edges).
 */
///@{

//! Loads graph \a G with subgraph defined by \a delEdges from stream \a is.
/**
 * @param G is assigned the loaded graph.
 * @param delEdges is assigned the edges of the subgraph.
 * @param is is the input stream from which the graph is read.
 * \return true if the graph was loaded successfully, false otherwise.
 */
OGDF_EXPORT bool loadEdgeListSubgraph(Graph & G, List<edge> & delEdges, istream & is);

//! Loads graph \a G with subgraph defined by \a delEdges from file \a fileName.
/**
 * @param G is assigned the loaded graph.
 * @param delEdges is assigned the edges of the subgraph.
 * @param fileName is the name of the file from which the graph is read.
 * \return true if the graph was loaded successfully, false otherwise.
 */
OGDF_EXPORT bool loadEdgeListSubgraph(Graph & G, List<edge> & delEdges, const char* fileName);

//! Writes graph \a G with subgraph defined by \a delEdges to stream \a os.
/**
 * @param G is the graph to be stored.
 * @param delEdges specifies the edges of the subgraph to be stored.
 * @param os is the output stream to which the graph is written.
 * \return true if the graph was stored successfully, false otherwise.
 */
OGDF_EXPORT bool saveEdgeListSubgraph(const Graph & G, const List<edge> & delEdges, ostream & os);

//! Writes graph \a G with subgraph defined by \a delEdges to file \a fileName.
/**
 * @param G is the graph to be stored.
 * @param delEdges specifies the edges of the subgraph to be stored.
 * @param fileName is the name of the file to which the graph is written.
 * \return true if the graph was stored successfully, false otherwise.
 */
OGDF_EXPORT bool saveEdgeListSubgraph(const Graph & G, const List<edge> & delEdges, const char* fileName);

///@}

/** @name Hypergraphs
 *  These functions load hypergraphs stored in file formats used for electrical circuits.
 *  The hypergraphs are directly transformed into their point-based expansions (and hence stored
 *  in a usual Graph and not a Hypergraph).
 */
///@{

//! Loads a hypergraph in the BENCH-format from input stream \a is.
/**
 * A hypergraph in OGDF is represented by its point-based expansion, i.e., for each
 * hyperedge <i>h</i> we have a corresponding hypernode <i>n</i>. All nodes originally
 * incident to <i>h</i> are incident to <i>n</i>, i.e., have regular edges to <i>n</i>.
 *
 * @param G is assigned the graph (point-based expansion of the hypergraph).
 * @param hypernodes is assigned the list of nodes which have to be interpreted as hypernodes.
 * @param shell if 0 only the BENCH-hypergraph is loaded. Otherwise we extend the loaded graph
 *        by a simple edge <i>e=(i,o)</i> and two hyperedges: one hyperedges groups all input nodes and
 *        <i>i</i> together, the other hyperedge groups all output edges and <i>o</i>.
 *        These additional edges are then also collocated in shell.
 * @param is is the input stream from which the hypergraph is read.
 *
 * \warning
 * This is a very simple implementation only usable for very properly formatted files!
 */
OGDF_EXPORT bool loadBenchHypergraph(Graph & G, List<node> & hypernodes, List<edge>* shell, istream & is);

//! Loads a hypergraph in the BENCH-format from the specified file.
/**
 * @param G is assigned the graph (point-based expansion of the hypergraph).
 * @param hypernodes is assigned the list of nodes which have to be interpreted as hypernodes.
 * @param shell if 0 only the BENCH-hypergraph is loaded. Otherwise we extend the loaded graph
 *        by a simple edge <i>e=(i,o)</i> and two hyperedges: one hyperedges groups all input nodes and
 *        <i>i</i> together, the other hyperedge groups all output edges and <i>o</i>.
 *        These additional edges are then also collocated in shell.
 * @param fileName is the name of the file from which the hypergraph is read.
 *
 * \see loadBenchHypergraph(Graph &G, List<node>& hypernodes, List<edge>* shell, istream &is)
 */
OGDF_EXPORT bool loadBenchHypergraph(Graph & G, List<node> & hypernodes, List<edge>* shell, const char* fileName);

//! Loads a hypergraph in the PLA-format from input stream \a is.
/**
 * A hypergraph in OGDF is represented by its point-based expansion, i.e., for each
 * hyperedge <i>h</i> we have a corresponding hypernode <i>n</i>. All nodes originally
 * incident to <i>h</i> are incident to <i>n</i>, i.e., have regular edges to <i>n</i>.
 *
 * @param G is assigned the graph (point-based expansion of the hypergraph).
 * @param hypernodes is assigned the list of nodes which have to be interpreted as hypernodes.
 * @param shell if 0 only the PLA-hypergraph is loaded. Otherwise we extend the loaded graph
 *        by a simple edge <i>e=(i,o)</i> and two hyperedges: one hyperedges groups all input nodes and
 *        <i>i</i> together, the other hyperedge groups all output edges and <i>o</i>.
 *        These additional edges are then also collocated in shell.
 * @param is is the input stream from which the hypergraph is read.
 *
 * \warning
 * This is a very simple implementation only usable for very properly formatted files!
 */
OGDF_EXPORT bool loadPlaHypergraph(Graph & G, List<node> & hypernodes, List<edge>* shell, istream & is);

//! Loads a hypergraph in the PLA-format from file \a fileName.
/**
 * @param G is assigned the graph (point-based expansion of the hypergraph).
 * @param hypernodes is assigned the list of nodes which have to be interpreted as hypernodes.
 * @param shell if 0 only the PLA-hypergraph is loaded. Otherwise we extend the loaded graph
 *        by a simple edge <i>e=(i,o)</i> and two hyperedges: one hyperedges groups all input nodes and
 *        <i>i</i> together, the other hyperedge groups all output edges and <i>o</i>.
 *        These additional edges are then also collocated in shell.
 * @param fileName is the name of the file from which the hypergraph is read.
 *
 * \see loadPlaHypergraph(Graph &G, List<node>& hypernodes, List<edge> *shell, istream &is)
 */
OGDF_EXPORT bool loadPlaHypergraph(Graph & G, List<node> & hypernodes, List<edge>* shell, const char* fileName);

///@}


}

#endif //OGDF_SIMPLE_GRAPH_LOAD_H
