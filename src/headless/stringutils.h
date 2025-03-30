#ifndef _STRINGUTILS_H
#define _STRINGUTILS_H

#include <string>
#include <vector>

typedef std::string String;
typedef std::wstring WString;

String Utf16ToUtf8(const WString & wstr);
String Utf16ToUtf8(const wchar_t* wstr);
WString Utf8ToUtf16(const String & str);
WString Utf8ToUtf16(const char* str);

#endif //_STRINGUTILS_H