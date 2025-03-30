#include "stringutils.h"
#include <windows.h>
#include <iostream>
#include <sstream>

// Functions from x64dbg project: https://github.com/x64dbg/x64dbg
//Conversion functions taken from: http://www.nubaria.com/en/blog/?p=289
String Utf16ToUtf8(const WString & wstr)
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

String Utf16ToUtf8(const wchar_t* wstr)
{
    return Utf16ToUtf8(wstr ? WString(wstr) : WString());
}

WString Utf8ToUtf16(const String & str)
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

WString Utf8ToUtf16(const char* str)
{
    return Utf8ToUtf16(str ? String(str) : String());
}