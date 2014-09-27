/*
 * $Revision: 2564 $
 *
 * last checkin:
 *   $Author: gutwenger $
 *   $Date: 2012-07-07 00:03:48 +0200 (Sa, 07. Jul 2012) $
 ***************************************************************/

/** \file
 * \brief Contains the struct declarations XmlAttributeObject, XmlTagObject
 * and the class DinoXmlParser.
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

#ifndef OGDF_DINO_XML_PARSER_H
#define OGDF_DINO_XML_PARSER_H

#include <ogdf/basic/Stack.h>
#include <ogdf/basic/String.h>
#include <ogdf/basic/Hashing.h>
#include <ogdf/basic/GraphAttributes.h>
#include <ogdf/fileformats/DinoXmlScanner.h>



namespace ogdf
{

//---------------------------------------------------------
// H a s h e d S t r i n g
//---------------------------------------------------------

typedef HashElement<String, int> HashedString;

//---------------------------------------------------------
// X m l A t t r i b u t e O b j e c t
//---------------------------------------------------------
/** This struct represents an attribute associated to a tag.
 */
struct OGDF_EXPORT XmlAttributeObject
{

    /** Contains the name of the attribute, i.e.
     *  for <A attr1="value1"> ... </A> it contains "attr1"
     */
    HashedString* m_pAttributeName;

    /** Contains the value assigned to this attribute without qoutes, i.e.
     *  for <A attr1="value1"> ... </A> it contains "value1" and not "\"value1\"".
     */
    HashedString* m_pAttributeValue;

    /** Pointer to the next attribute and 0 if this is the only attribute. */
    XmlAttributeObject* m_pNextAttribute;

    /** Constructor */
    XmlAttributeObject(HashedString* name, HashedString* value) :
        m_pAttributeName(name),
        m_pAttributeValue(value),
        m_pNextAttribute(0)
    {};

    /** Destructor; will be performed in destroyParseTree(). */
    ~XmlAttributeObject() {};

    /** Flag denotes whether attribute is valid or not. */
    bool m_valid;

    /** Getter. */
    const String & getName() const
    {
        return m_pAttributeName->key();
    }

    const String & getValue() const
    {
        return m_pAttributeValue->key();
    }

    const bool & valid() const
    {
        return m_valid;
    }

    /** Setter. */
    void setValid()
    {
        m_valid = true;
    }

    void setInvalid()
    {
        m_valid = false;
    }

    // Overloaded new and delete operators
    OGDF_NEW_DELETE

}; // struct XmlAttributeObject

//---------------------------------------------------------
// X m l T a g O b j e c t
//---------------------------------------------------------
/** This struct represents a node in the XML parse tree.
 */
struct OGDF_EXPORT XmlTagObject
{

    /** The identifier of the tag,
     *i.e. for <A> the identifier is "A"
     */
    HashedString* m_pTagName;

    /** Pointer to the first attribute;
     *  if there is more than one attribute these are linked by
     * m_pNextAttribute in struct XmlAttributeObject
     */
    XmlAttributeObject* m_pFirstAttribute;

    /** Contains the characters inbetween the start tag and the end tag,
     *  i.e. for <A attr1=... attr2=...> lala </A> it contains " lala "
     */
    HashedString* m_pTagValue;

    /** Contains the pointer to the first son tag object,
     *  i.e. for <A> <B> ... </B> <C> ... </C> </A> it contains a pointer
     *  to the object representing B
     *  The other children of A are reachable via m_pBrother of the first son,
     *  i.e. the variable m_pBrother of the object representing B contains a
     * pointer to the object representing C
     */
    XmlTagObject* m_pFirstSon;

    /** Contains the pointer to a brother tag object or 0 if this
     *  object is the only child
     */
    XmlTagObject* m_pBrother;

    /** Constructor */
    XmlTagObject(HashedString* name) :
        m_pTagName(name),
        m_pFirstAttribute(0),
        m_pTagValue(0),
        m_pFirstSon(0),
        m_pBrother(0),
        m_valid(0)
    {};

