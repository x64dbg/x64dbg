#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#define MS_VC_EXCEPTION 0x406D1388

#include "_global.h"

bool ExceptionCodeInit(const String & exceptionFile);
String ExceptionCodeToName(unsigned int ExceptionCode);

#endif // _EXCEPTION_H