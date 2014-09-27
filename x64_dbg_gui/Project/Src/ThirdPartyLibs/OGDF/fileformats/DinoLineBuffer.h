/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of the clssses DinoLineBuffer and
 * DinoLineBufferPosition
 *
 * \author Dino Ahr
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

#ifndef OGDF_DINO_LINE_BUFFER_H
#define OGDF_DINO_LINE_BUFFER_H

#include <ogdf/basic/basic.h>


namespace ogdf
{

//---------------------------------------------------------
// D i n o L i n e B u f f e r P o s i t i o n
//---------------------------------------------------------
/** This class characterizes uniquely a position in the line
 *  buffer.
 *
 * Note that the element m_lineUpdateCount allows to check
 * if a position is obsolete, i.e. its content has already
 * been overwritten.
 */
class OGDF_EXPORT DinoLineBufferPosition
{

private:

    /** Contains the lineNumber; Range [0 .. c_maxNoOfLines-1] */
    int m_lineNumber;

    /** Contains the number of times line m_lineNumber has been
     * overwritten by new data; Range [0 .. ]
     */
    int m_lineUpdateCount;

    /** Contains the position in line m_lineNumber; Range [0 .. c_maxLineLength-1] */
    int m_linePosition;

public:

    /** Default Constructor */
    DinoLineBufferPosition() :
        m_lineNumber(0),
        m_lineUpdateCount(0),
        m_linePosition(0)
    { }

    /** Constructor */
    DinoLineBufferPosition(
        int lineNumber,
        int lineUpdateCount,
        int linePosition);

    /** Copy Constructor */
    DinoLineBufferPosition(const DinoLineBufferPosition & position);

    /** Get the line number */
    inline int getLineNumber() const
    {
        return m_lineNumber;
    }

    /** Get the update count of the line */
    inline int getLineUpdateCount() const
    {
        return m_lineUpdateCount;
    }

    /** Get the position in the line */
    inline int getLinePosition() const
    {
        return m_linePosition;
    }

    /** Set all values */
    void set(int lineNumber, int lineUpdateCount, int linePosition);

    /** Increments the position by 1 */
    void incrementPosition();

    /** Test if inequal */
    bool operator!=(const DinoLineBufferPosition & position) const;

    /** Assignment */
    const DinoLineBufferPosition & operator=(const DinoLineBufferPosition & position);

}; // DinoLineBufferPosition

//---------------------------------------------------------
// D i n o L i n e B u f f e r
//---------------------------------------------------------
/** This class maintains the input file and provides a
 *  convenient interface to handle it.
 */
class OGDF_EXPORT DinoLineBuffer
{

public:

    // Maximal length of a string handled by extractString()
    const static int c_maxStringLength;

    // Maximal length of one line
    const static int c_maxLineLength;

    // Maximal number of lines
    const static int c_maxNoOfLines;

private:

    // Handle to the input file
    istream* m_pIs;

    // Contains for each line of the line buffer its update count
    // Range is [0 .. c_maxNoOfLines]
    int* m_lineUpdateCountArray;

    // Pointer to the line buffer
    char* m_pLinBuf;

    // The current position in m_pLinBuf
    DinoLineBufferPosition m_currentPosition;

    // The line which has been read from the file most recently;
    // this does not have to be equal to m_currentPosition.m_lineNumber
    // because of the lookahead facilities.
    // Range is [0 .. c_maxNoOfLines - 1]
    int m_numberOfMostRecentlyReadLine;

    // Contains the current line number of the input file;
    int m_inputFileLineCounter;

public:

    // construction
    DinoLineBuffer(const char* fileName);

    // destruction
    ~DinoLineBuffer();

    // Returns the current position (as a copy)
    DinoLineBufferPosition getCurrentPosition() const
    {
        return m_currentPosition;
    }

    // Returns the character which is currently pointed to
    inline char getCurrentCharacter() const
    {
        return m_pLinBuf[(m_currentPosition.getLineNumber() * DinoLineBuffer::c_maxLineLength) +
                         m_currentPosition.getLinePosition()];
    }

    // Returns line number of the most recently read line of the input file
    inline int getInputFileLineCounter() const
    {
        return m_inputFileLineCounter;
    }

    // Moves to the next position;
    // reading of new lines and handling of eof are done internally.
    // If end of file is reached the position will stuck to EOF character.
    // The current character after moving is returned
    char moveToNextCharacter();

    // Sets the current position to new positon.
    // Takes care if the given newPosition is valid.
    // Returns false if given position is invalid
    bool setCurrentPosition(const DinoLineBufferPosition & newPosition);

    // Moves to the next character until the currentCharacter is
    // no whitespace.
    void skipWhitespace();

    // Copys the characters which have been extracted from the
    // line buffer starting from position startPosition (including it)
    // to endPosition (excluding it) to targetString (terminated by '\0').
    // The length of strings is limited to c_maxStringLength
    //
    // Returns false if the startPosition is not valid, i.e. the string
    // is too long; targetString will contain the message "String too long!"
    bool extractString(
        const DinoLineBufferPosition & startPostion,
        const DinoLineBufferPosition & endPosition,
        char* targetString);

private:

    // Returns a pointer to the character which is currently pointed to
    inline char* getCurrentCharacterPointer()
    {
        return &m_pLinBuf[(m_currentPosition.getLineNumber() * DinoLineBuffer::c_maxLineLength) +
                          m_currentPosition.getLinePosition()];
    }

    // Sets the given character to the current position
    inline void setCurrentCharacter(char c)
    {
        m_pLinBuf[(m_currentPosition.getLineNumber() * DinoLineBuffer::c_maxLineLength) +
                  m_currentPosition.getLinePosition()] = c;
    }

    // Checks wether the given position is valid
    bool isValidPosition(const DinoLineBufferPosition & position) const;

}; // class DinoLineBuffer

} // end namespace ogdf

#endif
