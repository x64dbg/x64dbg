#include <sstream>
#include "stringutils.h"
#include "memory.h"
#include "dynamicmem.h"
#include <windows.h>
#include <cstdint>

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
        auto ch = uint8_t(s[i]);
        switch(ch)
        {
        case '\0':
            escaped += "\\0";
            break;
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
                sprintf_s(buf, "\\x%02X", ch);
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
    auto requiredSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if(requiredSize > 0)
    {
        std::vector<char> buffer(requiredSize);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &buffer[0], requiredSize, nullptr, nullptr);
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
    int requiredSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
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

//Taken from: http://stackoverflow.com/a/24315631
void StringUtils::ReplaceAll(String & s, const String & from, const String & to)
{
    size_t start_pos = 0;
    while((start_pos = s.find(from, start_pos)) != std::string::npos)
    {
        s.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
}

void StringUtils::ReplaceAll(WString & s, const WString & from, const WString & to)
{
    size_t start_pos = 0;
    while((start_pos = s.find(from, start_pos)) != std::string::npos)
    {
        s.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
}

String StringUtils::sprintf(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    Memory<char*> buffer(256 * sizeof(char), "StringUtils::sprintf");
    while(true)
    {
        int res = _vsnprintf_s(buffer(), buffer.size(), _TRUNCATE, format, args);
        if(res == -1)
        {
            buffer.realloc(buffer.size() * 2, "StringUtils::sprintf");
            continue;
        }
        else
            break;
    }
    va_end(args);
    return String(buffer());
}

WString StringUtils::sprintf(const wchar_t* format, ...)
{
    va_list args;
    va_start(args, format);
    Memory<wchar_t*> buffer(256 * sizeof(wchar_t), "StringUtils::sprintf");
    while(true)
    {
        int res = _vsnwprintf_s(buffer(), buffer.size(), _TRUNCATE, format, args);
        if(res == -1)
        {
            buffer.realloc(buffer.size() * 2, "StringUtils::sprintf");
            continue;
        }
        else
            break;
    }
    va_end(args);
    return WString(buffer());
}

String StringUtils::ToLower(const String & s)
{
    auto result = s;
    for(size_t i = 0; i < result.size(); i++)
        result[i] = tolower(result[i]);
    return result;
}

bool StringUtils::StartsWith(const String & h, const String & n)
{
    return strstr(h.c_str(), n.c_str()) == h.c_str();
}