    /** Destructor; will be performed in destroyParseTree(). */
    ~XmlTagObject() {};

    /** Flag denotes whether attribute is valid or not. */
    mutable bool m_valid;

    /** integer value for the depth in the xml parse tree */
    int m_depth;

    /** integer value that stores the line number
     *  of the tag in the parsed xml document */
    int m_line;

public:

    /**Checks if currentNode is leaf in the parse tree.
     * Returns true if list of sons is empty.
     * Returns false otherwise.
     *
     * NEW
     */
    bool isLeaf() const;

    /**Searches for a son with tag name sonsName.
     * Returns the son via the referenced pointer son.
     * Returns true if son is found.
     * Returns false, otherwise, son is set to NULL.
     *
     * NEW
     */
    bool findSonXmlTagObjectByName(const String sonsName,
                                   XmlTagObject* & son) const;

    /**Searches for sons with tag name sonsName.
     * Returns the sons via a list with pointers to the sons.
     * Returns true if at least one son was found.
     * Returns false otherwise, sons is set to NULL.
     *
     * NEW
     */
    bool findSonXmlTagObjectByName(const String sonsName,
                                   List<XmlTagObject*> & sons) const;

    /**Searches for sons of father which names are inequal to those
     * in list sonsNames.
     * Returns true if at least one son of father is found whose name
     * doesn't match one in sonsNames.
     * Returns false otherwise.
     *
     * NEW
     */
    bool hasMoreSonXmlTagObject(const List<String> & sonNamesToIgnore) const;

    /**Searches for an attribute with name name.
     *
     * NEW
     */
    bool findXmlAttributeObjectByName(
        const String attName,
        XmlAttributeObject* & attribute) const;

    /**Checks if currentTag owns at least one attribute.
     * Returns true if list of attributes isn't empty.
     * Returns false otherwise.
     *
     * NEW
     */
    bool isAttributeLess() const;

    /** Getter. */
    const bool & valid() const
    {
        return m_valid;
    }

    const String & getName() const
    {
        return m_pTagName->key();
    }

    const String & getValue() const
    {
        return m_pTagValue->key();
    }

    /** Setter. */
    void setValid() const
    {
        m_valid = true;
    }

    void setInvalid()
    {
        m_valid = false;
    }

    /* get for depth of xml-tag-object */
    const int & getDepth() const
    {
        return m_depth;
    }

    /* setter for new depth */
    void setDepth(int newDepth)
    {
        m_depth = newDepth;
    }


    /* get for line of xml-tag-object */
    const int & getLine() const
    {
        return m_line;
    }

    /* setter for line */
    void setLine(int line)
    {
        m_line = line;
    }

    // Overloaded new and delete operators
    OGDF_NEW_DELETE

}; // struct XmlTagObject

//---------------------------------------------------------
// D i n o X m l P a r s e r
//---------------------------------------------------------
/** This class parses the XML input file and builds up a
 *  parse tree with linked elements XMLTagObject and
 *  XMLAttributeObject. The class DinoXmlScanner is used to
 *  get the token for the parse process.
 */
class OGDF_EXPORT DinoXmlParser
{

    friend ostream & operator<<(ostream &, const DinoXmlParser &);

private:

    /** Pointer to the root element of the parse tree. */
    XmlTagObject* m_pRootTag;

    /** Pointer to the scanner. */
    DinoXmlScanner* m_pScanner;

    /** Hash table for storing names of TagObjects and
     *  AttributeObjects in an efficient manner.
     *  The key element is String.
     *  The info element is int.
     */
    Hashing<String, int> m_hashTable;

    /** The info element of the hash table is simply an integer
     * number which is incremented for each new element (starting at 0).
     * The value m_hashTableInfoIndex - 1 is the last used index.
     */
    int m_hashTableInfoIndex;

    /** Recursion depth of parse(). */
    int m_recursionDepth;
    /** stack for checking correctness of correspondent closing tags */
    Stack<String> m_tagObserver;


public:

    /** Constructor.
     *  Inside the constructor the scanner is generated.
     */
    DinoXmlParser(const char* fileName);

