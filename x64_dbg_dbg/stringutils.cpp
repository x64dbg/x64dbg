#include "stringutils.h"
#include <windows.h>
#include <iostream>
#include <sstream>

StringList StringUtils::Split(const String & s, char delim, std::vector<String> & elems)
{
    std::stringstream ss(s);
    String item;
    while(std::getline(ss, item, delim))
    {
        if(!item.length())
            continue;
        elems.push_back(item);
    }
    return elems;
}

StringList StringUtils::Split(const String & s, char delim)
{
    std::vector<String> elems;
    Split(s, delim, elems);
    return elems;
}

String StringUtils::Escape(const String & s)
{
    String escaped = "";
    for(size_t i = 0; i < s.length(); i++)
    {
        char ch = s[i];
        switch(ch)
        {
        case '\t':
            escaped += "\\t";
            break;
        case '\f':
            escaped += "\\f";
            break;
        case '\v':
            escaped += "\\v";
            break;
        case '\n':
            escaped += "\\n";
            break;
        case '\r':
            escaped += "\\r";
            break;
        case '\\':
            escaped += "\\\\";
            break;
        case '\"':
            escaped += "\\\"";
            break;
        default:
            if(!isprint(ch)) //unknown unprintable character
            {
                char buf[16] = "";
                sprintf_s(buf, "\\%.2X", ch);
                escaped += buf;
            }
            else
                escaped += ch;
            break;
        }
    }
    return escaped;
}

//Trim functions taken from: http://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring/16743707#16743707
const String StringUtils::WHITESPACE = " \n\r\t";

String StringUtils::Trim(const String & s)
{
    return TrimRight(TrimLeft(s));
}

String StringUtils::TrimLeft(const String & s)
{
    size_t startpos = s.find_first_not_of(StringUtils::WHITESPACE);
    return (startpos == String::npos) ? "" : s.substr(startpos);
}

String StringUtils::TrimRight(const String & s)
{
    size_t endpos = s.find_last_not_of(StringUtils::WHITESPACE);
    return (endpos == String::npos) ? "" : s.substr(0, endpos + 1);
}

//Conversion functions taken from: http://www.nubaria.com/en/blog/?p=289
String StringUtils::Utf16ToUtf8(const WString & wstr)
{
    String convertedString;
    int requiredSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, 0, 0, 0, 0);
    if(requiredSize > 0)
    {
        std::vector<char> buffer(requiredSize);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &buffer[0], requiredSize, 0, 0);
        convertedString.assign(buffer.begin(), buffer.end() - 1);
    }
    return convertedString;
}

String StringUtils::Utf16ToUtf8(const wchar_t* wstr)
{
    return Utf16ToUtf8(wstr ? WString(wstr) : WString());
}

WString StringUtils::Utf8ToUtf16(const String & str)
{
    WString convertedString;
    int requiredSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, 0, 0);
    if(requiredSize > 0)
    {
        std::vector<wchar_t> buffer(requiredSize);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &buffer[0], requiredSize);
        convertedString.assign(buffer.begin(), buffer.end() - 1);
    }
    return convertedString;
}

WString StringUtils::Utf8ToUtf16(const char* str)
{
    return Utf8ToUtf16(str ? String(str) : String());
}