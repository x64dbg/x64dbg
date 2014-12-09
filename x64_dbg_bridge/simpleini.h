/** @mainpage

    <table>
        <tr><th>Library     <td>SimpleIni
        <tr><th>File        <td>SimpleIni.h
        <tr><th>Author      <td>Brodie Thiesfield [code at jellycan dot com]
        <tr><th>Source      <td>https://github.com/brofield/simpleini
        <tr><th>Version     <td>4.17
    </table>

    Jump to the @link CSimpleIniTempl CSimpleIni @endlink interface documentation.

    @section intro INTRODUCTION

    This component allows an INI-style configuration file to be used on both
    Windows and Linux/Unix. It is fast, simple and source code using this
    component will compile unchanged on either OS.


    @section features FEATURES

    - MIT Licence allows free use in all software (including GPL and commercial)
    - multi-platform (Windows 95/98/ME/NT/2K/XP/2003, Windows CE, Linux, Unix)
    - loading and saving of INI-style configuration files
    - configuration files can have any newline format on all platforms
    - liberal acceptance of file format
        - key/values with no section
        - removal of whitespace around sections, keys and values
    - support for multi-line values (values with embedded newline characters)
    - optional support for multiple keys with the same name
    - optional case-insensitive sections and keys (for ASCII characters only)
    - saves files with sections and keys in the same order as they were loaded
    - preserves comments on the file, section and keys where possible.
    - supports both char or wchar_t programming interfaces
    - supports both MBCS (system locale) and UTF-8 file encodings
    - system locale does not need to be UTF-8 on Linux/Unix to load UTF-8 file
    - support for non-ASCII characters in section, keys, values and comments
    - support for non-standard character types or file encodings
      via user-written converter classes
    - support for adding/modifying values programmatically
    - compiles cleanly in the following compilers:
        - Windows/VC6 (warning level 3)
        - Windows/VC.NET 2003 (warning level 4)
        - Windows/VC 2005 (warning level 4)
        - Linux/gcc (-Wall)


    @section usage USAGE SUMMARY

    -#  Define the appropriate symbol for the converter you wish to use and
        include the SimpleIni.h header file. If no specific converter is defined
        then the default converter is used. The default conversion mode uses
        SI_CONVERT_WIN32 on Windows and SI_CONVERT_GENERIC on all other
        platforms. If you are using ICU then SI_CONVERT_ICU is supported on all
        platforms.
    -#  Declare an instance the appropriate class. Note that the following
        definitions are just shortcuts for commonly used types. Other types
        (PRUnichar, unsigned short, unsigned char) are also possible.
        <table>
            <tr><th>Interface   <th>Case-sensitive  <th>Load UTF-8  <th>Load MBCS   <th>Typedef
        <tr><th>SI_CONVERT_GENERIC
            <tr><td>char        <td>No              <td>Yes         <td>Yes #1      <td>CSimpleIniA
            <tr><td>char        <td>Yes             <td>Yes         <td>Yes         <td>CSimpleIniCaseA
            <tr><td>wchar_t     <td>No              <td>Yes         <td>Yes         <td>CSimpleIniW
            <tr><td>wchar_t     <td>Yes             <td>Yes         <td>Yes         <td>CSimpleIniCaseW
        <tr><th>SI_CONVERT_WIN32
            <tr><td>char        <td>No              <td>No #2       <td>Yes         <td>CSimpleIniA
            <tr><td>char        <td>Yes             <td>Yes         <td>Yes         <td>CSimpleIniCaseA
            <tr><td>wchar_t     <td>No              <td>Yes         <td>Yes         <td>CSimpleIniW
            <tr><td>wchar_t     <td>Yes             <td>Yes         <td>Yes         <td>CSimpleIniCaseW
        <tr><th>SI_CONVERT_ICU
            <tr><td>char        <td>No              <td>Yes         <td>Yes         <td>CSimpleIniA
            <tr><td>char        <td>Yes             <td>Yes         <td>Yes         <td>CSimpleIniCaseA
            <tr><td>UChar       <td>No              <td>Yes         <td>Yes         <td>CSimpleIniW
            <tr><td>UChar       <td>Yes             <td>Yes         <td>Yes         <td>CSimpleIniCaseW
        </table>
        #1  On Windows you are better to use CSimpleIniA with SI_CONVERT_WIN32.<br>
        #2  Only affects Windows. On Windows this uses MBCS functions and
            so may fold case incorrectly leading to uncertain results.
    -# Call LoadData() or LoadFile() to load and parse the INI configuration file
    -# Access and modify the data of the file using the following functions
        <table>
            <tr><td>GetAllSections  <td>Return all section names
            <tr><td>GetAllKeys      <td>Return all key names within a section
            <tr><td>GetAllValues    <td>Return all values within a section & key
            <tr><td>GetSection      <td>Return all key names and values in a section
            <tr><td>GetSectionSize  <td>Return the number of keys in a section
            <tr><td>GetValue        <td>Return a value for a section & key
            <tr><td>SetValue        <td>Add or update a value for a section & key
            <tr><td>Delete          <td>Remove a section, or a key from a section
        </table>
    -# Call Save() or SaveFile() to save the INI configuration data

    @section iostreams IO STREAMS

    SimpleIni supports reading from and writing to STL IO streams. Enable this
    by defining SI_SUPPORT_IOSTREAMS before including the SimpleIni.h header
    file. Ensure that if the streams are backed by a file (e.g. ifstream or
    ofstream) then the flag ios_base::binary has been used when the file was
    opened.

    @section multiline MULTI-LINE VALUES

    Values that span multiple lines are created using the following format.

        <pre>
        key = <<<ENDTAG
        .... multiline value ....
        ENDTAG
        </pre>

    Note the following:
    - The text used for ENDTAG can be anything and is used to find
      where the multi-line text ends.
    - The newline after ENDTAG in the start tag, and the newline
      before ENDTAG in the end tag is not included in the data value.
    - The ending tag must be on it's own line with no whitespace before
      or after it.
    - The multi-line value is modified at load so that each line in the value
      is delimited by a single '\\n' character on all platforms. At save time
      it will be converted into the newline format used by the current
      platform.

    @section comments COMMENTS

    Comments are preserved in the file within the following restrictions:
    - Every file may have a single "file comment". It must start with the
      first character in the file, and will end with the first non-comment
      line in the file.
    - Every section may have a single "section comment". It will start
      with the first comment line following the file comment, or the last
      data entry. It ends at the beginning of the section.
    - Every key may have a single "key comment". This comment will start
      with the first comment line following the section start, or the file
      comment if there is no section name.
    - Comments are set at the time that the file, section or key is first
      created. The only way to modify a comment on a section or a key is to
      delete that entry and recreate it with the new comment. There is no
      way to change the file comment.

    @section save SAVE ORDER

    The sections and keys are written out in the same order as they were
    read in from the file. Sections and keys added to the data after the
    file has been loaded will be added to the end of the file when it is
    written. There is no way to specify the location of a section or key
    other than in first-created, first-saved order.

    @section notes NOTES

    - To load UTF-8 data on Windows 95, you need to use Microsoft Layer for
      Unicode, or SI_CONVERT_GENERIC, or SI_CONVERT_ICU.
    - When using SI_CONVERT_GENERIC, ConvertUTF.c must be compiled and linked.
    - When using SI_CONVERT_ICU, ICU header files must be on the include
      path and icuuc.lib must be linked in.
    - To load a UTF-8 file on Windows AND expose it with SI_CHAR == char,
      you should use SI_CONVERT_GENERIC.
    - The collation (sorting) order used for sections and keys returned from
      iterators is NOT DEFINED. If collation order of the text is important
      then it should be done yourself by either supplying a replacement
      SI_STRLESS class, or by sorting the strings external to this library.
    - Usage of the <mbstring.h> header on Windows can be disabled by defining
      SI_NO_MBCS. This is defined automatically on Windows CE platforms.

    @section contrib CONTRIBUTIONS

    - 2010/05/03: Tobias Gehrig: added GetDoubleValue()

    @section licence MIT LICENCE

    The licence text below is the boilerplate "MIT Licence" used from:
    http://www.opensource.org/licenses/mit-license.php

    Copyright (c) 2006-2012, Brodie Thiesfield

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is furnished
    to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
    FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
    COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
    IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef INCLUDED_SimpleIni_h
#define INCLUDED_SimpleIni_h

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

// Disable these warnings in MSVC:
//  4127 "conditional expression is constant" as the conversion classes trigger
//  it with the statement if (sizeof(SI_CHAR) == sizeof(char)). This test will
//  be optimized away in a release build.
//  4503 'insert' : decorated name length exceeded, name was truncated
//  4702 "unreachable code" as the MS STL header causes it in release mode.
//  Again, the code causing the warning will be cleaned up by the compiler.
//  4786 "identifier truncated to 256 characters" as this is thrown hundreds
//  of times VC6 as soon as STL is used.
#ifdef _MSC_VER
# pragma warning (push)
# pragma warning (disable: 4127 4503 4702 4786)
#endif

#include <cstring>
#include <string>
#include <map>
#include <list>
#include <algorithm>
#include <stdio.h>

#ifdef SI_SUPPORT_IOSTREAMS
# include <iostream>
#endif // SI_SUPPORT_IOSTREAMS

#ifdef _DEBUG
# ifndef assert
#  include <cassert>
# endif
# define SI_ASSERT(x)   assert(x)
#else
# define SI_ASSERT(x)
#endif

enum SI_Error
{
    SI_OK       =  0,   //!< No error
    SI_UPDATED  =  1,   //!< An existing value was updated
    SI_INSERTED =  2,   //!< A new value was inserted

    // note: test for any error with (retval < 0)
    SI_FAIL     = -1,   //!< Generic failure
    SI_NOMEM    = -2,   //!< Out of memory error
    SI_FILE     = -3    //!< File error (see errno for detail error)
};

#define SI_UTF8_SIGNATURE     "\xEF\xBB\xBF"

#ifdef _WIN32
# define SI_NEWLINE_A   "\r\n"
# define SI_NEWLINE_W   L"\r\n"
#else // !_WIN32
# define SI_NEWLINE_A   "\n"
# define SI_NEWLINE_W   L"\n"
#endif // _WIN32

#if defined(SI_CONVERT_ICU)
# include <unicode/ustring.h>
#endif

#if defined(_WIN32)
# define SI_HAS_WIDE_FILE
# define SI_WCHAR_T     wchar_t
#elif defined(SI_CONVERT_ICU)
# define SI_HAS_WIDE_FILE
# define SI_WCHAR_T     UChar
#endif


// ---------------------------------------------------------------------------
//                              MAIN TEMPLATE CLASS
// ---------------------------------------------------------------------------

/** Simple INI file reader.

    This can be instantiated with the choice of unicode or native characterset,
    and case sensitive or insensitive comparisons of section and key names.
    The supported combinations are pre-defined with the following typedefs:

    <table>
        <tr><th>Interface   <th>Case-sensitive  <th>Typedef
        <tr><td>char        <td>No              <td>CSimpleIniA
        <tr><td>char        <td>Yes             <td>CSimpleIniCaseA
        <tr><td>wchar_t     <td>No              <td>CSimpleIniW
        <tr><td>wchar_t     <td>Yes             <td>CSimpleIniCaseW
    </table>

    Note that using other types for the SI_CHAR is supported. For instance,
    unsigned char, unsigned short, etc. Note that where the alternative type
    is a different size to char/wchar_t you may need to supply new helper
    classes for SI_STRLESS and SI_CONVERTER.
 */
template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
class CSimpleIniTempl
{
public:
    typedef SI_CHAR SI_CHAR_T;

    /** key entry */
    struct Entry
    {
        const SI_CHAR* pItem;
        const SI_CHAR* pComment;
        int             nOrder;

        Entry(const SI_CHAR* a_pszItem = NULL, int a_nOrder = 0)
            : pItem(a_pszItem)
            , pComment(NULL)
            , nOrder(a_nOrder)
        { }
        Entry(const SI_CHAR* a_pszItem, const SI_CHAR* a_pszComment, int a_nOrder)
            : pItem(a_pszItem)
            , pComment(a_pszComment)
            , nOrder(a_nOrder)
        { }
        Entry(const Entry & rhs)
        {
            operator=(rhs);
        }
        Entry & operator=(const Entry & rhs)
        {
            pItem    = rhs.pItem;
            pComment = rhs.pComment;
            nOrder   = rhs.nOrder;
            return *this;
        }

#if defined(_MSC_VER) && _MSC_VER <= 1200
        /** STL of VC6 doesn't allow me to specify my own comparator for list::sort() */
        bool operator<(const Entry & rhs) const
        {
            return LoadOrder()(*this, rhs);
        }
        bool operator>(const Entry & rhs) const
        {
            return LoadOrder()(rhs, *this);
        }
#endif

        /** Strict less ordering by name of key only */
        struct KeyOrder : std::binary_function<Entry, Entry, bool>
        {
            bool operator()(const Entry & lhs, const Entry & rhs) const
            {
                const static SI_STRLESS isLess = SI_STRLESS();
                return isLess(lhs.pItem, rhs.pItem);
            }
        };

        /** Strict less ordering by order, and then name of key */
        struct LoadOrder : std::binary_function<Entry, Entry, bool>
        {
            bool operator()(const Entry & lhs, const Entry & rhs) const
            {
                if(lhs.nOrder != rhs.nOrder)
                {
                    return lhs.nOrder < rhs.nOrder;
                }
                return KeyOrder()(lhs.pItem, rhs.pItem);
            }
        };
    };

    /** map keys to values */
    typedef std::multimap<Entry, const SI_CHAR*, typename Entry::KeyOrder> TKeyVal;

    /** map sections to key/value map */
    typedef std::map<Entry, TKeyVal, typename Entry::KeyOrder> TSection;

    /** set of dependent string pointers. Note that these pointers are
        dependent on memory owned by CSimpleIni.
    */
    typedef std::list<Entry> TNamesDepend;

    /** interface definition for the OutputWriter object to pass to Save()
        in order to output the INI file data.
    */
    class OutputWriter
    {
    public:
        OutputWriter() { }
        virtual ~OutputWriter() { }
        virtual void Write(const char* a_pBuf) = 0;
    private:
        OutputWriter(const OutputWriter &);             // disable
        OutputWriter & operator=(const OutputWriter &); // disable
    };

