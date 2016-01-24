#ifndef _EXCEPTION_H
#define _EXCEPTION_H

#define MS_VC_EXCEPTION 0x406D1388

void ExceptionCodeInit();
const char* ExceptionCodeToName(unsigned int ExceptionCode);

#endif // _EXCEPTION_H