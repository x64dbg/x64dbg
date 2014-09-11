/**
 * UTF8 string library.
 *
 * Allows to use native UTF8 sequences as a string class. Has many overloaded
 * operators that provides such features as concatenation, types converting and
 * much more.
 *
 * Distributed under GPL v3
 *
 * Author:
 *      Grigory Gorelov (gorelov@grigory.info)
 *      See more information on grigory.info
 */

#ifndef _UTF8_String_H
#define _UTF8_String_H

#include "Exception.h"
#include <string.h>
#include <stdint.h>
#include <fstream>
#include <vector>

namespace UTF8
{
/**
 * The only string class containing everything to work with UTF8 strings
 */
class String
{
public:
    static const int SearchDirectionFromLeftToRight = 1;
    static const int SearchDirectionFromRightToLeft = 2;

    /**
     * Search substring in string
     * @param StartPosition Position to start search
     * @param Direction Search forward or backward, uses SearchDirectionFromLeftToRight and SearchDirectionFromRightToLeft
     * @return Returns position of substring if found. Otherwise returns -1
     */
    long Search(const UTF8::String & SubString, unsigned int StartPosition = 0, int Direction = SearchDirectionFromLeftToRight) const;

    /// Simple constructor only initiates buffers
    String();

    /**
     * Create string object from UTF8 char * string
     */
    String(const char* str);

    /**
     * Create string object from UTF-32 string
     */
    String(const uint32_t*);

    /**
     * Create string object from UTF8 std::string
     */
    String(const std::string &);

    /**
     * Copying constructor. Feel free to such things UTF8::String s2=s1;
     */
    String(const String & orig);

    /**
     * Deconstructor.
     */
    ~String();

    /**
     * Converts UTF8::String to std::string
     */
    std::string ToString() const;

    /**
     * Reads content from a file and returns as UTF8::String
     */
    static String FromFile(const UTF8::String & Path);

    /**
     * Converts UTF8::String to const char *
     */
    const char* ToConstCharPtr() const;

    /**
     * Converts UTF8::String to const char *
     */
    const char* c_str() const;

    /**
     * Separates string using given separator and returns vector
     */
    std::vector <String> Explode(const String & Separator) const;

    /**
     * Creating String from array of String adding separator between them.
     */
    static String Implode(const std::vector <String> & Strings, const String & Separator);

    /**
     * Sum operator. Provides String1+String2 exression.
     */
    String operator+(const String &) const;

    /**
     * Sum operator. Provides String1+"Str" exression.
     */
    String operator+(const char*) const;

    /**
     * Unary sum operator. Provides String1+=String2 expression.
     */
    String & operator+=(const String &);

    /**
     * Assign operator. Provides String1=String2 expression.
     */
    String & operator=(const String &);

    /**
     * Assign operator. Provides String1="New value" expression.
     */
    String & operator=(const char*);

    /**
     * Assign operator. Provides String1=(uint32_t*) UTF32_StringPointer expression.
     * Automatically converts UNICODE to UTF-8 ans stores in itself
     */
    String & operator=(const uint32_t*);

    /**
     * Provides std::string test=String expression.
     */
    operator std::string() const;

    /**
     * Returns substring of current string.
     * @param Start Start position of substring
     * @param Count Number of sybmols after start position. If number==0 string from Start till end is returned.
     */
    String Substring(unsigned int Start, unsigned int Count = 0) const;

    /**
     * Replaces one text peace by another and returns result
     * @param Search Search string
     * @param Replace Replace string
     * @return Returns result of replacement
     */
    String Replace(const String & Search, const String & Replace) const;

    /**
     * Returns trimmed string. Removes whitespaces from left and right
     */
    String Trim() const;

    /**
     * Returns string with nice quotes like this « ».
     */
    String Quote() const;

    /**
     * Replaces region of string by text peace and returns result.
     * @param Search Search string
     * @param Replace Replace string
     * @return Returns result of replacement
     */
    String SubstringReplace(unsigned int Start, unsigned int Count, const String & Replace) const;

