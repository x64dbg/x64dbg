/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Coin implementation of class LPSolver
 *
 * \author
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


#ifndef OGDF_LPSOLVER_COIN_H
#define OGDF_LPSOLVER_COIN_H

#include <ogdf/basic/Array.h>
#include <ogdf/external/coin.h>


namespace ogdf
{

class OGDF_EXPORT LPSolver
{
public:
    enum OptimizationGoal { lpMinimize, lpMaximize };
    enum Status { lpOptimal, lpInfeasible, lpUnbounded };

    // Constructor
    LPSolver();
    ~LPSolver()
    {
        delete osi;
    }

    double infinity() const;

    // Call of LP solver
    //
    // Input is an optimization goal, an objective function, a matrix in sparse format, an
    // equation-sense, and a right-hand side.
    // The arrays have to be allocated as follows:
    //
    // double obj [numCols]
    // int    matrixBegin [numCols]
    // int    matrixCount [numCols]
    // int    matrixIndex [numNonzeroes]
    // double matrixValue [numNonzeroes]
    // double rightHandSide [numRows]
    // char   equationSense [numRows]
    // double lowerBound [numCols]
    // double upperBound [numCols]
    // double x [numCols]
    //
    // The return value indicates the status of the solution. If an optimum solitions has
    // been found, the result is lpOptimal

    Status optimize(
        OptimizationGoal goal,  // goal of optimization (minimize or maximize)
        Array<double> & obj,           // objective function vector
        Array<int>  &  matrixBegin,    // matrixBegin[i] = begin of column i
        Array<int>  &  matrixCount,    // matrixCount[i] = number of nonzeroes in column i
        Array<int>  &  matrixIndex,    // matrixIndex[n] = index of matrixValue[n] in its column
        Array<double> & matrixValue,   // matrixValue[n] = non-zero value in matrix
        Array<double> & rightHandSide, // right-hand side of LP constraints
        Array<char>  & equationSense,  // 'E' ==   'G' >=   'L' <=
        Array<double> & lowerBound,    // lower bound of x[i]
        Array<double> & upperBound,    // upper bound of x[i]
        double & optimum,              // optimum value of objective function (if result is lpOptimal)
        Array<double> & x              // x-vector of optimal solution (if result is lpOptimal)
    );

    bool checkFeasibility(
        const Array<int>  &  matrixBegin,   // matrixBegin[i] = begin of column i
        const Array<int>  &  matrixCount,   // matrixCount[i] = number of nonzeroes in column i
        const Array<int>  &  matrixIndex,   // matrixIndex[n] = index of matrixValue[n] in its column
        const Array<double> & matrixValue,  // matrixValue[n] = non-zero value in matrix
        const Array<double> & rightHandSide, // right-hand side of LP constraints
        const Array<char>  & equationSense, // 'E' ==   'G' >=   'L' <=
        const Array<double> & lowerBound,   // lower bound of x[i]
        const Array<double> & upperBound,   // upper bound of x[i]
        const Array<double> & x             // x-vector of optimal solution (if result is lpOptimal)
    );

private:
    OsiSolverInterface* osi;
};


}


#endif
