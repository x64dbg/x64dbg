#ifndef _STRINGUTILS_H
#define _STRINGUTILS_H

#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

typedef std::string String;
typedef std::wstring WString;
typedef std::vector<String> StringList;
typedef std::vector<WString> WStringList;

class StringUtils
{
public:
    static void Split(const String & s, char delim, std::vector<String> & elems);
    static StringList Split(const String & s, char delim);
    static String Escape(unsigned char ch, bool escapeSafe = true);
    static String Escape(const String & s, bool escapeSafe = true);
    static bool Unescape(const String & s, String & result, bool quoted = true);
    static String Trim(const String & s, const String & delim = StringUtils::WHITESPACE);
    static String TrimLeft(const String & s, const String & delim = StringUtils::WHITESPACE);
    static String TrimRight(const String & s, const String & delim = StringUtils::WHITESPACE);
    static String PadLeft(const String & s, size_t minLength, char ch);
    static String Utf16ToUtf8(const WString & wstr);
    static String Utf16ToUtf8(const wchar_t* wstr);
    static String Utf16ToLocalCp(const WString & wstr);
    static WString Utf8ToUtf16(const String & str);
    static WString Utf8ToUtf16(const char* str);
    static String LocalCpToUtf8(const String & str);
    static String LocalCpToUtf8(const char* str);
    static WString LocalCpToUtf16(const String & wstr);
    static WString LocalCpToUtf16(const char* wstr);
    static void ReplaceAll(String & s, const String & from, const String & to);
    static void ReplaceAll(WString & s, const WString & from, const WString & to);
    static String vsprintf(_In_z_ _Printf_format_string_ const char* format, va_list args);
    static String sprintf(_In_z_ _Printf_format_string_ const char* format, ...);
    static WString vsprintf(_In_z_ _Printf_format_string_ const wchar_t* format, va_list args);
    static WString sprintf(_In_z_ _Printf_format_string_ const wchar_t* format, ...);
    static String ToLower(const String & s);
    static bool StartsWith(const String & str, const String & prefix);
    static bool EndsWith(const String & str, const String & suffix);
    static bool FromHex(const String & text, std::vector<unsigned char> & data, bool reverse = false);
    static String ToHex(unsigned long long value);
    static String ToHex(const unsigned char* buffer, size_t size, bool reverse = false);
    static String ToCompressedHex(const unsigned char* buffer, size_t size);
    static bool FromCompressedHex(const String & text, std::vector<unsigned char> & data);
    static int hackicmp(const char* s1, const char* s2);

    static char ToLower(char ch)
    {
        return ch + (ch >= 'A' && ch <= 'Z') * 32;
    }

    template<typename T>
    static String ToFloatingString(void* buffer)
    {
        auto value = *(T*)buffer;
        std::stringstream wFloatingStr;
        wFloatingStr << std::setprecision(std::numeric_limits<T>::digits10) << value;
        return wFloatingStr.str();
    }

    template<typename T>
    static String ToIntegralString(void* buffer)
    {
        auto value = *(T*)buffer;
        return ToHex(value);
    }

    struct CaseInsensitiveLess
    {
        bool operator()(const std::string & str1, const std::string & str2) const
        {
            return _stricmp(str1.c_str(), str2.c_str()) < 0;
        }
    };

    struct CaseInsensitiveHash
    {
        size_t operator()(const std::string & str) const
        {
            size_t hash = 0;
            for(auto ch : str)
            {
                hash = ToLower(ch) + 65599 * hash;
            }
            return hash;
        }
    };

    struct CaseInsensitiveEqual
    {
        bool operator()(const std::string & str1, const std::string & str2) const
        {
            return _stricmp(str1.c_str(), str2.c_str()) == 0;
        }
    };

private:
    static const String WHITESPACE;
};

#endif //_STRINGUTILS_H