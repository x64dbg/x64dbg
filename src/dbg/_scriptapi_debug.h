#ifndef _SCRIPTAPI_DEBUG_H
#define _SCRIPTAPI_DEBUG_H

#include "_scriptapi.h"
#include <functional>

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

        enum HardwareSize
        {
            HardwareByte,
            HardwareWord,
            HardwareDword,
            HardwareQword
        };

        using Callback = void (*)();
        using CallbackPtr = void (*)(void*);

        SCRIPT_EXPORT void Wait();
        SCRIPT_EXPORT void Run();
        SCRIPT_EXPORT void Pause();
        SCRIPT_EXPORT void Stop();
        SCRIPT_EXPORT void StepIn();
        SCRIPT_EXPORT void StepOver();
        SCRIPT_EXPORT void StepOut();
        SCRIPT_EXPORT bool SetBreakpoint(duint address);
        SCRIPT_EXPORT bool SetBreakpoint(duint address, Callback callback);
        SCRIPT_EXPORT bool SetBreakpoint(duint address, CallbackPtr callback, void* userdata);
        SCRIPT_EXPORT bool DeleteBreakpoint(duint address);
        SCRIPT_EXPORT bool DisableBreakpoint(duint address);
        SCRIPT_EXPORT bool SetHardwareBreakpoint(duint address, HardwareType type = HardwareExecute, HardwareSize size = HardwareByte); // binary-compatibility
        SCRIPT_EXPORT bool DeleteHardwareBreakpoint(duint address);
        SCRIPT_EXPORT void SetBreakpointCallback(BPXTYPE type, duint address, CallbackPtr callback, void* userdata);

        namespace Context
        {
            SCRIPT_EXPORT void Create();
            SCRIPT_EXPORT void Destroy();
            SCRIPT_EXPORT void AddFinalizer(CallbackPtr callback, void* userdata);
        } // Context

        namespace Internal
        {
            void BreakpointHandler(const BRIDGEBP & bp);
        }
    }; //Debug
}; //Script

#endif //_SCRIPTAPI_DEBUG_H