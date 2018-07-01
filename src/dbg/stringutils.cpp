#include "stringutils.h"
#include <windows.h>
#include <cstdint>

static inline bool convertLongLongNumber(const char* str, unsigned long long & result, int radix)
{
    errno = 0;
    char* end;
    result = strtoull(str, &end, radix);
    if(!result && end == str)
        return false;
    if(result == ULLONG_MAX && errno)
        return false;
    if(*end)
        return false;
    return true;
}

static inline bool convertNumber(const char* str, size_t & result, int radix)
{
    unsigned long long llr;
    if(!convertLongLongNumber(str, llr, radix))
        return false;
    result = size_t(llr);
    return true;
}

void StringUtils::Split(const String & s, char delim, std::vector<String> & elems)
{
    elems.clear();
    String item;
    item.reserve(s.length());
    for(size_t i = 0; i < s.length(); i++)
    {
        if(s[i] == delim)
        {
            if(!item.empty())
                elems.push_back(item);
            item.clear();
        }
        else
            item.push_back(s[i]);
    }
    if(!item.empty())
        elems.push_back(std::move(item));
}

StringList StringUtils::Split(const String & s, char delim)
{
    std::vector<String> elems;
    Split(s, delim, elems);
    return elems;
}

//https://github.com/lefticus/presentations/blob/master/PracticalPerformancePractices.md#smaller-code-is-faster-code-11
String StringUtils::Escape(unsigned char ch, bool escapeSafe)
{
    char buf[8] = "";
    switch(ch)
    {
    case '\0':
        return "\\0";
    case '\t':
        return escapeSafe ? "\\t" : "\t";
    case '\f':
        return "\\f";
    case '\v':
        return "\\v";
    case '\n':
        return escapeSafe ? "\\n" : "\n";
    case '\r':
        return escapeSafe ? "\\r" : "\r";
    case '\\':
        return escapeSafe ? "\\\\" : "\\";
    case '\"':
        return escapeSafe ? "\\\"" : "\"";
    case '\a':
        return "\\a";
    case '\b':
        return "\\b";
    default:
        if(!isprint(ch)) //unknown unprintable character
            sprintf_s(buf, "\\x%02X", ch);
        else
            *buf = ch;
        return buf;
    }
}

static int IsValidUTF8Char(const char* data, int size)
{
    if(*(unsigned char*)data >= 0xF8) //5 or 6 bytes
        return 0;
    else if(*(unsigned char*)data >= 0xF0) //4 bytes
    {
        if(size < 4)
            return 0;
        for(int i = 1; i <= 3; i++)
        {
            if((*(unsigned char*)(data + i) & 0xC0) != 0x80)
                return 0;
        }
        return 4;
    }
    else if(*(unsigned char*)data >= 0xE0) //3 bytes
    {
        if(size < 3)
            return 0;
        for(int i = 1; i <= 2; i++)
        {
            if((*(unsigned char*)(data + i) & 0xC0) != 0x80)
                return 0;
        }
        return 3;
    }
    else if(*(unsigned char*)data >= 0xC0) //2 bytes
    {
        if(size < 2)
            return 0;
        if((*(unsigned char*)(data + 1) & 0xC0) != 0x80)
            return 0;
        return 2;
    }
    else if(*(unsigned char*)data >= 0x80) // BAD
        return 0;
    else
        return 1;
}