    /** OutputWriter class to write the INI data to a file */
    class FileWriter : public OutputWriter
    {
        FILE* m_file;
    public:
        FileWriter(FILE* a_file) : m_file(a_file) { }
        void Write(const char* a_pBuf)
        {
            fputs(a_pBuf, m_file);
        }
    private:
        FileWriter(const FileWriter &);             // disable
        FileWriter & operator=(const FileWriter &); // disable
    };

    /** OutputWriter class to write the INI data to a string */
    class StringWriter : public OutputWriter
    {
        std::string & m_string;
    public:
        StringWriter(std::string & a_string) : m_string(a_string) { }
        void Write(const char* a_pBuf)
        {
            m_string.append(a_pBuf);
        }
    private:
        StringWriter(const StringWriter &);             // disable
        StringWriter & operator=(const StringWriter &); // disable
    };

#ifdef SI_SUPPORT_IOSTREAMS
    /** OutputWriter class to write the INI data to an ostream */
    class StreamWriter : public OutputWriter
    {
        std::ostream & m_ostream;
    public:
        StreamWriter(std::ostream & a_ostream) : m_ostream(a_ostream) { }
        void Write(const char* a_pBuf)
        {
            m_ostream << a_pBuf;
        }
    private:
        StreamWriter(const StreamWriter &);             // disable
        StreamWriter & operator=(const StreamWriter &); // disable
    };
#endif // SI_SUPPORT_IOSTREAMS

    /** Characterset conversion utility class to convert strings to the
        same format as is used for the storage.
    */
    class Converter : private SI_CONVERTER
    {
    public:
        Converter(bool a_bStoreIsUtf8) : SI_CONVERTER(a_bStoreIsUtf8)
        {
            m_scratch.resize(1024);
        }
        Converter(const Converter & rhs)
        {
            operator=(rhs);
        }
        Converter & operator=(const Converter & rhs)
        {
            m_scratch = rhs.m_scratch;
            return *this;
        }
        bool ConvertToStore(const SI_CHAR* a_pszString)
        {
            size_t uLen = SI_CONVERTER::SizeToStore(a_pszString);
            if(uLen == (size_t)(-1))
            {
                return false;
            }
            while(uLen > m_scratch.size())
            {
                m_scratch.resize(m_scratch.size() * 2);
            }
            return SI_CONVERTER::ConvertToStore(
                       a_pszString,
                       const_cast<char*>(m_scratch.data()),
                       m_scratch.size());
        }
        const char* Data()
        {
            return m_scratch.data();
        }
    private:
        std::string m_scratch;
    };

public:
    /*-----------------------------------------------------------------------*/

    /** Default constructor.

        @param a_bIsUtf8     See the method SetUnicode() for details.
        @param a_bMultiKey   See the method SetMultiKey() for details.
        @param a_bMultiLine  See the method SetMultiLine() for details.
     */
    CSimpleIniTempl(
        bool a_bIsUtf8    = false,
        bool a_bMultiKey  = false,
        bool a_bMultiLine = false
    );

    /** Destructor */
    ~CSimpleIniTempl();

    /** Deallocate all memory stored by this object */
    void Reset();

    /** Has any data been loaded */
    bool IsEmpty() const
    {
        return m_data.empty();
    }

    /*-----------------------------------------------------------------------*/
    /** @{ @name Settings */

    /** Set the storage format of the INI data. This affects both the loading
        and saving of the INI data using all of the Load/Save API functions.
        This value cannot be changed after any INI data has been loaded.

        If the file is not set to Unicode (UTF-8), then the data encoding is
        assumed to be the OS native encoding. This encoding is the system
        locale on Linux/Unix and the legacy MBCS encoding on Windows NT/2K/XP.
        If the storage format is set to Unicode then the file will be loaded
        as UTF-8 encoded data regardless of the native file encoding. If
        SI_CHAR == char then all of the char* parameters take and return UTF-8
        encoded data regardless of the system locale.

        \param a_bIsUtf8     Assume UTF-8 encoding for the source?
     */
    void SetUnicode(bool a_bIsUtf8 = true)
    {
        if(!m_pData) m_bStoreIsUtf8 = a_bIsUtf8;
    }

    /** Get the storage format of the INI data. */
    bool IsUnicode() const
    {
        return m_bStoreIsUtf8;
    }

    /** Should multiple identical keys be permitted in the file. If set to false
        then the last value encountered will be used as the value of the key.
        If set to true, then all values will be available to be queried. For
        example, with the following input:

        <pre>
        [section]
        test=value1
        test=value2
        </pre>

        Then with SetMultiKey(true), both of the values "value1" and "value2"
        will be returned for the key test. If SetMultiKey(false) is used, then
        the value for "test" will only be "value2". This value may be changed
        at any time.

        \param a_bAllowMultiKey  Allow multi-keys in the source?
     */
    void SetMultiKey(bool a_bAllowMultiKey = true)
    {
        m_bAllowMultiKey = a_bAllowMultiKey;
    }

    /** Get the storage format of the INI data. */
    bool IsMultiKey() const
    {
        return m_bAllowMultiKey;
    }

    /** Should data values be permitted to span multiple lines in the file. If
        set to false then the multi-line construct <<<TAG as a value will be
        returned as is instead of loading the data. This value may be changed
        at any time.

        \param a_bAllowMultiLine     Allow multi-line values in the source?
     */
    void SetMultiLine(bool a_bAllowMultiLine = true)
    {
        m_bAllowMultiLine = a_bAllowMultiLine;
    }

    /** Query the status of multi-line data */
    bool IsMultiLine() const
    {
        return m_bAllowMultiLine;
    }

    /** Should spaces be added around the equals sign when writing key/value
        pairs out. When true, the result will be "key = value". When false,
        the result will be "key=value". This value may be changed at any time.

        \param a_bSpaces     Add spaces around the equals sign?
     */
    void SetSpaces(bool a_bSpaces = true)
    {
        m_bSpaces = a_bSpaces;
    }

    /** Query the status of spaces output */
    bool UsingSpaces() const
    {
        return m_bSpaces;
    }

    /*-----------------------------------------------------------------------*/
    /** @}
        @{ @name Loading INI Data */

    /** Load an INI file from disk into memory

        @param a_pszFile    Path of the file to be loaded. This will be passed
                            to fopen() and so must be a valid path for the
                            current platform.

        @return SI_Error    See error definitions
     */
    SI_Error LoadFile(
        const char* a_pszFile
    );

#ifdef SI_HAS_WIDE_FILE
    /** Load an INI file from disk into memory

        @param a_pwszFile   Path of the file to be loaded in UTF-16.

        @return SI_Error    See error definitions
     */
    SI_Error LoadFile(
        const SI_WCHAR_T* a_pwszFile
    );
#endif // SI_HAS_WIDE_FILE

    /** Load the file from a file pointer.

        @param a_fpFile     Valid file pointer to read the file data from. The
                            file will be read until end of file.

        @return SI_Error    See error definitions
    */
    SI_Error LoadFile(
        FILE* a_fpFile
    );

#ifdef SI_SUPPORT_IOSTREAMS
    /** Load INI file data from an istream.

        @param a_istream    Stream to read from

        @return SI_Error    See error definitions
     */
    SI_Error LoadData(
        std::istream & a_istream
    );
#endif // SI_SUPPORT_IOSTREAMS

    /** Load INI file data direct from a std::string

        @param a_strData    Data to be loaded

        @return SI_Error    See error definitions
     */
    SI_Error LoadData(const std::string & a_strData)
    {
        return LoadData(a_strData.c_str(), a_strData.size());
    }

    /** Load INI file data direct from memory

        @param a_pData      Data to be loaded
        @param a_uDataLen   Length of the data in bytes

        @return SI_Error    See error definitions
     */
    SI_Error LoadData(
        const char*     a_pData,
        size_t          a_uDataLen
    );

    /*-----------------------------------------------------------------------*/
    /** @}
        @{ @name Saving INI Data */

    /** Save an INI file from memory to disk

        @param a_pszFile    Path of the file to be saved. This will be passed
                            to fopen() and so must be a valid path for the
                            current platform.

        @param a_bAddSignature  Prepend the UTF-8 BOM if the output data is
                            in UTF-8 format. If it is not UTF-8 then
                            this parameter is ignored.

        @return SI_Error    See error definitions
     */
    SI_Error SaveFile(
        const char*     a_pszFile,
        bool            a_bAddSignature = true
    ) const;

#ifdef SI_HAS_WIDE_FILE
    /** Save an INI file from memory to disk

        @param a_pwszFile   Path of the file to be saved in UTF-16.

        @param a_bAddSignature  Prepend the UTF-8 BOM if the output data is
                            in UTF-8 format. If it is not UTF-8 then
                            this parameter is ignored.

        @return SI_Error    See error definitions
     */
    SI_Error SaveFile(
        const SI_WCHAR_T*   a_pwszFile,
        bool                a_bAddSignature = true
    ) const;
#endif // _WIN32

    /** Save the INI data to a file. See Save() for details.

        @param a_pFile      Handle to a file. File should be opened for
                            binary output.

        @param a_bAddSignature  Prepend the UTF-8 BOM if the output data is in
                            UTF-8 format. If it is not UTF-8 then this value is
                            ignored. Do not set this to true if anything has
                            already been written to the file.

        @return SI_Error    See error definitions
     */
    SI_Error SaveFile(
        FILE*   a_pFile,
        bool    a_bAddSignature = false
    ) const;

    /** Save the INI data. The data will be written to the output device
        in a format appropriate to the current data, selected by:

        <table>
            <tr><th>SI_CHAR     <th>FORMAT
            <tr><td>char        <td>same format as when loaded (MBCS or UTF-8)
            <tr><td>wchar_t     <td>UTF-8
            <tr><td>other       <td>UTF-8
        </table>

        Note that comments from the original data is preserved as per the
        documentation on comments. The order of the sections and values
        from the original file will be preserved.

        Any data prepended or appended to the output device must use the the
        same format (MBCS or UTF-8). You may use the GetConverter() method to
        convert text to the correct format regardless of the output format
        being used by SimpleIni.

        To add a BOM to UTF-8 data, write it out manually at the very beginning
        like is done in SaveFile when a_bUseBOM is true.

        @param a_oOutput    Output writer to write the data to.

        @param a_bAddSignature  Prepend the UTF-8 BOM if the output data is in
                            UTF-8 format. If it is not UTF-8 then this value is
                            ignored. Do not set this to true if anything has
                            already been written to the OutputWriter.

        @return SI_Error    See error definitions
     */
    SI_Error Save(
        OutputWriter  & a_oOutput,
        bool            a_bAddSignature = false
    ) const;

#ifdef SI_SUPPORT_IOSTREAMS
    /** Save the INI data to an ostream. See Save() for details.

        @param a_ostream    String to have the INI data appended to.

        @param a_bAddSignature  Prepend the UTF-8 BOM if the output data is in
                            UTF-8 format. If it is not UTF-8 then this value is
                            ignored. Do not set this to true if anything has
                            already been written to the stream.

        @return SI_Error    See error definitions
     */
    SI_Error Save(
        std::ostream  & a_ostream,
        bool            a_bAddSignature = false
    ) const
    {
        StreamWriter writer(a_ostream);
        return Save(writer, a_bAddSignature);
    }
#endif // SI_SUPPORT_IOSTREAMS

    /** Append the INI data to a string. See Save() for details.

        @param a_sBuffer    String to have the INI data appended to.

        @param a_bAddSignature  Prepend the UTF-8 BOM if the output data is in
                            UTF-8 format. If it is not UTF-8 then this value is
                            ignored. Do not set this to true if anything has
                            already been written to the string.

        @return SI_Error    See error definitions
     */
    SI_Error Save(
        std::string  &  a_sBuffer,
        bool            a_bAddSignature = false
    ) const
    {
        StringWriter writer(a_sBuffer);
        return Save(writer, a_bAddSignature);
    }

    /*-----------------------------------------------------------------------*/
    /** @}
        @{ @name Accessing INI Data */

    /** Retrieve all section names. The list is returned as an STL vector of
        names and can be iterated or searched as necessary. Note that the
        sort order of the returned strings is NOT DEFINED. You can sort
        the names into the load order if desired. Search this file for ".sort"
        for an example.

        NOTE! This structure contains only pointers to strings. The actual
        string data is stored in memory owned by CSimpleIni. Ensure that the
        CSimpleIni object is not destroyed or Reset() while these pointers
        are in use!

        @param a_names          Vector that will receive all of the section
                                 names. See note above!
     */
    void GetAllSections(
        TNamesDepend & a_names
    ) const;

    /** Retrieve all unique key names in a section. The sort order of the
        returned strings is NOT DEFINED. You can sort the names into the load
        order if desired. Search this file for ".sort" for an example. Only
        unique key names are returned.

        NOTE! This structure contains only pointers to strings. The actual
        string data is stored in memory owned by CSimpleIni. Ensure that the
        CSimpleIni object is not destroyed or Reset() while these strings
        are in use!

        @param a_pSection       Section to request data for
        @param a_names          List that will receive all of the key
                                 names. See note above!

        @return true            Section was found.
        @return false           Matching section was not found.
     */
    bool GetAllKeys(
        const SI_CHAR* a_pSection,
        TNamesDepend  & a_names
    ) const;

    /** Retrieve all values for a specific key. This method can be used when
        multiple keys are both enabled and disabled. Note that the sort order
        of the returned strings is NOT DEFINED. You can sort the names into
        the load order if desired. Search this file for ".sort" for an example.

        NOTE! The returned values are pointers to string data stored in memory
        owned by CSimpleIni. Ensure that the CSimpleIni object is not destroyed
        or Reset while you are using this pointer!

        @param a_pSection       Section to search
        @param a_pKey           Key to search for
        @param a_values         List to return if the key is not found

        @return true            Key was found.
        @return false           Matching section/key was not found.
     */
    bool GetAllValues(
        const SI_CHAR* a_pSection,
        const SI_CHAR* a_pKey,
        TNamesDepend  & a_values
    ) const;

