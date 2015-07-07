#ifndef _SCRIPTAPI_MISC_H
#define _SCRIPTAPI_MISC_H

#include "_scriptapi.h"

namespace Script
{
namespace Misc
{
SCRIPT_EXPORT bool ParseExpression(const char* expression, duint* value);
}; //Misc
}; //Script

#endif //_SCRIPTAPI_MISC_H