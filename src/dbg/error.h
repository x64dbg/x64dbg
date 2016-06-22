#ifndef _ERROR_H
#define _ERROR_H

#include "_global.h"

bool ErrorCodeInit(const String & errorFile);
String ErrorCodeToName(unsigned int ErrorCode);

#endif // _ERROR_H