    /**
     * Returns position of substring in current string.
     * @param Start Position to start search. Default is 0.
     * @return If substring not found returns -1.
     */
    long GetSubstringPosition(const UTF8::String & SubString, unsigned int Start = 0) const;

    /**
     * Get one char operator. Provides UTF8::String c=String1[1];
     */
    String operator[](unsigned int const) const;

    /**
     * Test operator. Provides String1==String2 expression.
     */
    bool operator==(const UTF8::String &) const;

    /**
     * Test operator. Provides String1!=String2 expression.
     */
    bool operator!=(const UTF8::String &) const;

    /**
     * Test operator. Provides String1=="Test" expression.
     */
    bool operator==(const char*) const;

    /**
     * Test operator. Provides String1!="Test" expression.
     */
    bool operator!=(const char*) const;

    /** Test operator. Provides String1<String2 expression.
     * Operator compares left characters of two strings.
     * If String1[0] value is less then the String2[0] returns true.
     * If they are equal then goes to second character and so on.
     * Can be used to sort strings alphabetical.
     */
    bool operator<(const UTF8::String &) const;

    /** Test operator. Provides String1>String2 expression.
     * Operator compares left characters of two strings.
     * If String1[0] value is greater then the String2[0] returns true.
     * If they are equal then goes to second character and so on.
     * Can be used to sort strings alphabetical.
     */
    bool operator>(const UTF8::String &) const;

    /**
     * Returns current string length. Also see DataLength to get buffer
     * size
     */
    unsigned int Length() const;

    /**
     * Returns current char data array length, containig UTF8 string.
     * As one character in UTF8 can be stored by more then one byte use
     * this function to know how much memory allocated for the string.
     */
    unsigned int DataLength() const;

    /**
     * Clears current string as if it is just created
     */
    void Empty();

    /**
     * If string is a one character check if it is one of given
     */
    bool CharacterIsOneOfThese(const UTF8::String & Characters) const;

    /**
     * Checks if this string contains given another string
     */
    bool HasThisString(const UTF8::String & Str) const;

    /**
     * Special function to convert from very big integers
     * Normally it is ok to assing UTF8::String to number. Or construct from it.
     * This function exists only for very very big integers conversion.
     */
    void ConvertFromInt64(int64_t n);

private:
    char* Data;
    unsigned int DataArrayLength;
    unsigned int StringLength;

    unsigned int GetSequenceLength(const char* StartByte) const;
    void CheckIfStringIsCorrect(const char* str) const;
    void CalculateStringLength();

    void InitString();
    void AppendString(const char* str);
    void SetString(const char* str);
    int GetSymbolIndexInDataArray(unsigned int Position) const;

    void ConvertFromUTF32(const uint32_t*);

};
}

/**
 * Not in class overloaded operator +. Provides "Sample"+String1 expression.
 */
UTF8::String operator+(const char*, const UTF8::String &);

/**
 * Not in class overloaded operator +. Provides std::string("123")+String1 expression.
 */
UTF8::String operator+(const std::string &, const UTF8::String &);

/**
 * Not in class overloaded operator ==. Provides "Test"==String1 expression.
 */
bool operator==(const char*, const UTF8::String &);

/**
 * Not in class overloaded operator ==. Provides std::string==String1 expression.
 */
bool operator==(const std::string &, const UTF8::String &);

/**
 * Not in class overloaded operator !=. Provides "Test"!=String1 expression.
 */
bool operator!=(const char*, const UTF8::String &);

/**
 * Not in class overloaded operator !=. Provides std::string!=String1 expression.
 */
bool operator!=(const std::string &, const UTF8::String &);

/**
 * Overloading for cout. Provides std::cout << (UTF8::String) operation;
 */
std::ostream & operator<<(std::ostream & os, const UTF8::String & s);

#endif  /* _UTF8STRING_H */