    /** Query the number of keys in a specific section. Note that if multiple
        keys are enabled, then this value may be different to the number of
        keys returned by GetAllKeys.

        @param a_pSection       Section to request data for

        @return -1              Section does not exist in the file
        @return >=0             Number of keys in the section
     */
    int GetSectionSize(
        const SI_CHAR* a_pSection
    ) const;

    /** Retrieve all key and value pairs for a section. The data is returned
        as a pointer to an STL map and can be iterated or searched as
        desired. Note that multiple entries for the same key may exist when
        multiple keys have been enabled.

        NOTE! This structure contains only pointers to strings. The actual
        string data is stored in memory owned by CSimpleIni. Ensure that the
        CSimpleIni object is not destroyed or Reset() while these strings
        are in use!

        @param a_pSection       Name of the section to return
        @return boolean         Was a section matching the supplied
                                name found.
     */
    const TKeyVal* GetSection(
        const SI_CHAR* a_pSection
    ) const;

    /** Retrieve the value for a specific key. If multiple keys are enabled
        (see SetMultiKey) then only the first value associated with that key
        will be returned, see GetAllValues for getting all values with multikey.

        NOTE! The returned value is a pointer to string data stored in memory
        owned by CSimpleIni. Ensure that the CSimpleIni object is not destroyed
        or Reset while you are using this pointer!

        @param a_pSection       Section to search
        @param a_pKey           Key to search for
        @param a_pDefault       Value to return if the key is not found
        @param a_pHasMultiple   Optionally receive notification of if there are
                                multiple entries for this key.

        @return a_pDefault      Key was not found in the section
        @return other           Value of the key
     */
    const SI_CHAR* GetValue(
        const SI_CHAR* a_pSection,
        const SI_CHAR* a_pKey,
        const SI_CHAR* a_pDefault     = NULL,
        bool*           a_pHasMultiple = NULL
    ) const;

    /** Retrieve a numeric value for a specific key. If multiple keys are enabled
        (see SetMultiKey) then only the first value associated with that key
        will be returned, see GetAllValues for getting all values with multikey.

        @param a_pSection       Section to search
        @param a_pKey           Key to search for
        @param a_nDefault       Value to return if the key is not found
        @param a_pHasMultiple   Optionally receive notification of if there are
                                multiple entries for this key.

        @return a_nDefault      Key was not found in the section
        @return other           Value of the key
     */
    long GetLongValue(
        const SI_CHAR* a_pSection,
        const SI_CHAR* a_pKey,
        long            a_nDefault     = 0,
        bool*           a_pHasMultiple = NULL
    ) const;

    /** Retrieve a numeric value for a specific key. If multiple keys are enabled
        (see SetMultiKey) then only the first value associated with that key
        will be returned, see GetAllValues for getting all values with multikey.

        @param a_pSection       Section to search
        @param a_pKey           Key to search for
        @param a_nDefault       Value to return if the key is not found
        @param a_pHasMultiple   Optionally receive notification of if there are
                                multiple entries for this key.

        @return a_nDefault      Key was not found in the section
        @return other           Value of the key
     */
    double GetDoubleValue(
        const SI_CHAR* a_pSection,
        const SI_CHAR* a_pKey,
        double          a_nDefault     = 0,
        bool*           a_pHasMultiple = NULL
    ) const;

    /** Retrieve a boolean value for a specific key. If multiple keys are enabled
        (see SetMultiKey) then only the first value associated with that key
        will be returned, see GetAllValues for getting all values with multikey.

        Strings starting with "t", "y", "on" or "1" are returned as logically true.
        Strings starting with "f", "n", "of" or "0" are returned as logically false.
        For all other values the default is returned. Character comparisons are
        case-insensitive.

        @param a_pSection       Section to search
        @param a_pKey           Key to search for
        @param a_bDefault       Value to return if the key is not found
        @param a_pHasMultiple   Optionally receive notification of if there are
                                multiple entries for this key.

        @return a_nDefault      Key was not found in the section
        @return other           Value of the key
     */
    bool GetBoolValue(
        const SI_CHAR* a_pSection,
        const SI_CHAR* a_pKey,
        bool            a_bDefault     = false,
        bool*           a_pHasMultiple = NULL
    ) const;

    /** Add or update a section or value. This will always insert
        when multiple keys are enabled.

        @param a_pSection   Section to add or update
        @param a_pKey       Key to add or update. Set to NULL to
                            create an empty section.
        @param a_pValue     Value to set. Set to NULL to create an
                            empty section.
        @param a_pComment   Comment to be associated with the section or the
                            key. If a_pKey is NULL then it will be associated
                            with the section, otherwise the key. Note that a
                            comment may be set ONLY when the section or key is
                            first created (i.e. when this function returns the
                            value SI_INSERTED). If you wish to create a section
                            with a comment then you need to create the section
                            separately to the key. The comment string must be
                            in full comment form already (have a comment
                            character starting every line).
        @param a_bForceReplace  Should all existing values in a multi-key INI
                            file be replaced with this entry. This option has
                            no effect if not using multi-key files. The
                            difference between Delete/SetValue and SetValue
                            with a_bForceReplace = true, is that the load
                            order and comment will be preserved this way.

        @return SI_Error    See error definitions
        @return SI_UPDATED  Value was updated
        @return SI_INSERTED Value was inserted
     */
    SI_Error SetValue(
        const SI_CHAR* a_pSection,
        const SI_CHAR* a_pKey,
        const SI_CHAR* a_pValue,
        const SI_CHAR* a_pComment      = NULL,
        bool            a_bForceReplace = false
    )
    {
        return AddEntry(a_pSection, a_pKey, a_pValue, a_pComment, a_bForceReplace, true);
    }

    /** Add or update a numeric value. This will always insert
        when multiple keys are enabled.

        @param a_pSection   Section to add or update
        @param a_pKey       Key to add or update.
        @param a_nValue     Value to set.
        @param a_pComment   Comment to be associated with the key. See the
                            notes on SetValue() for comments.
        @param a_bUseHex    By default the value will be written to the file
                            in decimal format. Set this to true to write it
                            as hexadecimal.
        @param a_bForceReplace  Should all existing values in a multi-key INI
                            file be replaced with this entry. This option has
                            no effect if not using multi-key files. The
                            difference between Delete/SetLongValue and
                            SetLongValue with a_bForceReplace = true, is that
                            the load order and comment will be preserved this
                            way.

        @return SI_Error    See error definitions
        @return SI_UPDATED  Value was updated
        @return SI_INSERTED Value was inserted
     */
    SI_Error SetLongValue(
        const SI_CHAR* a_pSection,
        const SI_CHAR* a_pKey,
        long            a_nValue,
        const SI_CHAR* a_pComment      = NULL,
        bool            a_bUseHex       = false,
        bool            a_bForceReplace = false
    );

    /** Add or update a double value. This will always insert
        when multiple keys are enabled.

        @param a_pSection   Section to add or update
        @param a_pKey       Key to add or update.
        @param a_nValue     Value to set.
        @param a_pComment   Comment to be associated with the key. See the
                            notes on SetValue() for comments.
        @param a_bForceReplace  Should all existing values in a multi-key INI
                            file be replaced with this entry. This option has
                            no effect if not using multi-key files. The
                            difference between Delete/SetDoubleValue and
                            SetDoubleValue with a_bForceReplace = true, is that
                            the load order and comment will be preserved this
                            way.

        @return SI_Error    See error definitions
        @return SI_UPDATED  Value was updated
        @return SI_INSERTED Value was inserted
     */
    SI_Error SetDoubleValue(
        const SI_CHAR* a_pSection,
        const SI_CHAR* a_pKey,
        double          a_nValue,
        const SI_CHAR* a_pComment      = NULL,
        bool            a_bForceReplace = false
    );

    /** Add or update a boolean value. This will always insert
        when multiple keys are enabled.

        @param a_pSection   Section to add or update
        @param a_pKey       Key to add or update.
        @param a_bValue     Value to set.
        @param a_pComment   Comment to be associated with the key. See the
                            notes on SetValue() for comments.
        @param a_bForceReplace  Should all existing values in a multi-key INI
                            file be replaced with this entry. This option has
                            no effect if not using multi-key files. The
                            difference between Delete/SetBoolValue and
                            SetBoolValue with a_bForceReplace = true, is that
                            the load order and comment will be preserved this
                            way.

        @return SI_Error    See error definitions
        @return SI_UPDATED  Value was updated
        @return SI_INSERTED Value was inserted
     */
    SI_Error SetBoolValue(
        const SI_CHAR* a_pSection,
        const SI_CHAR* a_pKey,
        bool            a_bValue,
        const SI_CHAR* a_pComment      = NULL,
        bool            a_bForceReplace = false
    );

    /** Delete an entire section, or a key from a section. Note that the
        data returned by GetSection is invalid and must not be used after
        anything has been deleted from that section using this method.
        Note when multiple keys is enabled, this will delete all keys with
        that name; to selectively delete individual key/values, use
        DeleteValue.

        @param a_pSection       Section to delete key from, or if
                                a_pKey is NULL, the section to remove.
        @param a_pKey           Key to remove from the section. Set to
                                NULL to remove the entire section.
        @param a_bRemoveEmpty   If the section is empty after this key has
                                been deleted, should the empty section be
                                removed?

        @return true            Key or section was deleted.
        @return false           Key or section was not found.
     */
    bool Delete(
        const SI_CHAR* a_pSection,
        const SI_CHAR* a_pKey,
        bool            a_bRemoveEmpty = false
    );

    /** Delete an entire section, or a key from a section. If value is
        provided, only remove keys with the value. Note that the data
        returned by GetSection is invalid and must not be used after
        anything has been deleted from that section using this method.
        Note when multiple keys is enabled, all keys with the value will
        be deleted.

        @param a_pSection       Section to delete key from, or if
                                a_pKey is NULL, the section to remove.
        @param a_pKey           Key to remove from the section. Set to
                                NULL to remove the entire section.
        @param a_pValue         Value of key to remove from the section.
                                Set to NULL to remove all keys.
        @param a_bRemoveEmpty   If the section is empty after this key has
                                been deleted, should the empty section be
                                removed?

        @return true            Key/value or section was deleted.
        @return false           Key/value or section was not found.
     */
    bool DeleteValue(
        const SI_CHAR* a_pSection,
        const SI_CHAR* a_pKey,
        const SI_CHAR* a_pValue,
        bool            a_bRemoveEmpty = false
    );

    /*-----------------------------------------------------------------------*/
    /** @}
        @{ @name Converter */

    /** Return a conversion object to convert text to the same encoding
        as is used by the Save(), SaveFile() and SaveString() functions.
        Use this to prepare the strings that you wish to append or prepend
        to the output INI data.
     */
    Converter GetConverter() const
    {
        return Converter(m_bStoreIsUtf8);
    }

    /*-----------------------------------------------------------------------*/
    /** @} */

private:
    // copying is not permitted
    CSimpleIniTempl(const CSimpleIniTempl &); // disabled
    CSimpleIniTempl & operator=(const CSimpleIniTempl &); // disabled

    /** Parse the data looking for a file comment and store it if found.
    */
    SI_Error FindFileComment(
        SI_CHAR*   &   a_pData,
        bool            a_bCopyStrings
    );

    /** Parse the data looking for the next valid entry. The memory pointed to
        by a_pData is modified by inserting NULL characters. The pointer is
        updated to the current location in the block of text.
    */
    bool FindEntry(
        SI_CHAR* & a_pData,
        const SI_CHAR* & a_pSection,
        const SI_CHAR* & a_pKey,
        const SI_CHAR* & a_pVal,
        const SI_CHAR* & a_pComment
    ) const;

    /** Add the section/key/value to our data.

        @param a_pSection   Section name. Sections will be created if they
                            don't already exist.
        @param a_pKey       Key name. May be NULL to create an empty section.
                            Existing entries will be updated. New entries will
                            be created.
        @param a_pValue     Value for the key.
        @param a_pComment   Comment to be associated with the section or the
                            key. If a_pKey is NULL then it will be associated
                            with the section, otherwise the key. This must be
                            a string in full comment form already (have a
                            comment character starting every line).
        @param a_bForceReplace  Should all existing values in a multi-key INI
                            file be replaced with this entry. This option has
                            no effect if not using multi-key files. The
                            difference between Delete/AddEntry and AddEntry
                            with a_bForceReplace = true, is that the load
                            order and comment will be preserved this way.
        @param a_bCopyStrings   Should copies of the strings be made or not.
                            If false then the pointers will be used as is.
    */
    SI_Error AddEntry(
        const SI_CHAR* a_pSection,
        const SI_CHAR* a_pKey,
        const SI_CHAR* a_pValue,
        const SI_CHAR* a_pComment,
        bool            a_bForceReplace,
        bool            a_bCopyStrings
    );

