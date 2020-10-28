#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#define MS_VC_EXCEPTION 0x406D1388

#include "_global.h"
#include "_dbgfunctions.h"
#include "stringutils.h"

bool ExceptionCodeInit(const String & exceptionFile);
const String & ExceptionCodeToName(unsigned int ExceptionCode);
std::vector<CONSTANTINFO> ExceptionList();
bool NtStatusCodeInit(const String & ntStatusFile);
const String & NtStatusCodeToName(unsigned int NtStatusCode);
bool ErrorCodeInit(const String & errorFile);
const String & ErrorCodeToName(unsigned int ErrorCode);
std::vector<CONSTANTINFO> ErrorCodeList();
bool ExceptionNameToCode(const char* Name, unsigned int* ErrorCode);
bool ConstantCodeInit(const String & constantFile);
bool ConstantFromName(const String & name, duint & value);
std::vector<CONSTANTINFO> ConstantList();
bool SyscallInit();
const String & SyscallToName(unsigned int index);

#endif // _EXCEPTION_H