    /** Destructor; destroys the parse tree. */
    ~DinoXmlParser();

    /** Creates a new hash element and inserts it into the hash table.
     */
    void addNewHashElement(const String & key, int info)
    {
        OGDF_ASSERT(info >= m_hashTableInfoIndex)
        m_hashTable.fastInsert(key, info);
        m_hashTableInfoIndex = info + 1;
    }

    /** Creates the parse tree and anchors it in m_pRootTag.
     *  TODO: Should return a value to indicate if success.
     */
    void createParseTree();

    /** Allows (non modifying) access to the parse tree. */
    const XmlTagObject & getRootTag() const
    {
        return *m_pRootTag;
    }

    /** Traverses the parseTree starting at startTag using the path
     * description in path, which contains the infoIndices of the tags
     * which have to be traversed.
     * If the XmlTagObject associated to the last infoIndex in the path is
     * found, it is returned via targetTag and the return value is true
     * If the XmlTagObject is not found the return value is false.
     */
    bool traversePath(
        const XmlTagObject & startTag,
        const Array<int> & infoIndexPath,
        const XmlTagObject* & targetTag) const;

    /** Searches for a specific son (identified by sonInfoIndex)
     *  of father.
     *  Returns the son via the referenced pointer son.
     *  Returns true if son is found.
     *  Returns false otherwise, son is set to NULL.
     */
    bool findSonXmlTagObject(
        const XmlTagObject & father,
        int sonInfoIndex,
        const XmlTagObject* & son) const;

    /** Searches for a specific brother (identified by brotherInfoIndex)
     *  of current.
     *  Returns the brother via the referenced pointer brother.
     *  Returns true if brother is found.
     *  Returns false otherwise, brother is set to NULL.
     */
    bool findBrotherXmlTagObject(
        const XmlTagObject & currentTag,
        int brotherInfoIndex,
        const XmlTagObject* & brother) const;

    /** Searches for a specific attribute (identified by attributeInfoIndex)
     *  of current.
     *  Returns the attribute via the referenced pointer attribute.
     *  Returns true if attribute is found.
     *  Returns false otherwise, attribute is set to NULL.
     */
    bool findXmlAttributeObject(
        const XmlTagObject & currentTag,
        int attributeInfoIndex,
        const XmlAttributeObject* & attribute) const;

    /** Returns line number of the most recently read line of
     *  the input file.
     */
    inline int getInputFileLineCounter() const
    {
        return m_pScanner->getInputFileLineCounter();
    }

    /** Prints the content of the hash table to os. */
    void printHashTable(ostream & os);

private:

    /** Destroys the parse tree appended to root. */
    void destroyParseTree(XmlTagObject* root);

    /** Parses the token stream provided by the scanner until a complete
     *  XmlTagObject is identified which will be returned.
     *  This function is likely to be called recursively
     * due to the recursive structure of XML documents.
     */
    XmlTagObject* parse();

    /** Append attributeObject to list of attributes of tagObject. */
    void appendAttributeObject(
        XmlTagObject* tagObject,
        XmlAttributeObject* attributeObject);

    /** Appends sonTagObject to the list of sons of currentTagObject. */
    void appendSonTagObject(
        XmlTagObject* currentTagObject,
        XmlTagObject* sonTagObject);

    /** Returns the hash element for the given string.
     *  If the key str is not contained in the table yet, it is
     *  inserted together with a new info index and the new
     *  hash element is returned.
     *  If the key str exists, the associated hash element is returned.
     */
    HashedString* hashString(const String & str);

    /** Prints the given XmlTagObject and its children recursively.
     *  The parameter indent is used as indentation value.
     */
    void printXmlTagObjectTree(
        ostream & os,
        const XmlTagObject & rootObject,
        int indent = 0) const;

    /** Little helper that prints nOfSpaces space characters. */
    void printSpaces(ostream & os, int nOfSpaces) const;

}; // class DinoXmlParser

/** Output operator for DinoXmlParser. */
ostream & operator<<(ostream & os, const DinoXmlParser & parser);

} // end namespace ogdf

#endif
