#ifndef _USTRING_H
#define _USTRING_H

#include "String.h"
#include <string>

typedef UTF8::String UString;

UString ConvertUtf16ToUtf8(const std::wstring & wstr);
std::wstring ConvertUtf8ToUtf16(const UString & str);

#endif // _USTRING_H
