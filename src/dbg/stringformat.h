#ifndef _STRINGFORMAT_H
#define _STRINGFORMAT_H

#include "_global.h"

typedef const char* FormatValueType;
typedef std::vector<FormatValueType> FormatValueVector;

String stringformat(String format, const FormatValueVector & values);
String stringformatinline(String format);

#endif //_STRINGFORMAT_H