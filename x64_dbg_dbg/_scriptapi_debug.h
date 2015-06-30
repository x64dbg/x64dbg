#ifndef _SCRIPTAPI_DEBUG_H
#define _SCRIPTAPI_DEBUG_H

#include "_scriptapi.h"

namespace Script
{
namespace Debug
{
SCRIPT_EXPORT void Wait();
SCRIPT_EXPORT void Run();
SCRIPT_EXPORT void Pause();
SCRIPT_EXPORT void Stop();
SCRIPT_EXPORT void StepIn();
SCRIPT_EXPORT void StepOver();
SCRIPT_EXPORT void StepOut();
}; //Debug
}; //Script

#endif //_SCRIPTAPI_DEBUG_H