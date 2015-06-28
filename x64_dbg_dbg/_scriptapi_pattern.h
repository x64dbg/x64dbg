#ifndef _SCRIPTAPI_PATTERN_H
#define _SCRIPTAPI_PATTERN_H

#include "_scriptapi.h"

namespace Script
{
namespace Pattern
{
duint Find(unsigned char* data, duint datasize, const char* pattern);
duint FindMem(duint start, duint size, const char* pattern);
void Write(unsigned char* data, duint datasize, const char* pattern);
void WriteMem(duint start, duint size, const char* pattern);
bool SearchAndReplace(unsigned char* data, duint datasize, const char* searchpattern, const char* replacepattern);
bool SearchAndReplaceMem(duint start, duint size, const char* searchpattern, const char* replacepattern);
};
};

#endif //_SCRIPTAPI_FIND_H