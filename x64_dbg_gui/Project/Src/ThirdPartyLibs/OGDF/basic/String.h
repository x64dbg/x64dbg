/*
 * $Revision: 2619 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-16 16:05:39 +0200 (Mo, 16. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class String.
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

#ifndef OGDF_STRING_H
#define OGDF_STRING_H


#include <ogdf/basic/basic.h>
#include <ogdf/basic/Hashing.h>


#define OGDF_STRING_BUFFER_SIZE 1024


namespace ogdf
{


//! Representation of character strings.
/**
 * Strings are internally stored as an Ascii character array. The positions
 * within a string a numbered 0,1,...
 */
class OGDF_EXPORT String
{

    char*  m_pChar; //!< Pointer to characters.
    size_t m_length;  //!< The length of the string (number of characters).

    static char s_pBuffer[OGDF_STRING_BUFFER_SIZE]; //!< Temporary buffer used by sprintf().

public:
    //! Constructs an empty string, i.e., a string with length 0.
    String();
    //! Constructs a string consisting of a single character \a c.
    String(const char c);
    //! Constructs a string that is a copy of \a str.
    String(const char* str);
    //String(const char *format, ...);
    //! Constructs a string consisting of the first \a maxLen characters of \a str.
    /**
     * @param maxLen is the number of characters to be copied from the begin of \a str.
     *        If \a str is shorter than \a maxLen, then the complete string is copied.
     * @param str is the string to be copied.
     */
    String(size_t maxLen, const char* str);
    //! Constructs a string that is a copy of \a str.
    String(const String & str);

    ~String();

    //! Cast a string into a 0-terminated C++ string.
    //operator const char *() const { return m_pChar; }
    const char* cstr() const
    {
        return m_pChar;
    }

    //! Returns the length of the string.
    size_t length() const
    {
        return m_length;
    }

    //! Returns a reference to the character at position \a i.
    char & operator[](size_t i)
    {
        OGDF_ASSERT(i < m_length)
        return m_pChar[i];
    }

    //! Returns a reference to the character at position \a i.
    const char & operator[](size_t i) const
    {
        OGDF_ASSERT(i < m_length)
        return m_pChar[i];
    }

    //! Equality operator.
    friend bool operator==(const String & x, const String & y)
    {
        return (compare(x, y) == 0);
    }
    //! Equality operator.
    friend bool operator==(const char* x, const String & y)
    {
        return (compare(x, y) == 0);
    }
    //! Equality operator.
    friend bool operator==(const String & x, const char* y)
    {
        return (compare(x, y) == 0);
    }

    //! Inequality operator.
    friend bool operator!=(const String & x, const String & y)
    {
        return (compare(x, y) != 0);
    }
    //! Inequality operator.
    friend bool operator!=(const char* x, const String & y)
    {
        return (compare(x, y) != 0);
    }
    //! Inequality operator.
    friend bool operator!=(const String & x, const char* y)
    {
        return (compare(x, y) != 0);
    }

    //! Less than operator.
    friend bool operator<(const String & x, const String & y)
    {
        return (compare(x, y) < 0);
    }
    //! Less than operator.
    friend bool operator<(const char* x, const String & y)
    {
        return (compare(x, y) < 0);
    }
    //! Less than operator.
    friend bool operator<(const String & x, const char* y)
    {
        return (compare(x, y) < 0);
    }

    //! Less or equal than operator.
    friend bool operator<=(const String & x, const String & y)
    {
        return (compare(x, y) <= 0);
    }
    //! Less or equal than operator.
    friend bool operator<=(const char* x, const String & y)
    {
        return (compare(x, y) <= 0);
    }
    //! Less or equal than operator.
    friend bool operator<=(const String & x, const char* y)
    {
        return (compare(x, y) <= 0);
    }

    //! Greater than operator.
    friend bool operator>(const String & x, const String & y)
    {
        return (compare(x, y) > 0);
    }
    //! Greater than operator.
    friend bool operator>(const char* x, const String & y)
    {
        return (compare(x, y) > 0);
    }
    //! Greater than operator.
    friend bool operator>(const String & x, const char* y)
    {
        return (compare(x, y) > 0);
    }

    //! Greater or equal than operator.
    friend bool operator>=(const String & x, const String & y)
    {
        return (compare(x, y) >= 0);
    }
    //! Greater or equal than operator.
    friend bool operator>=(const char* x, const String & y)
    {
        return (compare(x, y) >= 0);
    }
    //! Greater or equal than operator.
    friend bool operator>=(const String & x, const char* y)
    {
        return (compare(x, y) >= 0);
    }

    //! Assignment operator.
    String & operator=(const String & str);
    //! Assignment operator.
    String & operator=(const char* str);

    //! Appends string \a str to this string.
    String & operator+=(const String & str);

    //! Formatted assignment operator.
    /**
     * Behaves essentially like the C function \c printf().
     */
    void sprintf(const char* format, ...);

    //! Compare function for strings.
    static int compare(const String & x, const String & y);

    //! Input operator.
    friend istream & operator>>(istream & is, String & str);

    OGDF_NEW_DELETE
};

//! Output operator for strings.
inline ostream & operator<<(ostream & os, const String & str)
{
    os << str.cstr();
    return os;
}

template<> class DefHashFunc<String>
{
public:
    int hash(const String & key) const;
};


} // end namespace ogdf


#endif