String StringUtils::Escape(const String & s, bool escapeSafe)
{
    std::string escaped;
    escaped.reserve(s.length() + s.length() / 2);
    for(size_t i = 0; i < s.length(); i++)
    {
        char buf[8];
        memset(buf, 0, sizeof(buf));
        unsigned char ch = (unsigned char)s[i];
        switch(ch)
        {
        case '\0':
            memcpy(buf, "\\0", 2);
            break;
        case '\t':
            if(escapeSafe)
                memcpy(buf, "\\t", 2);
            else
                memcpy(buf, &ch, 1);
            break;
        case '\f':
            memcpy(buf, "\\f", 2);
            break;
        case '\v':
            memcpy(buf, "\\v", 2);
            break;
        case '\n':
            if(escapeSafe)
                memcpy(buf, "\\n", 2);
            else
                memcpy(buf, &ch, 1);
            break;
        case '\r':
            if(escapeSafe)
                memcpy(buf, "\\r", 2);
            else
                memcpy(buf, &ch, 1);
            break;
        case '\\':
            if(escapeSafe)
                memcpy(buf, "\\\\", 2);
            else
                memcpy(buf, &ch, 1);
            break;
        case '\"':
            if(escapeSafe)
                memcpy(buf, "\\\"", 2);
            else
                memcpy(buf, &ch, 1);
            break;
        default:
            int UTF8CharSize;
            if(ch >= 0x80 && (UTF8CharSize = IsValidUTF8Char(s.c_str() + i, int(s.length() - i))) != 0) //UTF-8 Character is emitted directly
            {
                memcpy(buf, s.c_str() + i, UTF8CharSize);
                i += UTF8CharSize - 1;
            }
            else if(!isprint(ch)) //unknown unprintable character
                sprintf_s(buf, "\\x%02X", ch);
            else
                *buf = ch;
        }
        escaped.append(buf);
    }
    return escaped;
}

bool StringUtils::Unescape(const String & s, String & result, bool quoted)
{
    int mLastChar = EOF;
    size_t i = 0;
    auto nextChar = [&]()
    {
        if(i == s.length())
            return mLastChar = EOF;
        return mLastChar = s[i++];
    };
    if(quoted)
    {
        nextChar();
        if(mLastChar != '\"') //start of quoted string literal
            return false; //invalid string literal
    }
    result.reserve(s.length());
    while(true)
    {
        nextChar();
        if(mLastChar == EOF) //end of file
        {
            if(!quoted)
                break;
            return false; //unexpected end of file in string literal (1)
        }
        if(mLastChar == '\r' || mLastChar == '\n')
            return false; //unexpected newline in string literal (1)
        if(quoted && mLastChar == '\"') //end of quoted string literal
            break;
        if(mLastChar == '\\') //escape sequence
        {
            nextChar();
            if(mLastChar == EOF)
                return false; //unexpected end of file in string literal (2)
            if(mLastChar == '\r' || mLastChar == '\n')
                return false; //unexpected newline in string literal (2)
            if(mLastChar == '\'' || mLastChar == '\"' || mLastChar == '?' || mLastChar == '\\')
                mLastChar = mLastChar;
            else if(mLastChar == 'a')
                mLastChar = '\a';
            else if(mLastChar == 'b')
                mLastChar = '\b';
            else if(mLastChar == 'f')
                mLastChar = '\f';
            else if(mLastChar == 'n')
                mLastChar = '\n';
            else if(mLastChar == 'r')
                mLastChar = '\r';
            else if(mLastChar == 't')
                mLastChar = '\t';
            else if(mLastChar == 'v')
                mLastChar = '\v';
            else if(mLastChar == '0')
                mLastChar = '\0';
            else if(mLastChar == 'x') //\xHH
            {
                auto ch1 = nextChar();
                auto ch2 = nextChar();
                if(isxdigit(ch1) && isxdigit(ch2))
                {
                    char byteStr[3] = "";
                    byteStr[0] = ch1;
                    byteStr[1] = ch2;
                    uint64_t hexData;
                    auto error = convertLongLongNumber(byteStr, hexData, 16);
                    if(error)
                        return false; //convertNumber failed (%s) for hex sequence \"\\x%c%c\" in string literal
                    mLastChar = hexData & 0xFF;
                }
                else
                    return false; //invalid hex sequence \"\\x%c%c\" in string literal
            }
            else
                return false; //invalid escape sequence \"\\%c\" in string literal
        }
        result.push_back(mLastChar);
    }
    return true;
}

//Trim functions taken from: https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring/16743707#16743707
const String StringUtils::WHITESPACE = " \n\r\t";

String StringUtils::Trim(const String & s, const String & delim)
{
    return TrimRight(TrimLeft(s));
}

String StringUtils::TrimLeft(const String & s, const String & delim)
{
    size_t startpos = s.find_first_not_of(delim);
    return (startpos == String::npos) ? "" : s.substr(startpos);
}

String StringUtils::TrimRight(const String & s, const String & delim)
{
    size_t endpos = s.find_last_not_of(delim);
    return (endpos == String::npos) ? "" : s.substr(0, endpos + 1);
}

