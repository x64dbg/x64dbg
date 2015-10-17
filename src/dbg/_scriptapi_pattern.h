#ifndef _SCRIPTAPI_PATTERN_H
#define _SCRIPTAPI_PATTERN_H

#include "_scriptapi.h"

namespace Script
{
namespace Pattern
{
SCRIPT_EXPORT duint Find(unsigned char* data, duint datasize, const char* pattern);
SCRIPT_EXPORT duint FindMem(duint start, duint size, const char* pattern);
SCRIPT_EXPORT void Write(unsigned char* data, duint datasize, const char* pattern);
SCRIPT_EXPORT void WriteMem(duint start, duint size, const char* pattern);
SCRIPT_EXPORT bool SearchAndReplace(unsigned char* data, duint datasize, const char* searchpattern, const char* replacepattern);
SCRIPT_EXPORT bool SearchAndReplaceMem(duint start, duint size, const char* searchpattern, const char* replacepattern);
};
};

#endif //_SCRIPTAPI_FIND_H