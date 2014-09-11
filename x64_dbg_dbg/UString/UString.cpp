#include "UString.h"
#include <windows.h>

//Functions taken from: http://www.nubaria.com/en/blog/?p=289
UString ConvertUtf16ToUtf8(const std::wstring & wstr)
{
    std::string convertedString;
    int requiredSize = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, 0, 0, 0, 0);
    if(requiredSize > 0)
    {
        std::vector<char> buffer(requiredSize);
        WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &buffer[0], requiredSize, 0, 0);
        convertedString.assign(buffer.begin(), buffer.end() - 1);
    }
    return convertedString;
}

std::wstring ConvertUtf8ToUtf16(const UString & str)
{
    std::wstring convertedString;
    int requiredSize = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, 0, 0);
    if(requiredSize > 0)
    {
        std::vector<wchar_t> buffer(requiredSize);
        MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &buffer[0], requiredSize);
        convertedString.assign(buffer.begin(), buffer.end() - 1);
    }
    return convertedString;
}