String StringUtils::PadLeft(const String & s, size_t minLength, char ch)
{
    if(s.length() >= minLength)
        return s;
    String pad;
    pad.resize(minLength - s.length());
    for(size_t i = 0; i < pad.length(); i++)
        pad[i] = ch;
    return pad + s;
}

//Conversion functions taken from: http://www.nubaria.com/en/blog/?p=289
String StringUtils::Utf16ToUtf8(const WString & wstr)
{
    return Utf16ToUtf8(wstr.c_str());
}

String StringUtils::Utf16ToUtf8(const wchar_t* wstr)
{
    String convertedString;
    if(!wstr || !*wstr)
        return convertedString;
    auto requiredSize = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    if(requiredSize > 0)
    {
        convertedString.resize(requiredSize - 1);
        if(!WideCharToMultiByte(CP_UTF8, 0, wstr, -1, (char*)convertedString.c_str(), requiredSize, nullptr, nullptr))
            convertedString.clear();
    }
    return convertedString;
}

WString StringUtils::Utf8ToUtf16(const String & str)
{
    return Utf8ToUtf16(str.c_str());
}

WString StringUtils::Utf8ToUtf16(const char* str)
{
    WString convertedString;
    if(!str || !*str)
        return convertedString;
    int requiredSize = MultiByteToWideChar(CP_UTF8, 0, str, -1, nullptr, 0);
    if(requiredSize > 0)
    {
        convertedString.resize(requiredSize - 1);
        if(!MultiByteToWideChar(CP_UTF8, 0, str, -1, (wchar_t*)convertedString.c_str(), requiredSize))
            convertedString.clear();
    }
    return convertedString;
}

String StringUtils::LocalCpToUtf8(const String & str)
{
    return LocalCpToUtf8(str.c_str());
}

String StringUtils::LocalCpToUtf8(const char* str)
{
    return Utf16ToUtf8(LocalCpToUtf16(str).c_str());
}

WString StringUtils::LocalCpToUtf16(const String & str)
{
    return LocalCpToUtf16(str.c_str());
}

WString StringUtils::LocalCpToUtf16(const char* str)
{
    WString convertedString;
    if(!str || !*str)
        return convertedString;
    int requiredSize = MultiByteToWideChar(CP_ACP, 0, str, -1, nullptr, 0);
    if(requiredSize > 0)
    {
        convertedString.resize(requiredSize - 1);
        if(!MultiByteToWideChar(CP_ACP, 0, str, -1, (wchar_t*)convertedString.c_str(), requiredSize))
            convertedString.clear();
    }
    return convertedString;
}

//Taken from: https://stackoverflow.com/a/24315631
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

String StringUtils::vsprintf(const char* format, va_list args)
{
    char sbuffer[64] = "";
    if(_vsnprintf_s(sbuffer, _TRUNCATE, format, args) != -1)
        return sbuffer;

    std::vector<char> buffer(256, '\0');
    while(true)
    {
        int res = _vsnprintf_s(buffer.data(), buffer.size(), _TRUNCATE, format, args);
        if(res == -1)
        {
            buffer.resize(buffer.size() * 2);
            continue;
        }
        else
            break;
    }
    return String(buffer.data());
}

String StringUtils::sprintf(_Printf_format_string_ const char* format, ...)
{
    va_list args;
    va_start(args, format);
    auto result = vsprintf(format, args);
    va_end(args);
    return result;
}

WString StringUtils::vsprintf(const wchar_t* format, va_list args)
{
    wchar_t sbuffer[64] = L"";
    if(_vsnwprintf_s(sbuffer, _TRUNCATE, format, args) != -1)
        return sbuffer;

    std::vector<wchar_t> buffer(256, L'\0');
    while(true)
    {
        int res = _vsnwprintf_s(buffer.data(), buffer.size(), _TRUNCATE, format, args);
        if(res == -1)
        {
            buffer.resize(buffer.size() * 2);
            continue;
        }
        else
            break;
    }
    return WString(buffer.data());
}

WString StringUtils::sprintf(_Printf_format_string_ const wchar_t* format, ...)
{
    va_list args;
    va_start(args, format);
    auto result = vsprintf(format, args);
    va_end(args);
    return result;
}

