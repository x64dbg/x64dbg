#ifndef _SCRIPTAPI_DEBUG_H
#define _SCRIPTAPI_DEBUG_H

#include "_scriptapi.h"

namespace Script
{
    namespace Debug
    {
        enum HardwareType
        {
            HardwareAccess,
            HardwareWrite,
            HardwareExecute
        };

        SCRIPT_EXPORT void Wait();
        SCRIPT_EXPORT void Run();
        SCRIPT_EXPORT void Pause();
        SCRIPT_EXPORT void Stop();
        SCRIPT_EXPORT void StepIn();
        SCRIPT_EXPORT void StepOver();
        SCRIPT_EXPORT void StepOut();
        SCRIPT_EXPORT bool SetBreakpoint(duint address);
        SCRIPT_EXPORT bool DeleteBreakpoint(duint address);
        SCRIPT_EXPORT bool DisableBreakpoint(duint address);
        SCRIPT_EXPORT bool SetHardwareBreakpoint(duint address, HardwareType type = HardwareExecute);
        SCRIPT_EXPORT bool DeleteHardwareBreakpoint(duint address);
    }; //Debug
}; //Script

#endif //_SCRIPTAPI_DEBUG_H