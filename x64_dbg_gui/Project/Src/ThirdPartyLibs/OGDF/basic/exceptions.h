/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Definition of exception classes
 *
 * \author Carsten Gutwenger, Markus Chimani
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

#include <stdio.h>
#include <ogdf/basic/basic.h>


#ifndef OGDF_EXCEPTIONS_H
#define OGDF_EXCEPTIONS_H


namespace ogdf
{

#ifdef OGDF_DEBUG
/**
 * If this flag is set the #OGDF_THROW macros pass the location where the
 * exception is thrown (file name and line number) to the exception
 * constructor, otherwise not.
 */
#define OGDF_THROW_WITH_INFO
#endif

#ifdef OGDF_THROW
#undef OGDF_THROW
#endif

#ifndef OGDF_THROW_WITH_INFO
#define OGDF_THROW_PARAM(CLASS, PARAM) throw CLASS ( PARAM )
#define OGDF_THROW(CLASS)              throw CLASS ( )
#else
//! Replacement for \c throw.
/**
 * This macro is used to throw an exception and pass the file name
 * and line number of the location in the source file.
 * @param CLASS is the name of the exception class.
 * @param PARAM is an additional parameter (like the error code) required
 *        by the exception calls.
 */
#define OGDF_THROW_PARAM(CLASS, PARAM) throw CLASS ( PARAM , __FILE__ , __LINE__ )
//! Replacement for \c throw.
/**
 * This macro is used to throw an exception and pass the file name
 * and line number of the location in the source file.
 * @param CLASS is the name of the exception class.
 */
#define OGDF_THROW(CLASS)              throw CLASS ( __FILE__ , __LINE__ )
#endif


//! Error code for a violated precondition.
/**
 * \see PreconditionViolatedException
 */
enum PreconditionViolatedCode
{
    pvcUnknown,
    pvcSelfLoop,          //!< graph contains a self-loop
    pvcTreeHierarchies,   //!< hierarchies are not only trees
    pvcAcyclicHierarchies,//!< hierarchies are not acyclic
    pvcSingleSource,      //!< graph has not a single source
    pvcUpwardPlanar,      //!< graph is not upward planar
    pvcTree,              //!< graph is not a rooted tree
    pvcForest,            //!< graph is not a rooted forest
    pvcOrthogonal,        //!< layout is not orthogonal
    pvcPlanar,            //!< graph is not planar
    pvcClusterPlanar,     //!< graph is not c-planar
    pvcNoCopy,            //!< graph is not a copy of the corresponding graph
    pvcConnected,         //!< graph is not connected
    pvcBiconnected,         //!< graph is not twoconnected
    pvcSTOP               // INSERT NEW CODES BEFORE pvcSTOP!
}; // enum PreconditionViolatedCode


//! Code for an internal failure condition
/**
 * \see AlgorithmFailureException
 */
enum AlgorithmFailureCode
{
    afcUnknown,
    afcIllegalParameter, //!< function parameter is illegal
    afcNoFlow,           //!< min-cost flow could not find a legal flow
    afcSort,             //!< sequence not sorted
    afcLabel,            //!< labelling failed
    afcExternalFace,     //!< external face not correct
    afcForbiddenCrossing,//!< crossing forbidden but necessary
    afcTimelimitExceeded,//!< it took too long
    afcNoSolutionFound,  //!< couldn't solve the problem
    afcSTOP              // INSERT NEW CODES BEFORE afcSTOP!
}; // enum AlgorithmFailureCode



//! Code for the library which was intended to get used, but its use is not supported.
/**
 * \see LibraryNotSupportedException
 */
enum LibraryNotSupportedCode
{
    lnscUnknown,
    lnscCoin,                          //!< COIN not supported
    lnscAbacus,                        //!< ABACUS not supported
    lnscFunctionNotImplemented,        //!< the used library doesn't support that function
    lnscMissingCallbackImplementation, //
    lnscSTOP                           // INSERT NEW CODES BEFORE nscSTOP!
}; // enum AlgorithmFailureCode



//! Base class of all ogdf exceptions.
class OGDF_EXPORT Exception
{

private:

    const char* m_file; //!< Source file where exception occurred.
    int         m_line; //!< Line number where exception occurred.

public:
    //! Constructs an exception.
    /**
     * @param file is the name of the source file where exception was thrown.
     * @param line is the line number in the source file where the exception was thrown.
     */
    Exception(const char* file = NULL, int line = -1) :
        m_file(file),
        m_line(line)
    { }

    //! Returns the name of the source file where exception was thrown.
    /**
     * Returns a null pointer if the name of the source file is unknown.
     */
    const char* file()
    {
        return m_file;
    }

    //! Returns the line number where the exception was thrown.
    /**
     * Returns -1 if the line number is unknown.
     */
    int line()
    {
        return m_line;
    }
};


//! %Exception thrown when result of cast is 0.
class OGDF_EXPORT DynamicCastFailedException : public Exception
{

public:
    //! Constructs a dynamic cast failed exception.
    DynamicCastFailedException(const char* file = NULL, int line = -1) : Exception(file, line) {}
};


//! %Exception thrown when not enough memory is available to execute an algorithm.
class OGDF_EXPORT InsufficientMemoryException : public Exception
{

public:
    //! Constructs an insufficient memory exception.
    InsufficientMemoryException(const char* file = NULL, int line = -1) : Exception(file, line) {}
};


//! %Exception thrown when a required standard comparer has not been specialized.
/**
 * The default implementation of StdComparer<E> throws this exception, since it
 * provides no meaningful implementation of comparer methods. You need to specialize
 * this class for the types you want to use with sorting and searching methods (like
 * quicksort and binary search).
 */
class OGDF_EXPORT NoStdComparerException : public Exception
{

public:
    //! Constructs a no standard comparer available exception.
    NoStdComparerException(const char* file = NULL, int line = -1) : Exception(file, line) {}
};


//! %Exception thrown when preconditions are violated.
class OGDF_EXPORT PreconditionViolatedException : public Exception
{
public:
    //! Constructs a precondition violated exception.
    PreconditionViolatedException(PreconditionViolatedCode code,
                                  const char* file = NULL,
                                  int line = -1) :
        Exception(file, line),
        m_exceptionCode(code)
    {}

    //! Constructs a precondition violated exception.
    PreconditionViolatedException(
        const char* file = NULL,
        int line = -1) :
        Exception(file, line),
        m_exceptionCode(pvcUnknown)
    {}

    //! Returns the error code of the exception.
    PreconditionViolatedCode exceptionCode() const
    {
        return m_exceptionCode;
    }

private:
    PreconditionViolatedCode m_exceptionCode; //!< The error code specifying the exception.
}; // class PreconditionViolatedException



//! %Exception thrown when an algorithm realizes an internal bug that prevents it from continuing.
class OGDF_EXPORT AlgorithmFailureException : public Exception
{
public:

    //! Constructs an algorithm failure exception.
    AlgorithmFailureException(AlgorithmFailureCode code,
                              const char* file = NULL,
                              int line = -1) :
        Exception(file, line),
        m_exceptionCode(code)
    {}

    //! Constructs an algorithm failure exception.
    AlgorithmFailureException(
        const char* file = NULL,
        int line = -1) :
        Exception(file, line),
        m_exceptionCode(afcUnknown)
    {}

    //! Returns the error code of the exception.
    AlgorithmFailureCode exceptionCode() const
    {
        return m_exceptionCode;
    }

private:
    AlgorithmFailureCode m_exceptionCode; //!< The error code specifying the exception.
}; // class AlgorithmFailureException



//! %Exception thrown when an external library shall be used which is not supported.
class OGDF_EXPORT LibraryNotSupportedException : public Exception
{
public:
    //! Constructs a library not supported exception.
    LibraryNotSupportedException(LibraryNotSupportedCode code,
                                 const char* file = NULL,
                                 int line = -1) :
        Exception(file, line),
        m_exceptionCode(code)
    {}

    //! Constructs a library not supported exception.
    LibraryNotSupportedException(
        const char* file = NULL,
        int line = -1) :
        Exception(file, line),
        m_exceptionCode(lnscUnknown)
    {}

    //! Returns the error code of the exception.
    LibraryNotSupportedCode exceptionCode() const
    {
        return m_exceptionCode;
    }

private:
    LibraryNotSupportedCode m_exceptionCode; //!< The error code specifying the exception.
}; // class LibraryNotSupportedException

} // end namespace ogdf


#endif
