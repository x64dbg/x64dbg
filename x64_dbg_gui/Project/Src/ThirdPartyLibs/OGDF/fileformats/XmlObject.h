/*
 * $Revision: 2523 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-02 20:59:27 +0200 (Mon, 02 Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Declaration of class XmlObject.
 *
 * \author Sebastian Leipert and Carsten Gutwenger
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

#ifndef OGDF_XML_OBJECT_H
#define OGDF_XML_OBJECT_H



namespace ogdf
{


typedef HashElement<String, int>* XmlKey;
enum XmlObjectType { xmlIntValue, xmlDoubleValue, xmlStringValue, xmlListBegin,
                     xmlListEnd, xmlKey, xmlEOF, xmlError
                   };


//---------------------------------------------------------
// XmlObject
// represents node in XML parse tree
//---------------------------------------------------------
struct OGDF_EXPORT XmlObject
{

    XmlObject* m_pBrother; // brother of node in tree
    XmlKey m_key; // tag of node
    XmlObjectType m_valueType; // type of node

    // the entry in the union is selected according to m_valueType:
    //   xmlIntValue -> m_intValue
    //   xmlDoubleValue -> m_doubleValue
    //   xmlStringValue -> m_stringValue
    //   xmlListBegin -> m_pFirstSon (in case of a list, m_pFirstSon is pointer
    //     to first son and the sons are chained by m_pBrother)
    union
    {
        int m_intValue;
        double m_doubleValue;
        const char* m_stringValue;
        XmlObject* m_pFirstSon;
    };

    // construction

    // Some Reference on the XML notation:
    // XML consists of one or more elements.
    // An element is marked with the following form:
    //      <body>
    //      elementinformation
    //      </body>
    // The opening <body> and the closing </body> are the tags
    // of the element. The text between the two tags is considered
    // part of the element.
    // Elemets can have attributes applied, e.g.
    //      <body style="bold">blablabla</body>
    // The attribute is specified inside the opening tag
    // and is called "style". It is given a value "bold" which is expressed
    // inside quotation marks.



    // Stores the "tag" of an XML element
    XmlObject(XmlKey key) : m_pBrother(0), m_key(key),
        m_valueType(xmlListBegin), m_pFirstSon(0)  { }

    // Stores an integer "attribute" of an XML element
    XmlObject(XmlKey key, int intValue) : m_pBrother(0), m_key(key),
        m_valueType(xmlIntValue), m_intValue(intValue)  { }

    // Stores a double "attribute" of an XML element
    XmlObject(XmlKey key, double doubleValue) : m_pBrother(0), m_key(key),
        m_valueType(xmlDoubleValue), m_doubleValue(doubleValue)  { }

    // Stores a string "attribute" of an XML element
    XmlObject(XmlKey key, const char* stringValue) : m_pBrother(0), m_key(key),
        m_valueType(xmlStringValue), m_stringValue(stringValue)  { }

    // Stores the body of the element
    XmlObject(const char* stringValue) : m_pBrother(0), m_key(0),
        m_valueType(xmlStringValue), m_stringValue(stringValue)  { }


    OGDF_NEW_DELETE
};

} // end namespace ogdf

#endif
