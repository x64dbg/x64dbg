#ifndef _USTRING_H
#define _USTRING_H

#include "String.h"
#include <string>

typedef UTF8::String UString;

std::string ConvertUtf16ToUtf8(const std::wstring & wstr);
std::wstring ConvertUtf8ToUtf16(const std::string & str);

#endif // _USTRING_H
