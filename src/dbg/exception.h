#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#define MS_VC_EXCEPTION 0x406D1388

#include "_global.h"
#include "stringutils.h"

bool ExceptionCodeInit(const String & exceptionFile);
const String & ExceptionCodeToName(unsigned int ExceptionCode);
bool NtStatusCodeInit(const String & ntStatusFile);
const String & NtStatusCodeToName(unsigned int NtStatusCode);
bool ErrorCodeInit(const String & errorFile);
const String & ErrorCodeToName(unsigned int ErrorCode);
bool ExceptionNameToCode(const char* Name, unsigned int* ErrorCode);

#endif // _EXCEPTION_H