String StringUtils::ToLower(const String & s)
{
    auto result = s;
    for(size_t i = 0; i < result.size(); i++)
        result[i] = tolower(result[i]);
    return result;
}

bool StringUtils::StartsWith(const String & str, const String & prefix)
{
    return str.size() >= prefix.size() && 0 == str.compare(0, prefix.size(), prefix);
}

bool StringUtils::EndsWith(const String & str, const String & suffix)
{
    return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
}

static int hex2int(char ch)
{
    if(ch >= '0' && ch <= '9')
        return ch - '0';
    if(ch >= 'A' && ch <= 'F')
        return ch - 'A' + 10;
    if(ch >= 'a' && ch <= 'f')
        return ch - 'a' + 10;
    return -1;
}

bool StringUtils::FromHex(const String & text, std::vector<unsigned char> & data, bool reverse)
{
    auto size = text.size();
    if(size % 2)
        return false;
    data.resize(size / 2);
    for(size_t i = 0, j = 0; i < size; i += 2, j++)
    {
        auto high = hex2int(text[i]);
        auto low = hex2int(text[i + 1]);
        if(high == -1 || low == -1)
            return false;
        data[reverse ? data.size() - j - 1 : j] = (high << 4) | low;
    }
    return true;
}

String StringUtils::ToHex(unsigned long long value)
{
    char buf[32];
    sprintf_s(buf, "%llX", value);
    return buf;
}

#define HEXLOOKUP "0123456789ABCDEF"

String StringUtils::ToHex(unsigned char* buffer, size_t size, bool reverse)
{
    String result;
    result.resize(size * 2);
    for(size_t i = 0, j = 0; i < size; i++, j += 2)
    {
        auto ch = buffer[reverse ? size - i - 1 : i];
        result[j] = HEXLOOKUP[(ch >> 4) & 0xF];
        result[j + 1] = HEXLOOKUP[ch & 0xF];
    }
    return result;
}

String StringUtils::ToCompressedHex(unsigned char* buffer, size_t size)
{
    if(!size)
        return "";
    String result;
    result.reserve(size * 2);
    for(size_t i = 0; i < size;)
    {
        size_t repeat = 0;
        auto lastCh = buffer[i];
        result.push_back(HEXLOOKUP[(lastCh >> 4) & 0xF]);
        result.push_back(HEXLOOKUP[lastCh & 0xF]);
        for(; i < size && buffer[i] == lastCh; i++)
            repeat++;
        if(repeat == 2)
        {
            result.push_back(HEXLOOKUP[(lastCh >> 4) & 0xF]);
            result.push_back(HEXLOOKUP[lastCh & 0xF]);
        }
        else if(repeat > 2)
#ifdef _WIN64
            result.append(StringUtils::sprintf("{%llX}", repeat));
#else //x86
            result.append(StringUtils::sprintf("{%X}", repeat));
#endif //_WIN64
    }
    return result;
}

bool StringUtils::FromCompressedHex(const String & text, std::vector<unsigned char> & data)
{
    auto size = text.size();
    if(size < 2)
        return false;
    data.clear();
    data.reserve(size); //TODO: better initial estimate
    String repeatStr;
    for(size_t i = 0; i < size;)
    {
        if(isspace(text[i])) //skip whitespace
        {
            i++;
            continue;
        }
        auto high = hex2int(text[i++]); //eat high nibble
        if(i >= size) //not enough data
            return false;
        auto low = hex2int(text[i++]); //eat low nibble
        if(high == -1 || low == -1) //invalid character
            return false;
        auto lastCh = (high << 4) | low;
        data.push_back(lastCh);

        if(i >= size) //end of buffer
            break;

        if(text[i] == '{')
        {
            repeatStr.clear();
            i++; //eat '{'
            while(text[i] != '}')
            {
                repeatStr.push_back(text[i++]); //eat character
                if(i >= size) //unexpected end of buffer (missing '}')
                    return false;
            }
            i++; //eat '}'

            size_t repeat = 0;
            if(!convertNumber(repeatStr.c_str(), repeat, 16) || !repeat) //conversion failed or repeat zero times
                return false;
            for(size_t j = 1; j < repeat; j++)
                data.push_back(lastCh);
        }
    }
    return true;
}