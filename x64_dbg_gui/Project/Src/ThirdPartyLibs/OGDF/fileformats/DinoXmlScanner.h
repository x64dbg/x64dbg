/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Contains the enum XmlToken and the class DinoXmlScanner.
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

#ifndef OGDF_DINO_XML_SCANNER_H
#define OGDF_DINO_XML_SCANNER_H

#include <ogdf/fileformats/DinoLineBuffer.h>

namespace ogdf
{

//---------------------------------------------------------
// X m l T o k e n
//---------------------------------------------------------
/** This enum type represents the values, which are returned by
 *  the function DinoXmlScanner::getNextToken().
 *  @see DinoXmlScanner::getNextToken()
 */
enum XmlToken
{
    openingBracket,     ///< <
    closingBracket,     ///< >
    questionMark,       ///< ?
    exclamationMark,    ///< !
    minus,              ///< -
    slash,              ///< /
    equalSign,          ///< =
    identifier,         ///< (a..z|A..Z){(a..z|A..Z|0..9|.|_|:)}
    attributeValue,     ///< a sequence of characters, digits, minus - and dot .
    quotedValue,        ///< all quoted content " ... " or ' ... '
    endOfFile,          ///< End of file detected
    invalidToken,       ///< No token identified
    noToken             ///< Used for the m_lookAheadToken to indicate that there
    ///< is no lookahead token
}; // enum XmlToken


//---------------------------------------------------------
// D i n o X m l S c a n n e r
//---------------------------------------------------------
/** This class scans the characters of the input file and
 *  provides the detected token.
 */
class OGDF_EXPORT DinoXmlScanner
{

private:

    // Pointer to the line buffer
    DinoLineBuffer* m_pLineBuffer;

    // String which contains the characters of the current token
    // Its size is limited to DinoLineBuffer::c_maxStringLength
    char* m_pCurrentTokenString;

public:
    // construction
    DinoXmlScanner(const char* fileName);

    // destruction: destroys the parse tree
    ~DinoXmlScanner();

    // This function represents the core of the scanner. It scans the input
    // and returns the identified token. After performing getNextToken() the
    // token is "consumed", i.e. the line buffer pointer already points to the
    // next token.
    // The scanned string is deposited in m_pCurrentTokenString, hence it is
    // available via getCurrentTokenString()
    XmlToken getNextToken();

    // Returns the current token string
    inline const char* getCurrentTokenString()
    {
        return m_pCurrentTokenString;
    }

    // This function provides a lookahead to the next token;
    // the token is NOT consumed like it is the case for getNextToken()
    XmlToken testNextToken();

    // This function provides a lookahead to the nextnext token;
    // the tokens are NOT consumed like it is the case for getNextToken()
    XmlToken testNextNextToken();

    // Skips until the searchCharacter is found;
    //
    // If skipOverSearchCharacter is set true the currentPosition will be set
    // BEHIND the search character
    // otherwise the pointer currentPosition points TO the searchCharacter
    //
    // Returns true if the searchCharacter is found
    // Returns false if file ends before the searchCharacter is found
    bool skipUntil(char searchCharacter, bool skipOverSearchCharacter = true);

    // Skips until '>' is found (> is consumed)
    // Nested brackets are taken into account
    // Returns true if matching bracket has been found; false otherwise
    bool skipUntilMatchingClosingBracket();

    // Reads until the searchCharacter is found; the string starting at the current
    // position and ending at the position where the search character is found
    // is deposited in m_pCurrentTokenString.
    // If includeSearchCharacter is false (default) the search character is
    // not contained; otherwise it is contained
    //
    // Returns true if the searchCharacter is found
    // Returns false if file ends before the searchCharacter is found
    bool readStringUntil(char searchCharacter, bool includeSearchCharacter = false);

    // Returns line number of the most recently read line of the input file
    inline int getInputFileLineCounter() const
    {
        return m_pLineBuffer->getInputFileLineCounter();
    }

    // This function tests the scanner by reading the complete
    // input file and printing the identified token to stdout
    void test();

}; // class DinoXmlScanner

} // end namespace ogdf

#endif
