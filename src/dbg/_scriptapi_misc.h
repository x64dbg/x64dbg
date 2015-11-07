#ifndef _SCRIPTAPI_MISC_H
#define _SCRIPTAPI_MISC_H

#include "_scriptapi.h"

namespace Script
{
namespace Misc
{
SCRIPT_EXPORT bool ParseExpression(const char* expression, duint* value);
SCRIPT_EXPORT duint RemoteGetProcAddress(const char* module, const char* api);
SCRIPT_EXPORT duint ResolveLabel(const char* label);
SCRIPT_EXPORT void* Alloc(duint size);
SCRIPT_EXPORT void Free(void* ptr);
}; //Misc
}; //Script

#endif //_SCRIPTAPI_MISC_H