    /** Is the supplied character a whitespace character? */
    inline bool IsSpace(SI_CHAR ch) const
    {
        return (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
    }

    /** Does the supplied character start a comment line? */
    inline bool IsComment(SI_CHAR ch) const
    {
        return (ch == ';' || ch == '#');
    }


    /** Skip over a newline character (or characters) for either DOS or UNIX */
    inline void SkipNewLine(SI_CHAR* & a_pData) const
    {
        a_pData += (*a_pData == '\r' && *(a_pData + 1) == '\n') ? 2 : 1;
    }

    /** Make a copy of the supplied string, replacing the original pointer */
    SI_Error CopyString(const SI_CHAR* & a_pString);

    /** Delete a string from the copied strings buffer if necessary */
    void DeleteString(const SI_CHAR* a_pString);

    /** Internal use of our string comparison function */
    bool IsLess(const SI_CHAR* a_pLeft, const SI_CHAR* a_pRight) const
    {
        const static SI_STRLESS isLess = SI_STRLESS();
        return isLess(a_pLeft, a_pRight);
    }

    bool IsMultiLineTag(const SI_CHAR* a_pData) const;
    bool IsMultiLineData(const SI_CHAR* a_pData) const;
    bool LoadMultiLineText(
        SI_CHAR*     &     a_pData,
        const SI_CHAR*  &  a_pVal,
        const SI_CHAR*      a_pTagName,
        bool                a_bAllowBlankLinesInComment = false
    ) const;
    bool IsNewLineChar(SI_CHAR a_c) const;

    bool OutputMultiLineText(
        OutputWriter  & a_oOutput,
        Converter   &   a_oConverter,
        const SI_CHAR* a_pText
    ) const;

private:
    /** Copy of the INI file data in our character format. This will be
        modified when parsed to have NULL characters added after all
        interesting string entries. All of the string pointers to sections,
        keys and values point into this block of memory.
     */
    SI_CHAR* m_pData;

    /** Length of the data that we have stored. Used when deleting strings
        to determine if the string is stored here or in the allocated string
        buffer.
     */
    size_t m_uDataLen;

    /** File comment for this data, if one exists. */
    const SI_CHAR* m_pFileComment;

    /** Parsed INI data. Section -> (Key -> Value). */
    TSection m_data;

    /** This vector stores allocated memory for copies of strings that have
        been supplied after the file load. It will be empty unless SetValue()
        has been called.
     */
    TNamesDepend m_strings;

    /** Is the format of our datafile UTF-8 or MBCS? */
    bool m_bStoreIsUtf8;

    /** Are multiple values permitted for the same key? */
    bool m_bAllowMultiKey;

    /** Are data values permitted to span multiple lines? */
    bool m_bAllowMultiLine;

    /** Should spaces be written out surrounding the equals sign? */
    bool m_bSpaces;

    /** Next order value, used to ensure sections and keys are output in the
        same order that they are loaded/added.
     */
    int m_nOrder;
};

// ---------------------------------------------------------------------------
//                                  IMPLEMENTATION
// ---------------------------------------------------------------------------

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::CSimpleIniTempl(
    bool a_bIsUtf8,
    bool a_bAllowMultiKey,
    bool a_bAllowMultiLine
)
    : m_pData(0)
    , m_uDataLen(0)
    , m_pFileComment(NULL)
    , m_bStoreIsUtf8(a_bIsUtf8)
    , m_bAllowMultiKey(a_bAllowMultiKey)
    , m_bAllowMultiLine(a_bAllowMultiLine)
    , m_bSpaces(false)
    , m_nOrder(0)
{ }

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::~CSimpleIniTempl()
{
    Reset();
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
void
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::Reset()
{
    // remove all data
    delete[] m_pData;
    m_pData = NULL;
    m_uDataLen = 0;
    m_pFileComment = NULL;
    if(!m_data.empty())
    {
        m_data.erase(m_data.begin(), m_data.end());
    }

    // remove all strings
    if(!m_strings.empty())
    {
        typename TNamesDepend::iterator i = m_strings.begin();
        for(; i != m_strings.end(); ++i)
        {
            delete[] const_cast<SI_CHAR*>(i->pItem);
        }
        m_strings.erase(m_strings.begin(), m_strings.end());
    }
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
SI_Error
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::LoadFile(
    const char* a_pszFile
)
{
    FILE* fp = NULL;
#if __STDC_WANT_SECURE_LIB__ && !_WIN32_WCE
    fopen_s(&fp, a_pszFile, "rb");
#else // !__STDC_WANT_SECURE_LIB__
    fp = fopen(a_pszFile, "rb");
#endif // __STDC_WANT_SECURE_LIB__
    if(!fp)
    {
        return SI_FILE;
    }
    SI_Error rc = LoadFile(fp);
    fclose(fp);
    return rc;
}

#ifdef SI_HAS_WIDE_FILE
template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
SI_Error
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::LoadFile(
    const SI_WCHAR_T* a_pwszFile
)
{
#ifdef _WIN32
    FILE* fp = NULL;
#if __STDC_WANT_SECURE_LIB__ && !_WIN32_WCE
    _wfopen_s(&fp, a_pwszFile, L"rb");
#else // !__STDC_WANT_SECURE_LIB__
    fp = _wfopen(a_pwszFile, L"rb");
#endif // __STDC_WANT_SECURE_LIB__
    if(!fp) return SI_FILE;
    SI_Error rc = LoadFile(fp);
    fclose(fp);
    return rc;
#else // !_WIN32 (therefore SI_CONVERT_ICU)
    char szFile[256];
    u_austrncpy(szFile, a_pwszFile, sizeof(szFile));
    return LoadFile(szFile);
#endif // _WIN32
}
#endif // SI_HAS_WIDE_FILE

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
SI_Error
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::LoadFile(
    FILE* a_fpFile
)
{
    // load the raw file data
    int retval = fseek(a_fpFile, 0, SEEK_END);
    if(retval != 0)
    {
        return SI_FILE;
    }
    long lSize = ftell(a_fpFile);
    if(lSize < 0)
    {
        return SI_FILE;
    }
    if(lSize == 0)
    {
        return SI_OK;
    }

    // allocate and ensure NULL terminated
    char* pData = new char[lSize + 1];
    if(!pData)
    {
        return SI_NOMEM;
    }
    pData[lSize] = 0;

    // load data into buffer
    fseek(a_fpFile, 0, SEEK_SET);
    size_t uRead = fread(pData, sizeof(char), lSize, a_fpFile);
    if(uRead != (size_t) lSize)
    {
        delete[] pData;
        return SI_FILE;
    }

    // convert the raw data to unicode
    SI_Error rc = LoadData(pData, uRead);
    delete[] pData;
    return rc;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
SI_Error
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::LoadData(
    const char*     a_pData,
    size_t          a_uDataLen
)
{
    SI_CONVERTER converter(m_bStoreIsUtf8);

    if(a_uDataLen == 0)
    {
        return SI_OK;
    }

    // consume the UTF-8 BOM if it exists
    if(m_bStoreIsUtf8 && a_uDataLen >= 3)
    {
        if(memcmp(a_pData, SI_UTF8_SIGNATURE, 3) == 0)
        {
            a_pData    += 3;
            a_uDataLen -= 3;
        }
    }

    // determine the length of the converted data
    size_t uLen = converter.SizeFromStore(a_pData, a_uDataLen);
    if(uLen == (size_t)(-1))
    {
        return SI_FAIL;
    }

    // allocate memory for the data, ensure that there is a NULL
    // terminator wherever the converted data ends
    SI_CHAR* pData = new SI_CHAR[uLen + 1];
    if(!pData)
    {
        return SI_NOMEM;
    }
    memset(pData, 0, sizeof(SI_CHAR) * (uLen + 1));

    // convert the data
    if(!converter.ConvertFromStore(a_pData, a_uDataLen, pData, uLen))
    {
        delete[] pData;
        return SI_FAIL;
    }

    // parse it
    const static SI_CHAR empty = 0;
    SI_CHAR* pWork = pData;
    const SI_CHAR* pSection = &empty;
    const SI_CHAR* pItem = NULL;
    const SI_CHAR* pVal = NULL;
    const SI_CHAR* pComment = NULL;

    // We copy the strings if we are loading data into this class when we
    // already have stored some.
    bool bCopyStrings = (m_pData != NULL);

    // find a file comment if it exists, this is a comment that starts at the
    // beginning of the file and continues until the first blank line.
    SI_Error rc = FindFileComment(pWork, bCopyStrings);
    if(rc < 0) return rc;

    // add every entry in the file to the data table
    while(FindEntry(pWork, pSection, pItem, pVal, pComment))
    {
        rc = AddEntry(pSection, pItem, pVal, pComment, false, bCopyStrings);
        if(rc < 0) return rc;
    }

    // store these strings if we didn't copy them
    if(bCopyStrings)
    {
        delete[] pData;
    }
    else
    {
        m_pData = pData;
        m_uDataLen = uLen + 1;
    }

    return SI_OK;
}

#ifdef SI_SUPPORT_IOSTREAMS
template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
SI_Error
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::LoadData(
    std::istream & a_istream
)
{
    std::string strData;
    char szBuf[512];
    do
    {
        a_istream.get(szBuf, sizeof(szBuf), '\0');
        strData.append(szBuf);
    }
    while(a_istream.good());
    return LoadData(strData);
}
#endif // SI_SUPPORT_IOSTREAMS

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
SI_Error
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::FindFileComment(
    SI_CHAR*   &   a_pData,
    bool            a_bCopyStrings
)
{
    // there can only be a single file comment
    if(m_pFileComment)
    {
        return SI_OK;
    }

    // Load the file comment as multi-line text, this will modify all of
    // the newline characters to be single \n chars
    if(!LoadMultiLineText(a_pData, m_pFileComment, NULL, false))
    {
        return SI_OK;
    }

    // copy the string if necessary
    if(a_bCopyStrings)
    {
        SI_Error rc = CopyString(m_pFileComment);
        if(rc < 0) return rc;
    }

    return SI_OK;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
bool
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::FindEntry(
    SI_CHAR*    &    a_pData,
    const SI_CHAR* & a_pSection,
    const SI_CHAR* & a_pKey,
    const SI_CHAR* & a_pVal,
    const SI_CHAR* & a_pComment
) const
{
    a_pComment = NULL;

    SI_CHAR* pTrail = NULL;
    while(*a_pData)
    {
        // skip spaces and empty lines
        while(*a_pData && IsSpace(*a_pData))
        {
            ++a_pData;
        }
        if(!*a_pData)
        {
            break;
        }

        // skip processing of comment lines but keep a pointer to
        // the start of the comment.
        if(IsComment(*a_pData))
        {
            LoadMultiLineText(a_pData, a_pComment, NULL, true);
            continue;
        }

        // process section names
        if(*a_pData == '[')
        {
            // skip leading spaces
            ++a_pData;
            while(*a_pData && IsSpace(*a_pData))
            {
                ++a_pData;
            }

            // find the end of the section name (it may contain spaces)
            // and convert it to lowercase as necessary
            a_pSection = a_pData;
            while(*a_pData && *a_pData != ']' && !IsNewLineChar(*a_pData))
            {
                ++a_pData;
            }

            // if it's an invalid line, just skip it
            if(*a_pData != ']')
            {
                continue;
            }

            // remove trailing spaces from the section
            pTrail = a_pData - 1;
            while(pTrail >= a_pSection && IsSpace(*pTrail))
            {
                --pTrail;
            }
            ++pTrail;
            *pTrail = 0;

            // skip to the end of the line
            ++a_pData;  // safe as checked that it == ']' above
            while(*a_pData && !IsNewLineChar(*a_pData))
            {
                ++a_pData;
            }

            a_pKey = NULL;
            a_pVal = NULL;
            return true;
        }

        // find the end of the key name (it may contain spaces)
        // and convert it to lowercase as necessary
        a_pKey = a_pData;
        while(*a_pData && *a_pData != '=' && !IsNewLineChar(*a_pData))
        {
            ++a_pData;
        }

        // if it's an invalid line, just skip it
        if(*a_pData != '=')
        {
            continue;
        }

        // empty keys are invalid
        if(a_pKey == a_pData)
        {
            while(*a_pData && !IsNewLineChar(*a_pData))
            {
                ++a_pData;
            }
            continue;
        }

        // remove trailing spaces from the key
        pTrail = a_pData - 1;
        while(pTrail >= a_pKey && IsSpace(*pTrail))
        {
            --pTrail;
        }
        ++pTrail;
        *pTrail = 0;

        // skip leading whitespace on the value
        ++a_pData;  // safe as checked that it == '=' above
        while(*a_pData && !IsNewLineChar(*a_pData) && IsSpace(*a_pData))
        {
            ++a_pData;
        }

        // find the end of the value which is the end of this line
        a_pVal = a_pData;
        while(*a_pData && !IsNewLineChar(*a_pData))
        {
            ++a_pData;
        }

        // remove trailing spaces from the value
        pTrail = a_pData - 1;
        if(*a_pData)    // prepare for the next round
        {
            SkipNewLine(a_pData);
        }
        while(pTrail >= a_pVal && IsSpace(*pTrail))
        {
            --pTrail;
        }
        ++pTrail;
        *pTrail = 0;

        // check for multi-line entries
        if(m_bAllowMultiLine && IsMultiLineTag(a_pVal))
        {
            // skip the "<<<" to get the tag that will end the multiline
            const SI_CHAR* pTagName = a_pVal + 3;
            return LoadMultiLineText(a_pData, a_pVal, pTagName);
        }

        // return the standard entry
        return true;
    }

    return false;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
bool
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::IsMultiLineTag(
    const SI_CHAR* a_pVal
) const
{
    // check for the "<<<" prefix for a multi-line entry
    if(*a_pVal++ != '<') return false;
    if(*a_pVal++ != '<') return false;
    if(*a_pVal++ != '<') return false;
    return true;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
bool
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::IsMultiLineData(
    const SI_CHAR* a_pData
) const
{
    // data is multi-line if it has any of the following features:
    //  * whitespace prefix
    //  * embedded newlines
    //  * whitespace suffix

    // empty string
    if(!*a_pData)
    {
        return false;
    }

    // check for prefix
    if(IsSpace(*a_pData))
    {
        return true;
    }

    // embedded newlines
    while(*a_pData)
    {
        if(IsNewLineChar(*a_pData))
        {
            return true;
        }
        ++a_pData;
    }

    // check for suffix
    if(IsSpace(*--a_pData))
    {
        return true;
    }

    return false;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
bool
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::IsNewLineChar(
    SI_CHAR a_c
) const
{
    return (a_c == '\n' || a_c == '\r');
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
bool
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::LoadMultiLineText(
    SI_CHAR*     &     a_pData,
    const SI_CHAR*  &  a_pVal,
    const SI_CHAR*      a_pTagName,
    bool                a_bAllowBlankLinesInComment
) const
{
    // we modify this data to strip all newlines down to a single '\n'
    // character. This means that on Windows we need to strip out some
    // characters which will make the data shorter.
    // i.e.  LINE1-LINE1\r\nLINE2-LINE2\0 will become
    //       LINE1-LINE1\nLINE2-LINE2\0
    // The pDataLine entry is the pointer to the location in memory that
    // the current line needs to start to run following the existing one.
    // This may be the same as pCurrLine in which case no move is needed.
    SI_CHAR* pDataLine = a_pData;
    SI_CHAR* pCurrLine;

    // value starts at the current line
    a_pVal = a_pData;

    // find the end tag. This tag must start in column 1 and be
    // followed by a newline. No whitespace removal is done while
    // searching for this tag.
    SI_CHAR cEndOfLineChar = *a_pData;
    for(;;)
    {
        // if we are loading comments then we need a comment character as
        // the first character on every line
        if(!a_pTagName && !IsComment(*a_pData))
        {
            // if we aren't allowing blank lines then we're done
            if(!a_bAllowBlankLinesInComment)
            {
                break;
            }

            // if we are allowing blank lines then we only include them
            // in this comment if another comment follows, so read ahead
            // to find out.
            SI_CHAR* pCurr = a_pData;
            int nNewLines = 0;
            while(IsSpace(*pCurr))
            {
                if(IsNewLineChar(*pCurr))
                {
                    ++nNewLines;
                    SkipNewLine(pCurr);
                }
                else
                {
                    ++pCurr;
                }
            }

            // we have a comment, add the blank lines to the output
            // and continue processing from here
            if(IsComment(*pCurr))
            {
                for(; nNewLines > 0; --nNewLines) *pDataLine++ = '\n';
                a_pData = pCurr;
                continue;
            }

            // the comment ends here
            break;
        }

        // find the end of this line
        pCurrLine = a_pData;
        while(*a_pData && !IsNewLineChar(*a_pData)) ++a_pData;

        // move this line down to the location that it should be if necessary
        if(pDataLine < pCurrLine)
        {
            size_t nLen = (size_t)(a_pData - pCurrLine);
            memmove(pDataLine, pCurrLine, nLen * sizeof(SI_CHAR));
            pDataLine[nLen] = '\0';
        }

        // end the line with a NULL
        cEndOfLineChar = *a_pData;
        *a_pData = 0;

        // if are looking for a tag then do the check now. This is done before
        // checking for end of the data, so that if we have the tag at the end
        // of the data then the tag is removed correctly.
        if(a_pTagName &&
                (!IsLess(pDataLine, a_pTagName) && !IsLess(a_pTagName, pDataLine)))
        {
            break;
        }

        // if we are at the end of the data then we just automatically end
        // this entry and return the current data.
        if(!cEndOfLineChar)
        {
            return true;
        }

        // otherwise we need to process this newline to ensure that it consists
        // of just a single \n character.
        pDataLine += (a_pData - pCurrLine);
        *a_pData = cEndOfLineChar;
        SkipNewLine(a_pData);
        *pDataLine++ = '\n';
    }

    // if we didn't find a comment at all then return false
    if(a_pVal == a_pData)
    {
        a_pVal = NULL;
        return false;
    }

    // the data (which ends at the end of the last line) needs to be
    // null-terminated BEFORE before the newline character(s). If the
    // user wants a new line in the multi-line data then they need to
    // add an empty line before the tag.
    *--pDataLine = '\0';

    // if looking for a tag and if we aren't at the end of the data,
    // then move a_pData to the start of the next line.
    if(a_pTagName && cEndOfLineChar)
    {
        SI_ASSERT(IsNewLineChar(cEndOfLineChar));
        *a_pData = cEndOfLineChar;
        SkipNewLine(a_pData);
    }

    return true;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
SI_Error
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::CopyString(
    const SI_CHAR* & a_pString
)
{
    size_t uLen = 0;
    if(sizeof(SI_CHAR) == sizeof(char))
    {
        uLen = strlen((const char*)a_pString);
    }
    else if(sizeof(SI_CHAR) == sizeof(wchar_t))
    {
        uLen = wcslen((const wchar_t*)a_pString);
    }
    else
    {
        for(; a_pString[uLen]; ++uLen) /*loop*/ ;
    }
    ++uLen; // NULL character
    SI_CHAR* pCopy = new SI_CHAR[uLen];
    if(!pCopy)
    {
        return SI_NOMEM;
    }
    memcpy(pCopy, a_pString, sizeof(SI_CHAR)*uLen);
    m_strings.push_back(pCopy);
    a_pString = pCopy;
    return SI_OK;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
SI_Error
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::AddEntry(
    const SI_CHAR* a_pSection,
    const SI_CHAR* a_pKey,
    const SI_CHAR* a_pValue,
    const SI_CHAR* a_pComment,
    bool            a_bForceReplace,
    bool            a_bCopyStrings
)
{
    SI_Error rc;
    bool bInserted = false;

    SI_ASSERT(!a_pComment || IsComment(*a_pComment));

    // if we are copying strings then make a copy of the comment now
    // because we will need it when we add the entry.
    if(a_bCopyStrings && a_pComment)
    {
        rc = CopyString(a_pComment);
        if(rc < 0) return rc;
    }

    // create the section entry if necessary
    typename TSection::iterator iSection = m_data.find(a_pSection);
    if(iSection == m_data.end())
    {
        // if the section doesn't exist then we need a copy as the
        // string needs to last beyond the end of this function
        if(a_bCopyStrings)
        {
            rc = CopyString(a_pSection);
            if(rc < 0) return rc;
        }

        // only set the comment if this is a section only entry
        Entry oSection(a_pSection, ++m_nOrder);
        if(a_pComment && (!a_pKey || !a_pValue))
        {
            oSection.pComment = a_pComment;
        }

        typename TSection::value_type oEntry(oSection, TKeyVal());
        typedef typename TSection::iterator SectionIterator;
        std::pair<SectionIterator, bool> i = m_data.insert(oEntry);
        iSection = i.first;
        bInserted = true;
    }
    if(!a_pKey || !a_pValue)
    {
        // section only entries are specified with pItem and pVal as NULL
        return bInserted ? SI_INSERTED : SI_UPDATED;
    }

    // check for existence of the key
    TKeyVal & keyval = iSection->second;
    typename TKeyVal::iterator iKey = keyval.find(a_pKey);

    // remove all existing entries but save the load order and
    // comment of the first entry
    int nLoadOrder = ++m_nOrder;
    if(iKey != keyval.end() && m_bAllowMultiKey && a_bForceReplace)
    {
        const SI_CHAR* pComment = NULL;
        while(iKey != keyval.end() && !IsLess(a_pKey, iKey->first.pItem))
        {
            if(iKey->first.nOrder < nLoadOrder)
            {
                nLoadOrder = iKey->first.nOrder;
                pComment   = iKey->first.pComment;
            }
            ++iKey;
        }
        if(pComment)
        {
            DeleteString(a_pComment);
            a_pComment = pComment;
            rc = CopyString(a_pComment);
            if(rc < 0) return rc;
        }
        Delete(a_pSection, a_pKey);
        iKey = keyval.end();
    }

    // make string copies if necessary
    bool bForceCreateNewKey = m_bAllowMultiKey && !a_bForceReplace;
    if(a_bCopyStrings)
    {
        if(bForceCreateNewKey || iKey == keyval.end())
        {
            // if the key doesn't exist then we need a copy as the
            // string needs to last beyond the end of this function
            // because we will be inserting the key next
            rc = CopyString(a_pKey);
            if(rc < 0) return rc;
        }

        // we always need a copy of the value
        rc = CopyString(a_pValue);
        if(rc < 0) return rc;
    }

    // create the key entry
    if(iKey == keyval.end() || bForceCreateNewKey)
    {
        Entry oKey(a_pKey, nLoadOrder);
        if(a_pComment)
        {
            oKey.pComment = a_pComment;
        }
        typename TKeyVal::value_type oEntry(oKey, static_cast<const SI_CHAR*>(NULL));
        iKey = keyval.insert(oEntry);
        bInserted = true;
    }
    iKey->second = a_pValue;
    return bInserted ? SI_INSERTED : SI_UPDATED;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
const SI_CHAR*
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::GetValue(
    const SI_CHAR* a_pSection,
    const SI_CHAR* a_pKey,
    const SI_CHAR* a_pDefault,
    bool*           a_pHasMultiple
) const
{
    if(a_pHasMultiple)
    {
        *a_pHasMultiple = false;
    }
    if(!a_pSection || !a_pKey)
    {
        return a_pDefault;
    }
    typename TSection::const_iterator iSection = m_data.find(a_pSection);
    if(iSection == m_data.end())
    {
        return a_pDefault;
    }
    typename TKeyVal::const_iterator iKeyVal = iSection->second.find(a_pKey);
    if(iKeyVal == iSection->second.end())
    {
        return a_pDefault;
    }

    // check for multiple entries with the same key
    if(m_bAllowMultiKey && a_pHasMultiple)
    {
        typename TKeyVal::const_iterator iTemp = iKeyVal;
        if(++iTemp != iSection->second.end())
        {
            if(!IsLess(a_pKey, iTemp->first.pItem))
            {
                *a_pHasMultiple = true;
            }
        }
    }

    return iKeyVal->second;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
long
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::GetLongValue(
    const SI_CHAR* a_pSection,
    const SI_CHAR* a_pKey,
    long            a_nDefault,
    bool*           a_pHasMultiple
) const
{
    // return the default if we don't have a value
    const SI_CHAR* pszValue = GetValue(a_pSection, a_pKey, NULL, a_pHasMultiple);
    if(!pszValue || !*pszValue) return a_nDefault;

    // convert to UTF-8/MBCS which for a numeric value will be the same as ASCII
    char szValue[64] = { 0 };
    SI_CONVERTER c(m_bStoreIsUtf8);
    if(!c.ConvertToStore(pszValue, szValue, sizeof(szValue)))
    {
        return a_nDefault;
    }

    // handle the value as hex if prefaced with "0x"
    long nValue = a_nDefault;
    char* pszSuffix = szValue;
    if(szValue[0] == '0' && (szValue[1] == 'x' || szValue[1] == 'X'))
    {
        if(!szValue[2]) return a_nDefault;
        nValue = strtol(&szValue[2], &pszSuffix, 16);
    }
    else
    {
        nValue = strtol(szValue, &pszSuffix, 10);
    }

    // any invalid strings will return the default value
    if(*pszSuffix)
    {
        return a_nDefault;
    }

    return nValue;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
SI_Error
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::SetLongValue(
    const SI_CHAR* a_pSection,
    const SI_CHAR* a_pKey,
    long            a_nValue,
    const SI_CHAR* a_pComment,
    bool            a_bUseHex,
    bool            a_bForceReplace
)
{
    // use SetValue to create sections
    if(!a_pSection || !a_pKey) return SI_FAIL;

    // convert to an ASCII string
    char szInput[64];
#if __STDC_WANT_SECURE_LIB__ && !_WIN32_WCE
    sprintf_s(szInput, a_bUseHex ? "0x%lx" : "%ld", a_nValue);
#else // !__STDC_WANT_SECURE_LIB__
    sprintf(szInput, a_bUseHex ? "0x%lx" : "%ld", a_nValue);
#endif // __STDC_WANT_SECURE_LIB__

    // convert to output text
    SI_CHAR szOutput[64];
    SI_CONVERTER c(m_bStoreIsUtf8);
    c.ConvertFromStore(szInput, strlen(szInput) + 1,
                       szOutput, sizeof(szOutput) / sizeof(SI_CHAR));

    // actually add it
    return AddEntry(a_pSection, a_pKey, szOutput, a_pComment, a_bForceReplace, true);
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
double
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::GetDoubleValue(
    const SI_CHAR* a_pSection,
    const SI_CHAR* a_pKey,
    double          a_nDefault,
    bool*           a_pHasMultiple
) const
{
    // return the default if we don't have a value
    const SI_CHAR* pszValue = GetValue(a_pSection, a_pKey, NULL, a_pHasMultiple);
    if(!pszValue || !*pszValue) return a_nDefault;

    // convert to UTF-8/MBCS which for a numeric value will be the same as ASCII
    char szValue[64] = { 0 };
    SI_CONVERTER c(m_bStoreIsUtf8);
    if(!c.ConvertToStore(pszValue, szValue, sizeof(szValue)))
    {
        return a_nDefault;
    }

    char* pszSuffix = NULL;
    double nValue = strtod(szValue, &pszSuffix);

    // any invalid strings will return the default value
    if(!pszSuffix || *pszSuffix)
    {
        return a_nDefault;
    }

    return nValue;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
SI_Error
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::SetDoubleValue(
    const SI_CHAR* a_pSection,
    const SI_CHAR* a_pKey,
    double          a_nValue,
    const SI_CHAR* a_pComment,
    bool            a_bForceReplace
)
{
    // use SetValue to create sections
    if(!a_pSection || !a_pKey) return SI_FAIL;

    // convert to an ASCII string
    char szInput[64];
#if __STDC_WANT_SECURE_LIB__ && !_WIN32_WCE
    sprintf_s(szInput, "%f", a_nValue);
#else // !__STDC_WANT_SECURE_LIB__
    sprintf(szInput, "%f", a_nValue);
#endif // __STDC_WANT_SECURE_LIB__

    // convert to output text
    SI_CHAR szOutput[64];
    SI_CONVERTER c(m_bStoreIsUtf8);
    c.ConvertFromStore(szInput, strlen(szInput) + 1,
                       szOutput, sizeof(szOutput) / sizeof(SI_CHAR));

    // actually add it
    return AddEntry(a_pSection, a_pKey, szOutput, a_pComment, a_bForceReplace, true);
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
bool
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::GetBoolValue(
    const SI_CHAR* a_pSection,
    const SI_CHAR* a_pKey,
    bool            a_bDefault,
    bool*           a_pHasMultiple
) const
{
    // return the default if we don't have a value
    const SI_CHAR* pszValue = GetValue(a_pSection, a_pKey, NULL, a_pHasMultiple);
    if(!pszValue || !*pszValue) return a_bDefault;

    // we only look at the minimum number of characters
    switch(pszValue[0])
    {
    case 't':
    case 'T': // true
    case 'y':
    case 'Y': // yes
    case '1':           // 1 (one)
        return true;

    case 'f':
    case 'F': // false
    case 'n':
    case 'N': // no
    case '0':           // 0 (zero)
        return false;

    case 'o':
    case 'O':
        if(pszValue[1] == 'n' || pszValue[1] == 'N') return true;   // on
        if(pszValue[1] == 'f' || pszValue[1] == 'F') return false;  // off
        break;
    }

    // no recognized value, return the default
    return a_bDefault;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
SI_Error
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::SetBoolValue(
    const SI_CHAR* a_pSection,
    const SI_CHAR* a_pKey,
    bool            a_bValue,
    const SI_CHAR* a_pComment,
    bool            a_bForceReplace
)
{
    // use SetValue to create sections
    if(!a_pSection || !a_pKey) return SI_FAIL;

    // convert to an ASCII string
    const char* pszInput = a_bValue ? "true" : "false";

    // convert to output text
    SI_CHAR szOutput[64];
    SI_CONVERTER c(m_bStoreIsUtf8);
    c.ConvertFromStore(pszInput, strlen(pszInput) + 1,
                       szOutput, sizeof(szOutput) / sizeof(SI_CHAR));

    // actually add it
    return AddEntry(a_pSection, a_pKey, szOutput, a_pComment, a_bForceReplace, true);
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
bool
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::GetAllValues(
    const SI_CHAR* a_pSection,
    const SI_CHAR* a_pKey,
    TNamesDepend  & a_values
) const
{
    a_values.clear();

    if(!a_pSection || !a_pKey)
    {
        return false;
    }
    typename TSection::const_iterator iSection = m_data.find(a_pSection);
    if(iSection == m_data.end())
    {
        return false;
    }
    typename TKeyVal::const_iterator iKeyVal = iSection->second.find(a_pKey);
    if(iKeyVal == iSection->second.end())
    {
        return false;
    }

    // insert all values for this key
    a_values.push_back(Entry(iKeyVal->second, iKeyVal->first.pComment, iKeyVal->first.nOrder));
    if(m_bAllowMultiKey)
    {
        ++iKeyVal;
        while(iKeyVal != iSection->second.end() && !IsLess(a_pKey, iKeyVal->first.pItem))
        {
            a_values.push_back(Entry(iKeyVal->second, iKeyVal->first.pComment, iKeyVal->first.nOrder));
            ++iKeyVal;
        }
    }

    return true;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
int
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::GetSectionSize(
    const SI_CHAR* a_pSection
) const
{
    if(!a_pSection)
    {
        return -1;
    }

    typename TSection::const_iterator iSection = m_data.find(a_pSection);
    if(iSection == m_data.end())
    {
        return -1;
    }
    const TKeyVal & section = iSection->second;

    // if multi-key isn't permitted then the section size is
    // the number of keys that we have.
    if(!m_bAllowMultiKey || section.empty())
    {
        return (int) section.size();
    }

    // otherwise we need to count them
    int nCount = 0;
    const SI_CHAR* pLastKey = NULL;
    typename TKeyVal::const_iterator iKeyVal = section.begin();
    for(int n = 0; iKeyVal != section.end(); ++iKeyVal, ++n)
    {
        if(!pLastKey || IsLess(pLastKey, iKeyVal->first.pItem))
        {
            ++nCount;
            pLastKey = iKeyVal->first.pItem;
        }
    }
    return nCount;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
const typename CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::TKeyVal*
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::GetSection(
    const SI_CHAR* a_pSection
) const
{
    if(a_pSection)
    {
        typename TSection::const_iterator i = m_data.find(a_pSection);
        if(i != m_data.end())
        {
            return &(i->second);
        }
    }
    return 0;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
void
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::GetAllSections(
    TNamesDepend & a_names
) const
{
    a_names.clear();
    typename TSection::const_iterator i = m_data.begin();
    for(int n = 0; i != m_data.end(); ++i, ++n)
    {
        a_names.push_back(i->first);
    }
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
bool
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::GetAllKeys(
    const SI_CHAR* a_pSection,
    TNamesDepend  & a_names
) const
{
    a_names.clear();

    if(!a_pSection)
    {
        return false;
    }

    typename TSection::const_iterator iSection = m_data.find(a_pSection);
    if(iSection == m_data.end())
    {
        return false;
    }

    const TKeyVal & section = iSection->second;
    const SI_CHAR* pLastKey = NULL;
    typename TKeyVal::const_iterator iKeyVal = section.begin();
    for(int n = 0; iKeyVal != section.end(); ++iKeyVal, ++n)
    {
        if(!pLastKey || IsLess(pLastKey, iKeyVal->first.pItem))
        {
            a_names.push_back(iKeyVal->first);
            pLastKey = iKeyVal->first.pItem;
        }
    }

    return true;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
SI_Error
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::SaveFile(
    const char*     a_pszFile,
    bool            a_bAddSignature
) const
{
    FILE* fp = NULL;
#if __STDC_WANT_SECURE_LIB__ && !_WIN32_WCE
    fopen_s(&fp, a_pszFile, "wb");
#else // !__STDC_WANT_SECURE_LIB__
    fp = fopen(a_pszFile, "wb");
#endif // __STDC_WANT_SECURE_LIB__
    if(!fp) return SI_FILE;
    SI_Error rc = SaveFile(fp, a_bAddSignature);
    fclose(fp);
    return rc;
}

#ifdef SI_HAS_WIDE_FILE
template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
SI_Error
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::SaveFile(
    const SI_WCHAR_T*   a_pwszFile,
    bool                a_bAddSignature
) const
{
#ifdef _WIN32
    FILE* fp = NULL;
#if __STDC_WANT_SECURE_LIB__ && !_WIN32_WCE
    _wfopen_s(&fp, a_pwszFile, L"wb");
#else // !__STDC_WANT_SECURE_LIB__
    fp = _wfopen(a_pwszFile, L"wb");
#endif // __STDC_WANT_SECURE_LIB__
    if(!fp) return SI_FILE;
    SI_Error rc = SaveFile(fp, a_bAddSignature);
    fclose(fp);
    return rc;
#else // !_WIN32 (therefore SI_CONVERT_ICU)
    char szFile[256];
    u_austrncpy(szFile, a_pwszFile, sizeof(szFile));
    return SaveFile(szFile, a_bAddSignature);
#endif // _WIN32
}
#endif // SI_HAS_WIDE_FILE

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
SI_Error
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::SaveFile(
    FILE*   a_pFile,
    bool    a_bAddSignature
) const
{
    FileWriter writer(a_pFile);
    return Save(writer, a_bAddSignature);
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
SI_Error
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::Save(
    OutputWriter  & a_oOutput,
    bool            a_bAddSignature
) const
{
    Converter convert(m_bStoreIsUtf8);

    // add the UTF-8 signature if it is desired
    if(m_bStoreIsUtf8 && a_bAddSignature)
    {
        a_oOutput.Write(SI_UTF8_SIGNATURE);
    }

    // get all of the sections sorted in load order
    TNamesDepend oSections;
    GetAllSections(oSections);
#if defined(_MSC_VER) && _MSC_VER <= 1200
    oSections.sort();
#elif defined(__BORLANDC__)
    oSections.sort(Entry::LoadOrder());
#else
    oSections.sort(typename Entry::LoadOrder());
#endif

    // write the file comment if we have one
    bool bNeedNewLine = false;
    if(m_pFileComment)
    {
        if(!OutputMultiLineText(a_oOutput, convert, m_pFileComment))
        {
            return SI_FAIL;
        }
        bNeedNewLine = true;
    }

    // iterate through our sections and output the data
    typename TNamesDepend::const_iterator iSection = oSections.begin();
    for(; iSection != oSections.end(); ++iSection)
    {
        // write out the comment if there is one
        if(iSection->pComment)
        {
            if(bNeedNewLine)
            {
                a_oOutput.Write(SI_NEWLINE_A);
            }
            if(!OutputMultiLineText(a_oOutput, convert, iSection->pComment))
            {
                return SI_FAIL;
            }
            bNeedNewLine = false;
        }

        if(bNeedNewLine)
        {
            a_oOutput.Write(SI_NEWLINE_A);
            bNeedNewLine = false;
        }

        // write the section (unless there is no section name)
        if(*iSection->pItem)
        {
            if(!convert.ConvertToStore(iSection->pItem))
            {
                return SI_FAIL;
            }
            a_oOutput.Write("[");
            a_oOutput.Write(convert.Data());
            a_oOutput.Write("]");
            a_oOutput.Write(SI_NEWLINE_A);
        }

        // get all of the keys sorted in load order
        TNamesDepend oKeys;
        GetAllKeys(iSection->pItem, oKeys);
#if defined(_MSC_VER) && _MSC_VER <= 1200
        oKeys.sort();
#elif defined(__BORLANDC__)
        oKeys.sort(Entry::LoadOrder());
#else
        oKeys.sort(typename Entry::LoadOrder());
#endif

        // write all keys and values
        typename TNamesDepend::const_iterator iKey = oKeys.begin();
        for(; iKey != oKeys.end(); ++iKey)
        {
            // get all values for this key
            TNamesDepend oValues;
            GetAllValues(iSection->pItem, iKey->pItem, oValues);

            typename TNamesDepend::const_iterator iValue = oValues.begin();
            for(; iValue != oValues.end(); ++iValue)
            {
                // write out the comment if there is one
                if(iValue->pComment)
                {
                    a_oOutput.Write(SI_NEWLINE_A);
                    if(!OutputMultiLineText(a_oOutput, convert, iValue->pComment))
                    {
                        return SI_FAIL;
                    }
                }

                // write the key
                if(!convert.ConvertToStore(iKey->pItem))
                {
                    return SI_FAIL;
                }
                a_oOutput.Write(convert.Data());

                // write the value
                if(!convert.ConvertToStore(iValue->pItem))
                {
                    return SI_FAIL;
                }
                a_oOutput.Write(m_bSpaces ? " = " : "=");
                if(m_bAllowMultiLine && IsMultiLineData(iValue->pItem))
                {
                    // multi-line data needs to be processed specially to ensure
                    // that we use the correct newline format for the current system
                    a_oOutput.Write("<<<END_OF_TEXT" SI_NEWLINE_A);
                    if(!OutputMultiLineText(a_oOutput, convert, iValue->pItem))
                    {
                        return SI_FAIL;
                    }
                    a_oOutput.Write("END_OF_TEXT");
                }
                else
                {
                    a_oOutput.Write(convert.Data());
                }
                a_oOutput.Write(SI_NEWLINE_A);
            }
        }

        bNeedNewLine = true;
    }

    return SI_OK;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
bool
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::OutputMultiLineText(
    OutputWriter  & a_oOutput,
    Converter   &   a_oConverter,
    const SI_CHAR* a_pText
) const
{
    const SI_CHAR* pEndOfLine;
    SI_CHAR cEndOfLineChar = *a_pText;
    while(cEndOfLineChar)
    {
        // find the end of this line
        pEndOfLine = a_pText;
        for(; *pEndOfLine && *pEndOfLine != '\n'; ++pEndOfLine) /*loop*/ ;
        cEndOfLineChar = *pEndOfLine;

        // temporarily null terminate, convert and output the line
        *const_cast<SI_CHAR*>(pEndOfLine) = 0;
        if(!a_oConverter.ConvertToStore(a_pText))
        {
            return false;
        }
        *const_cast<SI_CHAR*>(pEndOfLine) = cEndOfLineChar;
        a_pText += (pEndOfLine - a_pText) + 1;
        a_oOutput.Write(a_oConverter.Data());
        a_oOutput.Write(SI_NEWLINE_A);
    }
    return true;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
bool
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::Delete(
    const SI_CHAR* a_pSection,
    const SI_CHAR* a_pKey,
    bool            a_bRemoveEmpty
)
{
    return DeleteValue(a_pSection, a_pKey, NULL, a_bRemoveEmpty);
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
bool
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::DeleteValue(
    const SI_CHAR* a_pSection,
    const SI_CHAR* a_pKey,
    const SI_CHAR* a_pValue,
    bool            a_bRemoveEmpty
)
{
    if(!a_pSection)
    {
        return false;
    }

    typename TSection::iterator iSection = m_data.find(a_pSection);
    if(iSection == m_data.end())
    {
        return false;
    }

    // remove a single key if we have a keyname
    if(a_pKey)
    {
        typename TKeyVal::iterator iKeyVal = iSection->second.find(a_pKey);
        if(iKeyVal == iSection->second.end())
        {
            return false;
        }

        const static SI_STRLESS isLess = SI_STRLESS();

        // remove any copied strings and then the key
        typename TKeyVal::iterator iDelete;
        bool bDeleted = false;
        do
        {
            iDelete = iKeyVal++;

            if(a_pValue == NULL ||
                    (isLess(a_pValue, iDelete->second) == false &&
                     isLess(iDelete->second, a_pValue) == false))
            {
                DeleteString(iDelete->first.pItem);
                DeleteString(iDelete->second);
                iSection->second.erase(iDelete);
                bDeleted = true;
            }
        }
        while(iKeyVal != iSection->second.end()
                && !IsLess(a_pKey, iKeyVal->first.pItem));

        if(!bDeleted)
        {
            return false;
        }

        // done now if the section is not empty or we are not pruning away
        // the empty sections. Otherwise let it fall through into the section
        // deletion code
        if(!a_bRemoveEmpty || !iSection->second.empty())
        {
            return true;
        }
    }
    else
    {
        // delete all copied strings from this section. The actual
        // entries will be removed when the section is removed.
        typename TKeyVal::iterator iKeyVal = iSection->second.begin();
        for(; iKeyVal != iSection->second.end(); ++iKeyVal)
        {
            DeleteString(iKeyVal->first.pItem);
            DeleteString(iKeyVal->second);
        }
    }

    // delete the section itself
    DeleteString(iSection->first.pItem);
    m_data.erase(iSection);

    return true;
}

template<class SI_CHAR, class SI_STRLESS, class SI_CONVERTER>
void
CSimpleIniTempl<SI_CHAR, SI_STRLESS, SI_CONVERTER>::DeleteString(
    const SI_CHAR* a_pString
)
{
    // strings may exist either inside the data block, or they will be
    // individually allocated and stored in m_strings. We only physically
    // delete those stored in m_strings.
    if(a_pString < m_pData || a_pString >= m_pData + m_uDataLen)
    {
        typename TNamesDepend::iterator i = m_strings.begin();
        for(; i != m_strings.end(); ++i)
        {
            if(a_pString == i->pItem)
            {
                delete[] const_cast<SI_CHAR*>(i->pItem);
                m_strings.erase(i);
                break;
            }
        }
    }
}

// ---------------------------------------------------------------------------
//                              CONVERSION FUNCTIONS
// ---------------------------------------------------------------------------

// Defines the conversion classes for different libraries. Before including
// SimpleIni.h, set the converter that you wish you use by defining one of the
// following symbols.
//
//  SI_CONVERT_GENERIC      Use the Unicode reference conversion library in
//                          the accompanying files ConvertUTF.h/c
//  SI_CONVERT_ICU          Use the IBM ICU conversion library. Requires
//                          ICU headers on include path and icuuc.lib
//  SI_CONVERT_WIN32        Use the Win32 API functions for conversion.

#if !defined(SI_CONVERT_GENERIC) && !defined(SI_CONVERT_WIN32) && !defined(SI_CONVERT_ICU)
# ifdef _WIN32
#  define SI_CONVERT_WIN32
# else
#  define SI_CONVERT_GENERIC
# endif
#endif

/**
 * Generic case-sensitive less than comparison. This class returns numerically
 * ordered ASCII case-sensitive text for all possible sizes and types of
 * SI_CHAR.
 */
template<class SI_CHAR>
struct SI_GenericCase
{
    bool operator()(const SI_CHAR* pLeft, const SI_CHAR* pRight) const
    {
        long cmp;
        for(; *pLeft && *pRight; ++pLeft, ++pRight)
        {
            cmp = (long) * pLeft - (long) * pRight;
            if(cmp != 0)
            {
                return cmp < 0;
            }
        }
        return *pRight != 0;
    }
};

/**
 * Generic ASCII case-insensitive less than comparison. This class returns
 * numerically ordered ASCII case-insensitive text for all possible sizes
 * and types of SI_CHAR. It is not safe for MBCS text comparison where
 * ASCII A-Z characters are used in the encoding of multi-byte characters.
 */
template<class SI_CHAR>
struct SI_GenericNoCase
{
    inline SI_CHAR locase(SI_CHAR ch) const
    {
        return (ch < 'A' || ch > 'Z') ? ch : (ch - 'A' + 'a');
    }
    bool operator()(const SI_CHAR* pLeft, const SI_CHAR* pRight) const
    {
        long cmp;
        for(; *pLeft && *pRight; ++pLeft, ++pRight)
        {
            cmp = (long) locase(*pLeft) - (long) locase(*pRight);
            if(cmp != 0)
            {
                return cmp < 0;
            }
        }
        return *pRight != 0;
    }
};

/**
 * Null conversion class for MBCS/UTF-8 to char (or equivalent).
 */
template<class SI_CHAR>
class SI_ConvertA
{
    bool m_bStoreIsUtf8;
protected:
    SI_ConvertA() { }
public:
    SI_ConvertA(bool a_bStoreIsUtf8) : m_bStoreIsUtf8(a_bStoreIsUtf8) { }

    /* copy and assignment */
    SI_ConvertA(const SI_ConvertA & rhs)
    {
        operator=(rhs);
    }
    SI_ConvertA & operator=(const SI_ConvertA & rhs)
    {
        m_bStoreIsUtf8 = rhs.m_bStoreIsUtf8;
        return *this;
    }

    /** Calculate the number of SI_CHAR required for converting the input
     * from the storage format. The storage format is always UTF-8 or MBCS.
     *
     * @param a_pInputData  Data in storage format to be converted to SI_CHAR.
     * @param a_uInputDataLen Length of storage format data in bytes. This
     *                      must be the actual length of the data, including
     *                      NULL byte if NULL terminated string is required.
     * @return              Number of SI_CHAR required by the string when
     *                      converted. If there are embedded NULL bytes in the
     *                      input data, only the string up and not including
     *                      the NULL byte will be converted.
     * @return              -1 cast to size_t on a conversion error.
     */
    size_t SizeFromStore(
        const char*     a_pInputData,
        size_t          a_uInputDataLen)
    {
        (void)a_pInputData;
        SI_ASSERT(a_uInputDataLen != (size_t) - 1);

        // ASCII/MBCS/UTF-8 needs no conversion
        return a_uInputDataLen;
    }

    /** Convert the input string from the storage format to SI_CHAR.
     * The storage format is always UTF-8 or MBCS.
     *
     * @param a_pInputData  Data in storage format to be converted to SI_CHAR.
     * @param a_uInputDataLen Length of storage format data in bytes. This
     *                      must be the actual length of the data, including
     *                      NULL byte if NULL terminated string is required.
     * @param a_pOutputData Pointer to the output buffer to received the
     *                      converted data.
     * @param a_uOutputDataSize Size of the output buffer in SI_CHAR.
     * @return              true if all of the input data was successfully
     *                      converted.
     */
    bool ConvertFromStore(
        const char*     a_pInputData,
        size_t          a_uInputDataLen,
        SI_CHAR*        a_pOutputData,
        size_t          a_uOutputDataSize)
    {
        // ASCII/MBCS/UTF-8 needs no conversion
        if(a_uInputDataLen > a_uOutputDataSize)
        {
            return false;
        }
        memcpy(a_pOutputData, a_pInputData, a_uInputDataLen);
        return true;
    }

    /** Calculate the number of char required by the storage format of this
     * data. The storage format is always UTF-8 or MBCS.
     *
     * @param a_pInputData  NULL terminated string to calculate the number of
     *                      bytes required to be converted to storage format.
     * @return              Number of bytes required by the string when
     *                      converted to storage format. This size always
     *                      includes space for the terminating NULL character.
     * @return              -1 cast to size_t on a conversion error.
     */
    size_t SizeToStore(
        const SI_CHAR* a_pInputData)
    {
        // ASCII/MBCS/UTF-8 needs no conversion
        return strlen((const char*)a_pInputData) + 1;
    }

    /** Convert the input string to the storage format of this data.
     * The storage format is always UTF-8 or MBCS.
     *
     * @param a_pInputData  NULL terminated source string to convert. All of
     *                      the data will be converted including the
     *                      terminating NULL character.
     * @param a_pOutputData Pointer to the buffer to receive the converted
     *                      string.
     * @param a_uOutputDataSize Size of the output buffer in char.
     * @return              true if all of the input data, including the
     *                      terminating NULL character was successfully
     *                      converted.
     */
    bool ConvertToStore(
        const SI_CHAR* a_pInputData,
        char*           a_pOutputData,
        size_t          a_uOutputDataSize)
    {
        // calc input string length (SI_CHAR type and size independent)
        size_t uInputLen = strlen((const char*)a_pInputData) + 1;
        if(uInputLen > a_uOutputDataSize)
        {
            return false;
        }

        // ascii/UTF-8 needs no conversion
        memcpy(a_pOutputData, a_pInputData, uInputLen);
        return true;
    }
};


// ---------------------------------------------------------------------------
//                              SI_CONVERT_GENERIC
// ---------------------------------------------------------------------------
#ifdef SI_CONVERT_GENERIC

#define SI_Case     SI_GenericCase
#define SI_NoCase   SI_GenericNoCase

#include <wchar.h>
#include "ConvertUTF.h"

/**
 * Converts UTF-8 to a wchar_t (or equivalent) using the Unicode reference
 * library functions. This can be used on all platforms.
 */
template<class SI_CHAR>
class SI_ConvertW
{
    bool m_bStoreIsUtf8;
protected:
    SI_ConvertW() { }
public:
    SI_ConvertW(bool a_bStoreIsUtf8) : m_bStoreIsUtf8(a_bStoreIsUtf8) { }

    /* copy and assignment */
    SI_ConvertW(const SI_ConvertW & rhs)
    {
        operator=(rhs);
    }
    SI_ConvertW & operator=(const SI_ConvertW & rhs)
    {
        m_bStoreIsUtf8 = rhs.m_bStoreIsUtf8;
        return *this;
    }

    /** Calculate the number of SI_CHAR required for converting the input
     * from the storage format. The storage format is always UTF-8 or MBCS.
     *
     * @param a_pInputData  Data in storage format to be converted to SI_CHAR.
     * @param a_uInputDataLen Length of storage format data in bytes. This
     *                      must be the actual length of the data, including
     *                      NULL byte if NULL terminated string is required.
     * @return              Number of SI_CHAR required by the string when
     *                      converted. If there are embedded NULL bytes in the
     *                      input data, only the string up and not including
     *                      the NULL byte will be converted.
     * @return              -1 cast to size_t on a conversion error.
     */
    size_t SizeFromStore(
        const char*     a_pInputData,
        size_t          a_uInputDataLen)
    {
        SI_ASSERT(a_uInputDataLen != (size_t) - 1);

        if(m_bStoreIsUtf8)
        {
            // worst case scenario for UTF-8 to wchar_t is 1 char -> 1 wchar_t
            // so we just return the same number of characters required as for
            // the source text.
            return a_uInputDataLen;
        }

#if defined(SI_NO_MBSTOWCS_NULL) || (!defined(_MSC_VER) && !defined(_linux))
        // fall back processing for platforms that don't support a NULL dest to mbstowcs
        // worst case scenario is 1:1, this will be a sufficient buffer size
        (void)a_pInputData;
        return a_uInputDataLen;
#else
        // get the actual required buffer size
        return mbstowcs(NULL, a_pInputData, a_uInputDataLen);
#endif
    }

    /** Convert the input string from the storage format to SI_CHAR.
     * The storage format is always UTF-8 or MBCS.
     *
     * @param a_pInputData  Data in storage format to be converted to SI_CHAR.
     * @param a_uInputDataLen Length of storage format data in bytes. This
     *                       must be the actual length of the data, including
     *                       NULL byte if NULL terminated string is required.
     * @param a_pOutputData Pointer to the output buffer to received the
     *                       converted data.
     * @param a_uOutputDataSize Size of the output buffer in SI_CHAR.
     * @return              true if all of the input data was successfully
     *                       converted.
     */
    bool ConvertFromStore(
        const char*     a_pInputData,
        size_t          a_uInputDataLen,
        SI_CHAR*        a_pOutputData,
        size_t          a_uOutputDataSize)
    {
        if(m_bStoreIsUtf8)
        {
            // This uses the Unicode reference implementation to do the
            // conversion from UTF-8 to wchar_t. The required files are
            // ConvertUTF.h and ConvertUTF.c which should be included in
            // the distribution but are publically available from unicode.org
            // at http://www.unicode.org/Public/PROGRAMS/CVTUTF/
            ConversionResult retval;
            const UTF8* pUtf8 = (const UTF8*) a_pInputData;
            if(sizeof(wchar_t) == sizeof(UTF32))
            {
                UTF32* pUtf32 = (UTF32*) a_pOutputData;
                retval = ConvertUTF8toUTF32(
                             &pUtf8, pUtf8 + a_uInputDataLen,
                             &pUtf32, pUtf32 + a_uOutputDataSize,
                             lenientConversion);
            }
            else if(sizeof(wchar_t) == sizeof(UTF16))
            {
                UTF16* pUtf16 = (UTF16*) a_pOutputData;
                retval = ConvertUTF8toUTF16(
                             &pUtf8, pUtf8 + a_uInputDataLen,
                             &pUtf16, pUtf16 + a_uOutputDataSize,
                             lenientConversion);
            }
            return retval == conversionOK;
        }

        // convert to wchar_t
        size_t retval = mbstowcs(a_pOutputData,
                                 a_pInputData, a_uOutputDataSize);
        return retval != (size_t)(-1);
    }

    /** Calculate the number of char required by the storage format of this
     * data. The storage format is always UTF-8 or MBCS.
     *
     * @param a_pInputData  NULL terminated string to calculate the number of
     *                       bytes required to be converted to storage format.
     * @return              Number of bytes required by the string when
     *                       converted to storage format. This size always
     *                       includes space for the terminating NULL character.
     * @return              -1 cast to size_t on a conversion error.
     */
    size_t SizeToStore(
        const SI_CHAR* a_pInputData)
    {
        if(m_bStoreIsUtf8)
        {
            // worst case scenario for wchar_t to UTF-8 is 1 wchar_t -> 6 char
            size_t uLen = 0;
            while(a_pInputData[uLen])
            {
                ++uLen;
            }
            return (6 * uLen) + 1;
        }
        else
        {
            size_t uLen = wcstombs(NULL, a_pInputData, 0);
            if(uLen == (size_t)(-1))
            {
                return uLen;
            }
            return uLen + 1; // include NULL terminator
        }
    }

    /** Convert the input string to the storage format of this data.
     * The storage format is always UTF-8 or MBCS.
     *
     * @param a_pInputData  NULL terminated source string to convert. All of
     *                       the data will be converted including the
     *                       terminating NULL character.
     * @param a_pOutputData Pointer to the buffer to receive the converted
     *                       string.
     * @param a_uOutputDataSize Size of the output buffer in char.
     * @return              true if all of the input data, including the
     *                       terminating NULL character was successfully
     *                       converted.
     */
    bool ConvertToStore(
        const SI_CHAR* a_pInputData,
        char*           a_pOutputData,
        size_t          a_uOutputDataSize
    )
    {
        if(m_bStoreIsUtf8)
        {
            // calc input string length (SI_CHAR type and size independent)
            size_t uInputLen = 0;
            while(a_pInputData[uInputLen])
            {
                ++uInputLen;
            }
            ++uInputLen; // include the NULL char

            // This uses the Unicode reference implementation to do the
            // conversion from wchar_t to UTF-8. The required files are
            // ConvertUTF.h and ConvertUTF.c which should be included in
            // the distribution but are publically available from unicode.org
            // at http://www.unicode.org/Public/PROGRAMS/CVTUTF/
            ConversionResult retval;
            UTF8* pUtf8 = (UTF8*) a_pOutputData;
            if(sizeof(wchar_t) == sizeof(UTF32))
            {
                const UTF32* pUtf32 = (const UTF32*) a_pInputData;
                retval = ConvertUTF32toUTF8(
                             &pUtf32, pUtf32 + uInputLen,
                             &pUtf8, pUtf8 + a_uOutputDataSize,
                             lenientConversion);
            }
            else if(sizeof(wchar_t) == sizeof(UTF16))
            {
                const UTF16* pUtf16 = (const UTF16*) a_pInputData;
                retval = ConvertUTF16toUTF8(
                             &pUtf16, pUtf16 + uInputLen,
                             &pUtf8, pUtf8 + a_uOutputDataSize,
                             lenientConversion);
            }
            return retval == conversionOK;
        }
        else
        {
            size_t retval = wcstombs(a_pOutputData,
                                     a_pInputData, a_uOutputDataSize);
            return retval != (size_t) - 1;
        }
    }
};

#endif // SI_CONVERT_GENERIC


// ---------------------------------------------------------------------------
//                              SI_CONVERT_ICU
// ---------------------------------------------------------------------------
#ifdef SI_CONVERT_ICU

#define SI_Case     SI_GenericCase
#define SI_NoCase   SI_GenericNoCase

#include <unicode/ucnv.h>

/**
 * Converts MBCS/UTF-8 to UChar using ICU. This can be used on all platforms.
 */
template<class SI_CHAR>
class SI_ConvertW
{
    const char* m_pEncoding;
    UConverter* m_pConverter;
protected:
    SI_ConvertW() : m_pEncoding(NULL), m_pConverter(NULL) { }
public:
    SI_ConvertW(bool a_bStoreIsUtf8) : m_pConverter(NULL)
    {
        m_pEncoding = a_bStoreIsUtf8 ? "UTF-8" : NULL;
    }

    /* copy and assignment */
    SI_ConvertW(const SI_ConvertW & rhs)
    {
        operator=(rhs);
    }
    SI_ConvertW & operator=(const SI_ConvertW & rhs)
    {
        m_pEncoding = rhs.m_pEncoding;
        m_pConverter = NULL;
        return *this;
    }
    ~SI_ConvertW()
    {
        if(m_pConverter) ucnv_close(m_pConverter);
    }

    /** Calculate the number of UChar required for converting the input
     * from the storage format. The storage format is always UTF-8 or MBCS.
     *
     * @param a_pInputData  Data in storage format to be converted to UChar.
     * @param a_uInputDataLen Length of storage format data in bytes. This
     *                      must be the actual length of the data, including
     *                      NULL byte if NULL terminated string is required.
     * @return              Number of UChar required by the string when
     *                      converted. If there are embedded NULL bytes in the
     *                      input data, only the string up and not including
     *                      the NULL byte will be converted.
     * @return              -1 cast to size_t on a conversion error.
     */
    size_t SizeFromStore(
        const char*     a_pInputData,
        size_t          a_uInputDataLen)
    {
        SI_ASSERT(a_uInputDataLen != (size_t) - 1);

        UErrorCode nError;

        if(!m_pConverter)
        {
            nError = U_ZERO_ERROR;
            m_pConverter = ucnv_open(m_pEncoding, &nError);
            if(U_FAILURE(nError))
            {
                return (size_t) - 1;
            }
        }

        nError = U_ZERO_ERROR;
        int32_t nLen = ucnv_toUChars(m_pConverter, NULL, 0,
                                     a_pInputData, (int32_t) a_uInputDataLen, &nError);
        if(U_FAILURE(nError) && nError != U_BUFFER_OVERFLOW_ERROR)
        {
            return (size_t) - 1;
        }

        return (size_t) nLen;
    }

    /** Convert the input string from the storage format to UChar.
     * The storage format is always UTF-8 or MBCS.
     *
     * @param a_pInputData  Data in storage format to be converted to UChar.
     * @param a_uInputDataLen Length of storage format data in bytes. This
     *                      must be the actual length of the data, including
     *                      NULL byte if NULL terminated string is required.
     * @param a_pOutputData Pointer to the output buffer to received the
     *                      converted data.
     * @param a_uOutputDataSize Size of the output buffer in UChar.
     * @return              true if all of the input data was successfully
     *                      converted.
     */
    bool ConvertFromStore(
        const char*     a_pInputData,
        size_t          a_uInputDataLen,
        UChar*          a_pOutputData,
        size_t          a_uOutputDataSize)
    {
        UErrorCode nError;

        if(!m_pConverter)
        {
            nError = U_ZERO_ERROR;
            m_pConverter = ucnv_open(m_pEncoding, &nError);
            if(U_FAILURE(nError))
            {
                return false;
            }
        }

        nError = U_ZERO_ERROR;
        ucnv_toUChars(m_pConverter,
                      a_pOutputData, (int32_t) a_uOutputDataSize,
                      a_pInputData, (int32_t) a_uInputDataLen, &nError);
        if(U_FAILURE(nError))
        {
            return false;
        }

        return true;
    }

    /** Calculate the number of char required by the storage format of this
     * data. The storage format is always UTF-8 or MBCS.
     *
     * @param a_pInputData  NULL terminated string to calculate the number of
     *                      bytes required to be converted to storage format.
     * @return              Number of bytes required by the string when
     *                      converted to storage format. This size always
     *                      includes space for the terminating NULL character.
     * @return              -1 cast to size_t on a conversion error.
     */
    size_t SizeToStore(
        const UChar* a_pInputData)
    {
        UErrorCode nError;

        if(!m_pConverter)
        {
            nError = U_ZERO_ERROR;
            m_pConverter = ucnv_open(m_pEncoding, &nError);
            if(U_FAILURE(nError))
            {
                return (size_t) - 1;
            }
        }

        nError = U_ZERO_ERROR;
        int32_t nLen = ucnv_fromUChars(m_pConverter, NULL, 0,
                                       a_pInputData, -1, &nError);
        if(U_FAILURE(nError) && nError != U_BUFFER_OVERFLOW_ERROR)
        {
            return (size_t) - 1;
        }

        return (size_t) nLen + 1;
    }

    /** Convert the input string to the storage format of this data.
     * The storage format is always UTF-8 or MBCS.
     *
     * @param a_pInputData  NULL terminated source string to convert. All of
     *                      the data will be converted including the
     *                      terminating NULL character.
     * @param a_pOutputData Pointer to the buffer to receive the converted
     *                      string.
     * @param a_pOutputDataSize Size of the output buffer in char.
     * @return              true if all of the input data, including the
     *                      terminating NULL character was successfully
     *                      converted.
     */
    bool ConvertToStore(
        const UChar*    a_pInputData,
        char*           a_pOutputData,
        size_t          a_uOutputDataSize)
    {
        UErrorCode nError;

        if(!m_pConverter)
        {
            nError = U_ZERO_ERROR;
            m_pConverter = ucnv_open(m_pEncoding, &nError);
            if(U_FAILURE(nError))
            {
                return false;
            }
        }

        nError = U_ZERO_ERROR;
        ucnv_fromUChars(m_pConverter,
                        a_pOutputData, (int32_t) a_uOutputDataSize,
                        a_pInputData, -1, &nError);
        if(U_FAILURE(nError))
        {
            return false;
        }

        return true;
    }
};

#endif // SI_CONVERT_ICU


// ---------------------------------------------------------------------------
//                              SI_CONVERT_WIN32
// ---------------------------------------------------------------------------
#ifdef SI_CONVERT_WIN32

#define SI_Case     SI_GenericCase

// Windows CE doesn't have errno or MBCS libraries
#ifdef _WIN32_WCE
# ifndef SI_NO_MBCS
#  define SI_NO_MBCS
# endif
#endif

#include <windows.h>
#ifdef SI_NO_MBCS
# define SI_NoCase   SI_GenericNoCase
#else // !SI_NO_MBCS
/**
 * Case-insensitive comparison class using Win32 MBCS functions. This class
 * returns a case-insensitive semi-collation order for MBCS text. It may not
 * be safe for UTF-8 text returned in char format as we don't know what
 * characters will be folded by the function! Therefore, if you are using
 * SI_CHAR == char and SetUnicode(true), then you need to use the generic
 * SI_NoCase class instead.
 */
#include <mbstring.h>
template<class SI_CHAR>
struct SI_NoCase
{
    bool operator()(const SI_CHAR* pLeft, const SI_CHAR* pRight) const
    {
        if(sizeof(SI_CHAR) == sizeof(char))
        {
            return _mbsicmp((const unsigned char*)pLeft,
                            (const unsigned char*)pRight) < 0;
        }
        if(sizeof(SI_CHAR) == sizeof(wchar_t))
        {
            return _wcsicmp((const wchar_t*)pLeft,
                            (const wchar_t*)pRight) < 0;
        }
        return SI_GenericNoCase<SI_CHAR>()(pLeft, pRight);
    }
};
#endif // SI_NO_MBCS

/**
 * Converts MBCS and UTF-8 to a wchar_t (or equivalent) on Windows. This uses
 * only the Win32 functions and doesn't require the external Unicode UTF-8
 * conversion library. It will not work on Windows 95 without using Microsoft
 * Layer for Unicode in your application.
 */
template<class SI_CHAR>
class SI_ConvertW
{
    UINT m_uCodePage;
protected:
    SI_ConvertW() { }
public:
    SI_ConvertW(bool a_bStoreIsUtf8)
    {
        m_uCodePage = a_bStoreIsUtf8 ? CP_UTF8 : CP_ACP;
    }

    /* copy and assignment */
    SI_ConvertW(const SI_ConvertW & rhs)
    {
        operator=(rhs);
    }
    SI_ConvertW & operator=(const SI_ConvertW & rhs)
    {
        m_uCodePage = rhs.m_uCodePage;
        return *this;
    }

    /** Calculate the number of SI_CHAR required for converting the input
     * from the storage format. The storage format is always UTF-8 or MBCS.
     *
     * @param a_pInputData  Data in storage format to be converted to SI_CHAR.
     * @param a_uInputDataLen Length of storage format data in bytes. This
     *                      must be the actual length of the data, including
     *                      NULL byte if NULL terminated string is required.
     * @return              Number of SI_CHAR required by the string when
     *                      converted. If there are embedded NULL bytes in the
     *                      input data, only the string up and not including
     *                      the NULL byte will be converted.
     * @return              -1 cast to size_t on a conversion error.
     */
    size_t SizeFromStore(
        const char*     a_pInputData,
        size_t          a_uInputDataLen)
    {
        SI_ASSERT(a_uInputDataLen != (size_t) - 1);

        int retval = MultiByteToWideChar(
                         m_uCodePage, 0,
                         a_pInputData, (int) a_uInputDataLen,
                         0, 0);
        return (size_t)(retval > 0 ? retval : -1);
    }

    /** Convert the input string from the storage format to SI_CHAR.
     * The storage format is always UTF-8 or MBCS.
     *
     * @param a_pInputData  Data in storage format to be converted to SI_CHAR.
     * @param a_uInputDataLen Length of storage format data in bytes. This
     *                      must be the actual length of the data, including
     *                      NULL byte if NULL terminated string is required.
     * @param a_pOutputData Pointer to the output buffer to received the
     *                      converted data.
     * @param a_uOutputDataSize Size of the output buffer in SI_CHAR.
     * @return              true if all of the input data was successfully
     *                      converted.
     */
    bool ConvertFromStore(
        const char*     a_pInputData,
        size_t          a_uInputDataLen,
        SI_CHAR*        a_pOutputData,
        size_t          a_uOutputDataSize)
    {
        int nSize = MultiByteToWideChar(
                        m_uCodePage, 0,
                        a_pInputData, (int) a_uInputDataLen,
                        (wchar_t*) a_pOutputData, (int) a_uOutputDataSize);
        return (nSize > 0);
    }

    /** Calculate the number of char required by the storage format of this
     * data. The storage format is always UTF-8.
     *
     * @param a_pInputData  NULL terminated string to calculate the number of
     *                      bytes required to be converted to storage format.
     * @return              Number of bytes required by the string when
     *                      converted to storage format. This size always
     *                      includes space for the terminating NULL character.
     * @return              -1 cast to size_t on a conversion error.
     */
    size_t SizeToStore(
        const SI_CHAR* a_pInputData)
    {
        int retval = WideCharToMultiByte(
                         m_uCodePage, 0,
                         (const wchar_t*) a_pInputData, -1,
                         0, 0, 0, 0);
        return (size_t)(retval > 0 ? retval : -1);
    }

    /** Convert the input string to the storage format of this data.
     * The storage format is always UTF-8 or MBCS.
     *
     * @param a_pInputData  NULL terminated source string to convert. All of
     *                      the data will be converted including the
     *                      terminating NULL character.
     * @param a_pOutputData Pointer to the buffer to receive the converted
     *                      string.
     * @param a_pOutputDataSize Size of the output buffer in char.
     * @return              true if all of the input data, including the
     *                      terminating NULL character was successfully
     *                      converted.
     */
    bool ConvertToStore(
        const SI_CHAR* a_pInputData,
        char*           a_pOutputData,
        size_t          a_uOutputDataSize)
    {
        int retval = WideCharToMultiByte(
                         m_uCodePage, 0,
                         (const wchar_t*) a_pInputData, -1,
                         a_pOutputData, (int) a_uOutputDataSize, 0, 0);
        return retval > 0;
    }
};

#endif // SI_CONVERT_WIN32


// ---------------------------------------------------------------------------
//                                  TYPE DEFINITIONS
// ---------------------------------------------------------------------------

typedef CSimpleIniTempl<char,
        SI_NoCase<char>, SI_ConvertA<char> >                 CSimpleIniA;
typedef CSimpleIniTempl<char,
        SI_Case<char>, SI_ConvertA<char> >                   CSimpleIniCaseA;

#if defined(SI_CONVERT_ICU)
typedef CSimpleIniTempl<UChar,
        SI_NoCase<UChar>, SI_ConvertW<UChar> >               CSimpleIniW;
typedef CSimpleIniTempl<UChar,
        SI_Case<UChar>, SI_ConvertW<UChar> >                 CSimpleIniCaseW;
#else
typedef CSimpleIniTempl<wchar_t,
        SI_NoCase<wchar_t>, SI_ConvertW<wchar_t> >           CSimpleIniW;
typedef CSimpleIniTempl<wchar_t,
        SI_Case<wchar_t>, SI_ConvertW<wchar_t> >             CSimpleIniCaseW;
#endif

#ifdef _UNICODE
# define CSimpleIni      CSimpleIniW
# define CSimpleIniCase  CSimpleIniCaseW
# define SI_NEWLINE      SI_NEWLINE_W
#else // !_UNICODE
# define CSimpleIni      CSimpleIniA
# define CSimpleIniCase  CSimpleIniCaseA
# define SI_NEWLINE      SI_NEWLINE_A
#endif // _UNICODE

#ifdef _MSC_VER
# pragma warning (pop)
#endif

#endif // INCLUDED_SimpleIni_h

