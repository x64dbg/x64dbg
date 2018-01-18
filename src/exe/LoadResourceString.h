#include <string>

#include "strings.h" // String resources

inline std::wstring _LoadResString(UINT uID)
{
    wchar_t* p = nullptr;
    int len = ::LoadStringW(NULL, uID, reinterpret_cast<LPWSTR>(&p), 0);
    if(len > 0)
    {
        return std::wstring(p, len);
    }
    else
    {
        return std::wstring();
    }
}

#define LoadResString(uID) _LoadResString(uID).c_str()
