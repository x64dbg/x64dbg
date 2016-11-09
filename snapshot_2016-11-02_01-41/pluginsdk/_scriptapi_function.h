#ifndef _SCRIPTAPI_FUNCTION_H
#define _SCRIPTAPI_FUNCTION_H

#include "_scriptapi.h"

namespace Script
{
    namespace Function
    {
        struct FunctionInfo
        {
            char mod[MAX_MODULE_SIZE];
            duint rvaStart;
            duint rvaEnd;
            bool manual;
            duint instructioncount;
        };

        SCRIPT_EXPORT bool Add(duint start, duint end, bool manual, duint instructionCount = 0);
        SCRIPT_EXPORT bool Add(const FunctionInfo* info);
        SCRIPT_EXPORT bool Get(duint addr, duint* start = nullptr, duint* end = nullptr, duint* instructionCount = nullptr);
        SCRIPT_EXPORT bool GetInfo(duint addr, FunctionInfo* info);
        SCRIPT_EXPORT bool Overlaps(duint start, duint end);
        SCRIPT_EXPORT bool Delete(duint address);
        SCRIPT_EXPORT void DeleteRange(duint start, duint end, bool deleteManual);
        SCRIPT_EXPORT void DeleteRange(duint start, duint end);
        SCRIPT_EXPORT void Clear();
        SCRIPT_EXPORT bool GetList(ListOf(FunctionInfo) list); //caller has the responsibility to free the list
    }; //Function
}; //Script

#endif //_SCRIPTAPI_FUNCTION_H
